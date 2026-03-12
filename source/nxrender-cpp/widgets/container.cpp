// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file container.cpp
 * @brief Container widget implementation
 */

#include "widgets/container.h"
#include "nxgfx/context.h"

namespace NXRender {

Container::Container() {}
Container::~Container() = default;

void Container::render(GpuContext* ctx) {
    if (!isVisible()) return;
    
    // Draw background
    if (backgroundColor_.a > 0) {
        ctx->fillRect(bounds_, backgroundColor_);
    }
    
    // Render children
    renderChildren(ctx);
}

Size Container::measure(const Size& available) {
    std::vector<Widget*> kids;
    for (auto& child : children_) {
        kids.push_back(child.get());
    }
    return layout_.measure(kids, available);
}

void Container::layout() {
    std::vector<Widget*> kids;
    for (auto& child : children_) {
        kids.push_back(child.get());
    }
    layout_.layout(kids, bounds_);
    
    // Layout children recursively
    for (auto& child : children_) {
        child->layout();
    }
}

} // namespace NXRender
