// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file block_layout.cpp
 * @brief Block formatting context layout implementation
 */

#include "layout/block_layout.h"
#include "widgets/widget.h"
#include <algorithm>

namespace NXRender {

float BlockLayout::collapseMargin(float marginA, float marginB) {
    // CSS margin collapse: if both positive, take max; if one negative, sum;
    // if both negative, take more negative.
    if (marginA >= 0 && marginB >= 0) return std::max(marginA, marginB);
    if (marginA < 0 && marginB < 0) return std::min(marginA, marginB);
    return marginA + marginB;
}

void BlockLayout::layout(std::vector<Widget*>& children, const Rect& container,
                         const EdgeInsets& padding) {
    if (children.empty()) return;

    float contentX = container.x + padding.left;
    float contentY = container.y + padding.top;
    float contentWidth = container.width - padding.left - padding.right;
    float yOffset = 0;
    float prevMarginBottom = 0;

    for (size_t i = 0; i < children.size(); i++) {
        Widget* child = children[i];
        if (!child->isVisible()) continue;

        EdgeInsets childMargin = child->margin();

        // Collapse top margin with previous bottom margin
        float effectiveMarginTop = (i == 0) 
            ? childMargin.top 
            : collapseMargin(prevMarginBottom, childMargin.top);

        if (i > 0) {
            // Remove the already-added prevMarginBottom, replace with collapsed
            yOffset -= prevMarginBottom;
            yOffset += effectiveMarginTop;
        } else {
            yOffset += childMargin.top;
        }

        // Measure child with available width minus horizontal margins
        float childAvailWidth = contentWidth - childMargin.left - childMargin.right;
        Size childSize = child->measure(Size(childAvailWidth, container.height));

        // Block-level: child typically fills available width. 
        // We assume widgets in a block layout want to expand horizontally.
        float childWidth = childAvailWidth;
        if (childSize.width > childAvailWidth) {
            // Only allow shrinking if constraint is violated, though normally blocks don't overflow
            childWidth = childAvailWidth; 
        }

        float childX = contentX + childMargin.left;
        float childY = contentY + yOffset;

        child->setBounds(Rect(childX, childY, childWidth, childSize.height));

        yOffset += childSize.height + childMargin.bottom;
        prevMarginBottom = childMargin.bottom;
    }
}

Size BlockLayout::measure(std::vector<Widget*>& children, const Size& available,
                          const EdgeInsets& padding) {
    if (children.empty()) return Size(padding.left + padding.right, 
                                       padding.top + padding.bottom);

    float contentWidth = available.width - padding.left - padding.right;
    float totalHeight = 0;
    float maxWidth = 0;
    float prevMarginBottom = 0;

    for (size_t i = 0; i < children.size(); i++) {
        Widget* child = children[i];
        if (!child->isVisible()) continue;

        EdgeInsets childMargin = child->margin();

        float effectiveMarginTop = (i == 0)
            ? childMargin.top
            : collapseMargin(prevMarginBottom, childMargin.top);

        if (i > 0) {
            totalHeight -= prevMarginBottom;
            totalHeight += effectiveMarginTop;
        } else {
            totalHeight += childMargin.top;
        }

        float childAvailWidth = contentWidth - childMargin.left - childMargin.right;
        Size childSize = child->measure(Size(childAvailWidth, available.height));

        totalHeight += childSize.height + childMargin.bottom;
        maxWidth = std::max(maxWidth, childSize.width + childMargin.left + childMargin.right);
        prevMarginBottom = childMargin.bottom;
    }

    return Size(
        std::min(maxWidth + padding.left + padding.right, available.width),
        totalHeight + padding.top + padding.bottom
    );
}

} // namespace NXRender
