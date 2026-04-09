// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "widgets/container.h"
#include "nxgfx/context.h"
#include <algorithm>
#include <cmath>

namespace NXRender {

Container::Container() {}
Container::~Container() = default;

// ==================================================================
// Rendering
// ==================================================================

void Container::render(GpuContext* ctx) {
    if (!isVisible() || layoutMode_ == LayoutMode::None) return;

    // Opacity layer
    bool needsOpacityLayer = (opacity_ < 1.0f && opacity_ > 0.0f);
    if (needsOpacityLayer) {
        ctx->pushTransform();
        // True opacity compositing would use an off-screen buffer.
        // Approximate via alpha modulation on background/border.
    }

    // Background
    if (backgroundColor_.a > 0) {
        Color bg = backgroundColor_;
        if (needsOpacityLayer) {
            bg = Color(bg.r, bg.g, bg.b,
                       static_cast<uint8_t>(bg.a * opacity_));
        }

        if (borderRadius_ > 0) {
            ctx->fillRoundedRect(bounds_, bg, borderRadius_);
        } else {
            ctx->fillRect(bounds_, bg);
        }
    }

    // Background gradient
    if (gradientEnabled_) {
        if (borderRadius_ > 0) {
            // Gradient with rounded corners: fill rect then clip approximation
            ctx->pushClip(bounds_);
            ctx->fillRectGradient(bounds_, gradientStart_, gradientEnd_, gradientHorizontal_);
            ctx->popClip();
        } else {
            ctx->fillRectGradient(bounds_, gradientStart_, gradientEnd_, gradientHorizontal_);
        }
    }

    // Shadow
    if (shadowBlur_ > 0 && shadowColor_.a > 0) {
        ctx->drawShadow(bounds_, shadowColor_, shadowBlur_, shadowOffsetX_, shadowOffsetY_);
    }

    // Border
    if (borderWidth_ > 0 && borderColor_.a > 0) {
        Color bc = borderColor_;
        if (needsOpacityLayer) {
            bc = Color(bc.r, bc.g, bc.b,
                       static_cast<uint8_t>(bc.a * opacity_));
        }

        if (borderRadius_ > 0) {
            ctx->strokeRoundedRect(bounds_, bc, borderRadius_, borderWidth_);
        } else {
            ctx->strokeRect(bounds_, bc, borderWidth_);
        }
    }

    // Render children (with optional clipping for overflow)
    if (clipChildren_) {
        Rect clipRect = bounds_;
        if (borderWidth_ > 0) {
            clipRect.x += borderWidth_;
            clipRect.y += borderWidth_;
            clipRect.width -= borderWidth_ * 2;
            clipRect.height -= borderWidth_ * 2;
        }
        ctx->pushClip(clipRect);
    }

    // Scroll offset
    if (scrollOffsetX_ != 0 || scrollOffsetY_ != 0) {
        ctx->pushTransform();
        ctx->translate(-scrollOffsetX_, -scrollOffsetY_);
    }

    renderChildren(ctx);

    if (scrollOffsetX_ != 0 || scrollOffsetY_ != 0) {
        ctx->popTransform();
    }

    if (clipChildren_) {
        ctx->popClip();
    }

    // Debug: draw layout bounds
    if (debugBorders_) {
        ctx->strokeRect(bounds_, Color(255, 0, 0, 80), 1.0f);
        for (const auto& child : children_) {
            if (child->isVisible()) {
                ctx->strokeRect(child->bounds(), Color(0, 0, 255, 60), 0.5f);
            }
        }
    }

    if (needsOpacityLayer) {
        ctx->popTransform();
    }
}

// ==================================================================
// Measurement
// ==================================================================

Size Container::measure(const Size& available) {
    Size effectiveAvail = available;
    if (cssWidth_ > 0) effectiveAvail.width = cssWidth_;
    if (cssHeight_ > 0) effectiveAvail.height = cssHeight_;

    // Enforce min/max
    if (minWidth_ > 0) effectiveAvail.width = std::max(effectiveAvail.width, minWidth_);
    if (maxWidth_ > 0) effectiveAvail.width = std::min(effectiveAvail.width, maxWidth_);

    std::vector<Widget*> kids;
    for (auto& child : children_) {
        if (child->isVisible()) kids.push_back(child.get());
    }

    Size result;
    switch (layoutMode_) {
        case LayoutMode::Block:
            result = blockLayout_.measure(kids, effectiveAvail, padding_);
            break;
        case LayoutMode::Flex:
            result = flexLayout_.measure(kids, effectiveAvail);
            break;
        case LayoutMode::Inline:
            // Inline: approximate as block (full inline layout happens in layout())
            result = blockLayout_.measure(kids, effectiveAvail, padding_);
            break;
        case LayoutMode::None:
            return Size(0, 0);
    }

    // CSS constraints
    if (cssWidth_ > 0) result.width = cssWidth_;
    if (cssHeight_ > 0) result.height = cssHeight_;

    // Min/Max
    if (minWidth_ > 0) result.width = std::max(result.width, minWidth_);
    if (maxWidth_ > 0) result.width = std::min(result.width, maxWidth_);
    if (minHeight_ > 0) result.height = std::max(result.height, minHeight_);
    if (maxHeight_ > 0) result.height = std::min(result.height, maxHeight_);

    // Add border to outer size
    result.width += borderWidth_ * 2;
    result.height += borderWidth_ * 2;

    return result;
}

// ==================================================================
// Layout
// ==================================================================

void Container::layout() {
    if (layoutMode_ == LayoutMode::None) return;

    std::vector<Widget*> kids;
    for (auto& child : children_) {
        if (child->isVisible()) kids.push_back(child.get());
    }

    // Inset bounds by border
    Rect contentBounds = bounds_;
    if (borderWidth_ > 0) {
        contentBounds.x += borderWidth_;
        contentBounds.y += borderWidth_;
        contentBounds.width -= borderWidth_ * 2;
        contentBounds.height -= borderWidth_ * 2;
    }

    switch (layoutMode_) {
        case LayoutMode::Block:
            blockLayout_.layout(kids, contentBounds, padding_);
            break;
        case LayoutMode::Flex:
            flexLayout_.layout(kids, contentBounds);
            break;
        case LayoutMode::Inline:
            // Use block layout as fallback — full inline layout uses inline_layout.cpp
            blockLayout_.layout(kids, contentBounds, padding_);
            break;
        default:
            blockLayout_.layout(kids, contentBounds, padding_);
            break;
    }

    // Compute content size for scrolling
    float maxChildBottom = 0;
    float maxChildRight = 0;
    for (auto* kid : kids) {
        Rect kb = kid->bounds();
        maxChildBottom = std::max(maxChildBottom, kb.y + kb.height - contentBounds.y);
        maxChildRight = std::max(maxChildRight, kb.x + kb.width - contentBounds.x);
    }
    contentWidth_ = maxChildRight;
    contentHeight_ = maxChildBottom;

    // Recursive layout
    for (auto& child : children_) {
        child->layout();
    }
}

// ==================================================================
// Scrolling
// ==================================================================

void Container::scrollTo(float x, float y) {
    scrollOffsetX_ = std::max(0.0f, x);
    scrollOffsetY_ = std::max(0.0f, y);
    clampScroll();
}

void Container::scrollBy(float dx, float dy) {
    scrollOffsetX_ += dx;
    scrollOffsetY_ += dy;
    clampScroll();
}

void Container::clampScroll() {
    float contentW = bounds_.width - padding_.horizontal() - borderWidth_ * 2;
    float contentH = bounds_.height - padding_.vertical() - borderWidth_ * 2;

    float maxX = std::max(0.0f, contentWidth_ - contentW);
    float maxY = std::max(0.0f, contentHeight_ - contentH);

    scrollOffsetX_ = std::clamp(scrollOffsetX_, 0.0f, maxX);
    scrollOffsetY_ = std::clamp(scrollOffsetY_, 0.0f, maxY);
}

bool Container::canScrollVertically() const {
    float contentH = bounds_.height - padding_.vertical() - borderWidth_ * 2;
    return contentHeight_ > contentH;
}

bool Container::canScrollHorizontally() const {
    float contentW = bounds_.width - padding_.horizontal() - borderWidth_ * 2;
    return contentWidth_ > contentW;
}

float Container::scrollProgress() const {
    float contentH = bounds_.height - padding_.vertical() - borderWidth_ * 2;
    float maxY = contentHeight_ - contentH;
    if (maxY <= 0) return 0;
    return scrollOffsetY_ / maxY;
}

// ==================================================================
// Gradient
// ==================================================================

void Container::setGradient(const Color& start, const Color& end, bool horizontal) {
    gradientEnabled_ = true;
    gradientStart_ = start;
    gradientEnd_ = end;
    gradientHorizontal_ = horizontal;
}

void Container::clearGradient() {
    gradientEnabled_ = false;
}

// ==================================================================
// Shadow
// ==================================================================

void Container::setShadow(const Color& color, float blur, float offsetX, float offsetY) {
    shadowColor_ = color;
    shadowBlur_ = blur;
    shadowOffsetX_ = offsetX;
    shadowOffsetY_ = offsetY;
}

void Container::clearShadow() {
    shadowColor_ = Color::transparent();
    shadowBlur_ = 0;
    shadowOffsetX_ = 0;
    shadowOffsetY_ = 0;
}

} // namespace NXRender
