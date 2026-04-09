// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file clip_stack.h
 * @brief Hierarchical clip region manager.
 *
 * Manages a stack of rectangular and rounded-rect clip regions.
 * Used by the widget tree to enforce ancestor clip bounds during paint.
 * Backed by GL scissor test for rect clips and stencil buffer for
 * rounded-rect / arbitrary path clips.
 */

#pragma once

#include "primitives.h"
#include <vector>
#include <cstdint>

namespace NXRender {

enum class ClipType : uint8_t {
    Rect,         // Axis-aligned rectangle (cheapest)
    RoundedRect,  // Rounded rectangle (uses stencil)
    Path          // Arbitrary path (uses stencil)
};

struct ClipEntry {
    ClipType type = ClipType::Rect;
    Rect rect;
    CornerRadii radii;
    Rect effectiveRect; // Intersection with parent clip
    int stencilRef = 0; // Stencil reference for this level
};

/**
 * @brief Manages nested clip regions.
 *
 * During widget painting:
 *   clipStack.push(widget.bounds());
 *   widget.paintContent(ctx);
 *   clipStack.pop();
 */
class ClipStack {
public:
    ClipStack();
    ~ClipStack();

    /**
     * @brief Push a rectangular clip region.
     * The effective clip is the intersection with the current top clip.
     */
    void pushRect(const Rect& rect);

    /**
     * @brief Push a rounded-rect clip region.
     */
    void pushRoundedRect(const Rect& rect, const CornerRadii& radii);

    /**
     * @brief Pop the most recent clip region.
     */
    void pop();

    /**
     * @brief Reset the clip stack (remove all clips).
     */
    void clear();

    /**
     * @brief Get the current effective clip rect.
     * This is the intersection of all stacked clips.
     */
    Rect currentClip() const;

    /**
     * @brief Test if a rect is fully clipped (invisible).
     */
    bool isClipped(const Rect& rect) const;

    /**
     * @brief Test if a rect is fully inside the current clip.
     */
    bool isFullyVisible(const Rect& rect) const;

    /**
     * @brief Apply the current clip state to GL.
     * Uses scissor for rect clips, stencil for rounded/path clips.
     */
    void applyToGL(int viewportHeight) const;

    /**
     * @brief Restore GL state (disable scissor/stencil).
     */
    void restoreGL() const;

    /**
     * @brief Get the stack depth.
     */
    size_t depth() const { return stack_.size(); }
    bool empty() const { return stack_.empty(); }

    /**
     * @brief Get the top clip entry.
     */
    const ClipEntry* top() const;

private:
    Rect intersectRects(const Rect& a, const Rect& b) const;
    void updateEffective();

    std::vector<ClipEntry> stack_;
};

} // namespace NXRender
