// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file container.cpp
 * @brief Container widget implementation with layout mode dispatch
 */

#include "widgets/container.h"
#include "nxgfx/context.h"

namespace NXRender {

Container::Container() {}
Container::~Container() = default;

void Container::render(GpuContext* ctx) {
    if (!isVisible() || layoutMode_ == LayoutMode::None) return;
    
    // Background
    if (backgroundColor_.a > 0) {
        ctx->fillRect(bounds_, backgroundColor_);
    }
    
    // Border
    if (borderWidth_ > 0 && borderColor_.a > 0) {
        ctx->strokeRect(bounds_, borderColor_, borderWidth_);
    }
    
    // Render children (with optional clipping for overflow)
    if (clipChildren_) {
        ctx->pushClip(bounds_);
    }
    
    renderChildren(ctx);
    
    if (clipChildren_) {
        ctx->popClip();
    }
}

Size Container::measure(const Size& available) {
    // Apply CSS constraints
    Size effectiveAvail = available;
    if (cssWidth_ > 0) effectiveAvail.width = cssWidth_;
    if (cssHeight_ > 0) effectiveAvail.height = cssHeight_;
    
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
        case LayoutMode::None:
            return Size(0, 0);
        default:
            result = blockLayout_.measure(kids, effectiveAvail, padding_);
            break;
    }
    
    // Enforce CSS dimensions
    if (cssWidth_ > 0) result.width = cssWidth_;
    if (cssHeight_ > 0) result.height = cssHeight_;
    
    return result;
}

void Container::layout() {
    if (layoutMode_ == LayoutMode::None) return;
    
    std::vector<Widget*> kids;
    for (auto& child : children_) {
        if (child->isVisible()) kids.push_back(child.get());
    }
    
    switch (layoutMode_) {
        case LayoutMode::Block:
            blockLayout_.layout(kids, bounds_, padding_);
            break;
        case LayoutMode::Flex:
            flexLayout_.layout(kids, bounds_);
            break;
        default:
            blockLayout_.layout(kids, bounds_, padding_);
            break;
    }
    
    // Layout children recursively
    for (auto& child : children_) {
        child->layout();
    }
}

} // namespace NXRender
