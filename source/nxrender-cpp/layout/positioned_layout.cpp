// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "layout/positioned_layout.h"
#include "widgets/widget.h"
#include <algorithm>
#include <cmath>

namespace NXRender {

// ==================================================================
// Core positioning
// ==================================================================

void PositionedLayout::apply(Widget* widget, const PositionedProps& props,
                             const Rect& container) {
    if (!widget || props.type == PositionType::Static) return;

    Rect bounds = widget->bounds();

    if (props.type == PositionType::Relative) {
        applyRelative(widget, props, bounds);
        return;
    }

    if (props.type == PositionType::Absolute || props.type == PositionType::Fixed) {
        applyAbsolute(widget, props, container, bounds);
        return;
    }

    if (props.type == PositionType::Sticky) {
        applySticky(widget, props, container, bounds, 0, 0);
        return;
    }
}

void PositionedLayout::applyRelative(Widget* widget, const PositionedProps& props,
                                      Rect& bounds) {
    if (props.offsets.hasTop)
        bounds.y += props.offsets.top;
    else if (props.offsets.hasBottom)
        bounds.y -= props.offsets.bottom;

    if (props.offsets.hasLeft)
        bounds.x += props.offsets.left;
    else if (props.offsets.hasRight)
        bounds.x -= props.offsets.right;

    widget->setBounds(bounds);
}

void PositionedLayout::applyAbsolute(Widget* widget, const PositionedProps& props,
                                      const Rect& container, Rect& bounds) {
    // Measure child if needed
    Size naturalSize = widget->measure(Size(container.width, container.height));

    // Horizontal resolution
    if (props.offsets.hasLeft && props.offsets.hasRight) {
        bounds.x = container.x + props.offsets.left;
        bounds.width = container.width - props.offsets.left - props.offsets.right;
    } else if (props.offsets.hasLeft) {
        bounds.x = container.x + props.offsets.left;
        if (bounds.width <= 0) bounds.width = naturalSize.width;
    } else if (props.offsets.hasRight) {
        if (bounds.width <= 0) bounds.width = naturalSize.width;
        bounds.x = container.x + container.width - bounds.width - props.offsets.right;
    } else {
        // No horizontal offset specified — use auto positioning
        bounds.x = container.x;
        if (bounds.width <= 0) bounds.width = naturalSize.width;
    }

    // Vertical resolution
    if (props.offsets.hasTop && props.offsets.hasBottom) {
        bounds.y = container.y + props.offsets.top;
        bounds.height = container.height - props.offsets.top - props.offsets.bottom;
    } else if (props.offsets.hasTop) {
        bounds.y = container.y + props.offsets.top;
        if (bounds.height <= 0) bounds.height = naturalSize.height;
    } else if (props.offsets.hasBottom) {
        if (bounds.height <= 0) bounds.height = naturalSize.height;
        bounds.y = container.y + container.height - bounds.height - props.offsets.bottom;
    } else {
        bounds.y = container.y;
        if (bounds.height <= 0) bounds.height = naturalSize.height;
    }

    // Clamp to minimum 0 dimensions
    bounds.width = std::max(0.0f, bounds.width);
    bounds.height = std::max(0.0f, bounds.height);

    widget->setBounds(bounds);
}

void PositionedLayout::applySticky(Widget* widget, const PositionedProps& props,
                                    const Rect& container, Rect& bounds,
                                    float scrollX, float scrollY) {
    // Sticky: behaves like relative until scroll crosses threshold,
    // then sticks to the containing block edge.

    float stickyX = bounds.x;
    float stickyY = bounds.y;

    if (props.offsets.hasTop) {
        float threshold = container.y + props.offsets.top + scrollY;
        stickyY = std::max(bounds.y, threshold);
        // Don't exceed container bottom
        float maxY = container.y + container.height - bounds.height;
        stickyY = std::min(stickyY, maxY);
    }
    if (props.offsets.hasBottom) {
        float threshold = container.y + container.height - bounds.height
                          - props.offsets.bottom + scrollY;
        stickyY = std::min(bounds.y, threshold);
        stickyY = std::max(stickyY, container.y);
    }
    if (props.offsets.hasLeft) {
        float threshold = container.x + props.offsets.left + scrollX;
        stickyX = std::max(bounds.x, threshold);
        float maxX = container.x + container.width - bounds.width;
        stickyX = std::min(stickyX, maxX);
    }
    if (props.offsets.hasRight) {
        float threshold = container.x + container.width - bounds.width
                          - props.offsets.right + scrollX;
        stickyX = std::min(bounds.x, threshold);
        stickyX = std::max(stickyX, container.x);
    }

    bounds.x = stickyX;
    bounds.y = stickyY;
    widget->setBounds(bounds);
}

// ==================================================================
// Containing block resolution
// ==================================================================

Rect PositionedLayout::findContainingBlock(Widget* widget, PositionType posType) {
    if (!widget) return Rect(0, 0, 0, 0);

    if (posType == PositionType::Fixed) {
        // Fixed: containing block is the viewport
        // Walk up to root and use its bounds
        Widget* root = widget;
        while (root->parent()) root = root->parent();
        return root->bounds();
    }

    // Absolute: containing block is nearest positioned ancestor
    // For now, use parent's bounds (full impl needs positioned ancestor lookup)
    Widget* parent = widget->parent();
    if (parent) return parent->bounds();
    return Rect(0, 0, 0, 0);
}

// ==================================================================
// Z-index sorting
// ==================================================================

void PositionedLayout::sortByZIndex(std::vector<Widget*>& widgets,
                                     const std::vector<PositionedProps>& props) {
    if (widgets.size() != props.size()) return;

    std::vector<size_t> indices(widgets.size());
    for (size_t i = 0; i < indices.size(); i++) indices[i] = i;

    std::stable_sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        int zA = props[a].zIndexAuto ? 0 : props[a].zIndex;
        int zB = props[b].zIndexAuto ? 0 : props[b].zIndex;
        return zA < zB;
    });

    std::vector<Widget*> sorted;
    sorted.reserve(widgets.size());
    for (size_t idx : indices) sorted.push_back(widgets[idx]);
    widgets = std::move(sorted);
}

// ==================================================================
// Centering helpers
// ==================================================================

void PositionedLayout::centerInContainer(Widget* widget, const Rect& container) {
    if (!widget) return;
    Rect bounds = widget->bounds();
    bounds.x = container.x + (container.width - bounds.width) / 2.0f;
    bounds.y = container.y + (container.height - bounds.height) / 2.0f;
    widget->setBounds(bounds);
}

void PositionedLayout::centerHorizontally(Widget* widget, const Rect& container) {
    if (!widget) return;
    Rect bounds = widget->bounds();
    bounds.x = container.x + (container.width - bounds.width) / 2.0f;
    widget->setBounds(bounds);
}

void PositionedLayout::centerVertically(Widget* widget, const Rect& container) {
    if (!widget) return;
    Rect bounds = widget->bounds();
    bounds.y = container.y + (container.height - bounds.height) / 2.0f;
    widget->setBounds(bounds);
}

} // namespace NXRender
