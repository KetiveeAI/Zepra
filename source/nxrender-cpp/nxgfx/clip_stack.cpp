// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "nxgfx/clip_stack.h"
#include <GL/gl.h>
#include <algorithm>

namespace NXRender {

ClipStack::ClipStack() {}
ClipStack::~ClipStack() {}

Rect ClipStack::intersectRects(const Rect& a, const Rect& b) const {
    float x1 = std::max(a.x, b.x);
    float y1 = std::max(a.y, b.y);
    float x2 = std::min(a.x + a.width, b.x + b.width);
    float y2 = std::min(a.y + a.height, b.y + b.height);

    if (x2 <= x1 || y2 <= y1) return Rect(0, 0, 0, 0);
    return Rect(x1, y1, x2 - x1, y2 - y1);
}

void ClipStack::pushRect(const Rect& rect) {
    ClipEntry entry;
    entry.type = ClipType::Rect;
    entry.rect = rect;

    if (stack_.empty()) {
        entry.effectiveRect = rect;
    } else {
        entry.effectiveRect = intersectRects(stack_.back().effectiveRect, rect);
    }

    entry.stencilRef = static_cast<int>(stack_.size());
    stack_.push_back(entry);
}

void ClipStack::pushRoundedRect(const Rect& rect, const CornerRadii& radii) {
    ClipEntry entry;
    entry.type = ClipType::RoundedRect;
    entry.rect = rect;
    entry.radii = radii;

    if (stack_.empty()) {
        entry.effectiveRect = rect;
    } else {
        entry.effectiveRect = intersectRects(stack_.back().effectiveRect, rect);
    }

    entry.stencilRef = static_cast<int>(stack_.size());
    stack_.push_back(entry);
}

void ClipStack::pop() {
    if (!stack_.empty()) {
        stack_.pop_back();
    }
}

void ClipStack::clear() {
    stack_.clear();
}

Rect ClipStack::currentClip() const {
    if (stack_.empty()) {
        return Rect(0, 0, 1e6f, 1e6f); // Effectively infinite
    }
    return stack_.back().effectiveRect;
}

bool ClipStack::isClipped(const Rect& rect) const {
    if (stack_.empty()) return false;
    Rect clip = currentClip();
    return !rect.intersects(clip);
}

bool ClipStack::isFullyVisible(const Rect& rect) const {
    if (stack_.empty()) return true;
    Rect clip = currentClip();
    return rect.x >= clip.x &&
           rect.y >= clip.y &&
           rect.x + rect.width <= clip.x + clip.width &&
           rect.y + rect.height <= clip.y + clip.height;
}

const ClipEntry* ClipStack::top() const {
    if (stack_.empty()) return nullptr;
    return &stack_.back();
}

void ClipStack::applyToGL(int viewportHeight) const {
    if (stack_.empty()) {
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_STENCIL_TEST);
        return;
    }

    // Check if all clips are simple rects
    bool allRects = true;
    for (const auto& entry : stack_) {
        if (entry.type != ClipType::Rect) {
            allRects = false;
            break;
        }
    }

    Rect clip = currentClip();

    if (allRects) {
        // Fast path: use scissor only
        glEnable(GL_SCISSOR_TEST);
        glDisable(GL_STENCIL_TEST);

        // GL scissor uses bottom-left origin
        int scissorX = static_cast<int>(clip.x);
        int scissorY = viewportHeight - static_cast<int>(clip.y + clip.height);
        int scissorW = static_cast<int>(clip.width);
        int scissorH = static_cast<int>(clip.height);

        // Clamp to positive values
        if (scissorW < 0) scissorW = 0;
        if (scissorH < 0) scissorH = 0;

        glScissor(scissorX, scissorY, scissorW, scissorH);
    } else {
        // Scissor for the bounding rect
        glEnable(GL_SCISSOR_TEST);
        int scissorX = static_cast<int>(clip.x);
        int scissorY = viewportHeight - static_cast<int>(clip.y + clip.height);
        int scissorW = std::max(0, static_cast<int>(clip.width));
        int scissorH = std::max(0, static_cast<int>(clip.height));
        glScissor(scissorX, scissorY, scissorW, scissorH);

        // Setup stencil for rounded-rect clips
        glEnable(GL_STENCIL_TEST);
        glClear(GL_STENCIL_BUFFER_BIT);

        // Write stencil for each rounded-rect clip
        int stencilRef = 0;
        for (const auto& entry : stack_) {
            if (entry.type == ClipType::RoundedRect) {
                stencilRef++;

                // Write to stencil buffer where the rounded rect is
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                glStencilFunc(GL_ALWAYS, stencilRef, 0xFF);
                glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

                // Render the rounded rect into the stencil buffer
                // A full implementation would tessellate the rounded rect here
                // For now, use the bounding rect as the stencil shape
                // (the actual rounded-rect tessellation would call through path system)
                float x = entry.rect.x, y = entry.rect.y;
                float w = entry.rect.width, h = entry.rect.height;

                glBegin(GL_QUADS);
                glVertex2f(x, y);
                glVertex2f(x + w, y);
                glVertex2f(x + w, y + h);
                glVertex2f(x, y + h);
                glEnd();

                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            }
        }

        // Now set stencil test to only pass where all clips wrote
        if (stencilRef > 0) {
            glStencilFunc(GL_LEQUAL, stencilRef, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        }
    }
}

void ClipStack::restoreGL() const {
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
}

} // namespace NXRender
