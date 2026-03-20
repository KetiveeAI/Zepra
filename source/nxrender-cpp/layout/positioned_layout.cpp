// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file positioned_layout.cpp
 * @brief CSS positioned element layout implementation
 */

#include "layout/positioned_layout.h"
#include "widgets/widget.h"
#include <algorithm>

namespace NXRender {

void PositionedLayout::apply(Widget* widget, const PositionedProps& props,
                             const Rect& container) {
    if (!widget || props.type == PositionType::Static) return;

    Rect bounds = widget->bounds();

    if (props.type == PositionType::Relative) {
        // Relative: offset from normal flow position
        if (props.offsets.hasTop)
            bounds.y += props.offsets.top;
        else if (props.offsets.hasBottom)
            bounds.y -= props.offsets.bottom;

        if (props.offsets.hasLeft)
            bounds.x += props.offsets.left;
        else if (props.offsets.hasRight)
            bounds.x -= props.offsets.right;

        widget->setBounds(bounds);
        return;
    }

    if (props.type == PositionType::Absolute || props.type == PositionType::Fixed) {
        // Absolute/Fixed: position relative to containing block
        // Resolve horizontal position
        if (props.offsets.hasLeft && props.offsets.hasRight) {
            // Both specified — stretch width
            bounds.x = container.x + props.offsets.left;
            bounds.width = container.width - props.offsets.left - props.offsets.right;
        } else if (props.offsets.hasLeft) {
            bounds.x = container.x + props.offsets.left;
        } else if (props.offsets.hasRight) {
            bounds.x = container.x + container.width - bounds.width - props.offsets.right;
        }

        // Resolve vertical position
        if (props.offsets.hasTop && props.offsets.hasBottom) {
            bounds.y = container.y + props.offsets.top;
            bounds.height = container.height - props.offsets.top - props.offsets.bottom;
        } else if (props.offsets.hasTop) {
            bounds.y = container.y + props.offsets.top;
        } else if (props.offsets.hasBottom) {
            bounds.y = container.y + container.height - bounds.height - props.offsets.bottom;
        }

        widget->setBounds(bounds);
        return;
    }

    if (props.type == PositionType::Sticky) {
        // Sticky: acts like relative until scroll threshold, then fixed
        // For now, treat as relative (full impl needs scroll position)
        if (props.offsets.hasTop)
            bounds.y = std::max(bounds.y, container.y + props.offsets.top);
        if (props.offsets.hasBottom)
            bounds.y = std::min(bounds.y, container.y + container.height - bounds.height - props.offsets.bottom);

        widget->setBounds(bounds);
    }
}

void PositionedLayout::sortByZIndex(std::vector<Widget*>& widgets,
                                     const std::vector<PositionedProps>& props) {
    if (widgets.size() != props.size()) return;

    // Build index pairs and sort by z-index
    std::vector<size_t> indices(widgets.size());
    for (size_t i = 0; i < indices.size(); i++) indices[i] = i;

    std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        int zA = props[a].zIndexAuto ? 0 : props[a].zIndex;
        int zB = props[b].zIndexAuto ? 0 : props[b].zIndex;
        return zA < zB;
    });

    // Reorder widgets in-place
    std::vector<Widget*> sorted;
    sorted.reserve(widgets.size());
    for (size_t idx : indices) sorted.push_back(widgets[idx]);
    widgets = std::move(sorted);
}

} // namespace NXRender
