// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "core/display_list.h"
#include "nxgfx/context.h"
#include <algorithm>
#include <cmath>

namespace NXRender {

DisplayList::DisplayList() {
    entries_.reserve(256);
}

DisplayList::~DisplayList() {
    clear();
}

DisplayList::DisplayList(DisplayList&& other) noexcept
    : entries_(std::move(other.entries_))
    , bounds_(other.bounds_)
    , ownedPaths_(std::move(other.ownedPaths_))
    , ownedComplexPaths_(std::move(other.ownedComplexPaths_)) {}

DisplayList& DisplayList::operator=(DisplayList&& other) noexcept {
    if (this != &other) {
        clear();
        entries_ = std::move(other.entries_);
        bounds_ = other.bounds_;
        ownedPaths_ = std::move(other.ownedPaths_);
        ownedComplexPaths_ = std::move(other.ownedComplexPaths_);
    }
    return *this;
}

DisplayListEntry& DisplayList::append(DrawCommand cmd) {
    entries_.emplace_back();
    auto& entry = entries_.back();
    entry.command = cmd;
    return entry;
}

void DisplayList::expandBounds(const Rect& rect) {
    if (entries_.size() == 1) {
        bounds_ = rect;
        return;
    }
    float minX = std::min(bounds_.x, rect.x);
    float minY = std::min(bounds_.y, rect.y);
    float maxX = std::max(bounds_.x + bounds_.width, rect.x + rect.width);
    float maxY = std::max(bounds_.y + bounds_.height, rect.y + rect.height);
    bounds_ = Rect(minX, minY, maxX - minX, maxY - minY);
}

void DisplayList::clear() {
    entries_.clear();
    for (auto* p : ownedPaths_) delete p;
    ownedPaths_.clear();
    for (auto* p : ownedComplexPaths_) delete p;
    ownedComplexPaths_.clear();
    bounds_ = Rect();
}

// ======================================================================
// Recording
// ======================================================================

void DisplayList::fillRect(const Rect& rect, const Color& color) {
    auto& e = append(DrawCommand::FillRect);
    e.rect = rect;
    e.color = color;
    expandBounds(rect);
}

void DisplayList::strokeRect(const Rect& rect, const Color& color, float lineWidth) {
    auto& e = append(DrawCommand::StrokeRect);
    e.rect = rect;
    e.color = color;
    e.lineWidth = lineWidth;
    Rect expanded(rect.x - lineWidth, rect.y - lineWidth,
                  rect.width + lineWidth * 2, rect.height + lineWidth * 2);
    expandBounds(expanded);
}

void DisplayList::fillRoundedRect(const Rect& rect, const Color& color, float radius) {
    auto& e = append(DrawCommand::FillRoundedRect);
    e.rect = rect;
    e.color = color;
    e.radius = radius;
    expandBounds(rect);
}

void DisplayList::fillRoundedRect(const Rect& rect, const Color& color, const CornerRadii& radii) {
    auto& e = append(DrawCommand::FillRoundedRect);
    e.rect = rect;
    e.color = color;
    e.cornerRadii = radii;
    e.radius = -1.0f; // Signal to use cornerRadii
    expandBounds(rect);
}

void DisplayList::strokeRoundedRect(const Rect& rect, const Color& color, float radius, float lineWidth) {
    auto& e = append(DrawCommand::StrokeRoundedRect);
    e.rect = rect;
    e.color = color;
    e.radius = radius;
    e.lineWidth = lineWidth;
    expandBounds(rect);
}

void DisplayList::fillCircle(float cx, float cy, float radius, const Color& color) {
    auto& e = append(DrawCommand::FillCircle);
    e.x1 = cx;
    e.y1 = cy;
    e.radius = radius;
    e.color = color;
    expandBounds(Rect(cx - radius, cy - radius, radius * 2, radius * 2));
}

void DisplayList::strokeCircle(float cx, float cy, float radius, const Color& color, float lineWidth) {
    auto& e = append(DrawCommand::StrokeCircle);
    e.x1 = cx;
    e.y1 = cy;
    e.radius = radius;
    e.color = color;
    e.lineWidth = lineWidth;
    float outer = radius + lineWidth;
    expandBounds(Rect(cx - outer, cy - outer, outer * 2, outer * 2));
}

void DisplayList::drawLine(float x1, float y1, float x2, float y2, const Color& color, float lineWidth) {
    auto& e = append(DrawCommand::DrawLine);
    e.x1 = x1;
    e.y1 = y1;
    e.x2 = x2;
    e.y2 = y2;
    e.color = color;
    e.lineWidth = lineWidth;
    float minX = std::min(x1, x2) - lineWidth;
    float minY = std::min(y1, y2) - lineWidth;
    float maxX = std::max(x1, x2) + lineWidth;
    float maxY = std::max(y1, y2) + lineWidth;
    expandBounds(Rect(minX, minY, maxX - minX, maxY - minY));
}

void DisplayList::drawText(const std::string& text, float x, float y, const Color& color, float fontSize) {
    auto& e = append(DrawCommand::DrawText);
    e.text = text;
    e.x1 = x;
    e.y1 = y;
    e.color = color;
    e.fontSize = fontSize;
    // Approximate text bounds (actual measurement requires font metrics)
    float approxWidth = static_cast<float>(text.size()) * fontSize * 0.6f;
    expandBounds(Rect(x, y, approxWidth, fontSize));
}

void DisplayList::drawTexture(uint32_t texture, const Rect& dest) {
    auto& e = append(DrawCommand::DrawTexture);
    e.textureId = texture;
    e.rect = dest;
    expandBounds(dest);
}

void DisplayList::drawTextureRegion(uint32_t texture, const Rect& src, const Rect& dest) {
    auto& e = append(DrawCommand::DrawTextureRegion);
    e.textureId = texture;
    e.srcRect = src;
    e.rect = dest;
    expandBounds(dest);
}

void DisplayList::fillRectGradient(const Rect& rect, const Color& start, const Color& end, bool horizontal) {
    auto& e = append(DrawCommand::FillRectGradient);
    e.rect = rect;
    e.color = start;
    e.endColor = end;
    e.horizontal = horizontal;
    expandBounds(rect);
}

void DisplayList::drawShadow(const Rect& rect, const Color& color, float blur, float offsetX, float offsetY) {
    auto& e = append(DrawCommand::DrawShadow);
    e.rect = rect;
    e.color = color;
    e.blur = blur;
    e.offsetX = offsetX;
    e.offsetY = offsetY;
    Rect expanded(rect.x + offsetX - blur * 2, rect.y + offsetY - blur * 2,
                  rect.width + blur * 4, rect.height + blur * 4);
    expandBounds(expanded);
}

void DisplayList::fillPath(const std::vector<Point>& points, const Color& color) {
    auto& e = append(DrawCommand::FillPath);
    e.color = color;
    auto* pd = new PathData();
    pd->points = points;
    pd->closed = true;
    e.pathData = pd;
    ownedPaths_.push_back(pd);

    // Compute path bounds
    if (!points.empty()) {
        float minX = points[0].x, maxX = points[0].x;
        float minY = points[0].y, maxY = points[0].y;
        for (size_t i = 1; i < points.size(); i++) {
            minX = std::min(minX, points[i].x);
            maxX = std::max(maxX, points[i].x);
            minY = std::min(minY, points[i].y);
            maxY = std::max(maxY, points[i].y);
        }
        expandBounds(Rect(minX, minY, maxX - minX, maxY - minY));
    }
}

void DisplayList::strokePath(const std::vector<Point>& points, const Color& color, float lineWidth, bool closed) {
    auto& e = append(DrawCommand::StrokePath);
    e.color = color;
    e.lineWidth = lineWidth;
    auto* pd = new PathData();
    pd->points = points;
    pd->closed = closed;
    e.pathData = pd;
    ownedPaths_.push_back(pd);

    if (!points.empty()) {
        float minX = points[0].x, maxX = points[0].x;
        float minY = points[0].y, maxY = points[0].y;
        for (size_t i = 1; i < points.size(); i++) {
            minX = std::min(minX, points[i].x);
            maxX = std::max(maxX, points[i].x);
            minY = std::min(minY, points[i].y);
            maxY = std::max(maxY, points[i].y);
        }
        expandBounds(Rect(minX - lineWidth, minY - lineWidth,
                          maxX - minX + lineWidth * 2, maxY - minY + lineWidth * 2));
    }
}

void DisplayList::fillComplexPath(const std::vector<std::vector<Point>>& contours,
                                   const Color& color, const std::string& rule) {
    auto& e = append(DrawCommand::FillComplexPath);
    e.color = color;
    auto* cpd = new ComplexPathData();
    cpd->contours = contours;
    cpd->fillRule = rule;
    e.complexPathData = cpd;
    ownedComplexPaths_.push_back(cpd);

    float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
    for (const auto& contour : contours) {
        for (const auto& pt : contour) {
            minX = std::min(minX, pt.x);
            maxX = std::max(maxX, pt.x);
            minY = std::min(minY, pt.y);
            maxY = std::max(maxY, pt.y);
        }
    }
    if (minX < 1e9f) {
        expandBounds(Rect(minX, minY, maxX - minX, maxY - minY));
    }
}

void DisplayList::pushClip(const Rect& rect) {
    auto& e = append(DrawCommand::PushClip);
    e.rect = rect;
}

void DisplayList::popClip() {
    append(DrawCommand::PopClip);
}

void DisplayList::pushTransform() {
    append(DrawCommand::PushTransform);
}

void DisplayList::popTransform() {
    append(DrawCommand::PopTransform);
}

void DisplayList::translate(float x, float y) {
    auto& e = append(DrawCommand::Translate);
    e.tx = x;
    e.ty = y;
}

void DisplayList::scale(float sx, float sy) {
    auto& e = append(DrawCommand::Scale);
    e.sx = sx;
    e.sy = sy;
}

void DisplayList::rotate(float radians) {
    auto& e = append(DrawCommand::Rotate);
    e.angle = radians;
}

void DisplayList::setBlendMode(int mode) {
    auto& e = append(DrawCommand::SetBlendMode);
    e.blendMode = mode;
}

void DisplayList::clear(const Color& color) {
    auto& e = append(DrawCommand::Clear);
    e.color = color;
}

void DisplayList::setRenderTarget(uint32_t targetId) {
    auto& e = append(DrawCommand::SetRenderTarget);
    e.renderTargetId = targetId;
}

// ======================================================================
// Playback
// ======================================================================

void DisplayList::replay(GpuContext* ctx) const {
    if (!ctx) return;

    for (const auto& e : entries_) {
        switch (e.command) {
            case DrawCommand::FillRect:
                ctx->fillRect(e.rect, e.color);
                break;
            case DrawCommand::StrokeRect:
                ctx->strokeRect(e.rect, e.color, e.lineWidth);
                break;
            case DrawCommand::FillRoundedRect:
                if (e.radius < 0) {
                    ctx->fillRoundedRect(e.rect, e.color, e.cornerRadii);
                } else {
                    ctx->fillRoundedRect(e.rect, e.color, e.radius);
                }
                break;
            case DrawCommand::StrokeRoundedRect:
                ctx->strokeRoundedRect(e.rect, e.color, e.radius, e.lineWidth);
                break;
            case DrawCommand::FillCircle:
                ctx->fillCircle(e.x1, e.y1, e.radius, e.color);
                break;
            case DrawCommand::StrokeCircle:
                ctx->strokeCircle(e.x1, e.y1, e.radius, e.color, e.lineWidth);
                break;
            case DrawCommand::DrawLine:
                ctx->drawLine(e.x1, e.y1, e.x2, e.y2, e.color, e.lineWidth);
                break;
            case DrawCommand::DrawText:
                ctx->drawText(e.text, e.x1, e.y1, e.color, e.fontSize);
                break;
            case DrawCommand::DrawTexture:
                ctx->drawTexture(e.textureId, e.rect);
                break;
            case DrawCommand::DrawTextureRegion:
                ctx->drawTexture(e.textureId, e.srcRect, e.rect);
                break;
            case DrawCommand::FillRectGradient:
                ctx->fillRectGradient(e.rect, e.color, e.endColor, e.horizontal);
                break;
            case DrawCommand::DrawShadow:
                ctx->drawShadow(e.rect, e.color, e.blur, e.offsetX, e.offsetY);
                break;
            case DrawCommand::FillPath:
                if (e.pathData) ctx->fillPath(e.pathData->points, e.color);
                break;
            case DrawCommand::StrokePath:
                if (e.pathData) ctx->strokePath(e.pathData->points, e.color, e.lineWidth, e.pathData->closed);
                break;
            case DrawCommand::FillComplexPath:
                if (e.complexPathData) ctx->fillComplexPath(e.complexPathData->contours, e.color, e.complexPathData->fillRule);
                break;
            case DrawCommand::PushClip:
                ctx->pushClip(e.rect);
                break;
            case DrawCommand::PopClip:
                ctx->popClip();
                break;
            case DrawCommand::PushTransform:
                ctx->pushTransform();
                break;
            case DrawCommand::PopTransform:
                ctx->popTransform();
                break;
            case DrawCommand::Translate:
                ctx->translate(e.tx, e.ty);
                break;
            case DrawCommand::Scale:
                ctx->scale(e.sx, e.sy);
                break;
            case DrawCommand::Rotate:
                ctx->rotate(e.angle);
                break;
            case DrawCommand::SetBlendMode:
                ctx->setBlendMode(static_cast<BlendMode>(e.blendMode));
                break;
            case DrawCommand::Clear:
                ctx->clear(e.color);
                break;
            case DrawCommand::SetRenderTarget:
                ctx->setRenderTarget(e.renderTargetId);
                break;
            case DrawCommand::Nop:
                break;
        }
    }
}

} // namespace NXRender
