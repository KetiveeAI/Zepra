// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "layout/block_layout.h"
#include "widgets/widget.h"
#include <algorithm>
#include <cmath>

namespace NXRender {

// ==================================================================
// CSS margin collapse
// ==================================================================

float BlockLayout::collapseMargin(float marginA, float marginB) {
    if (marginA >= 0 && marginB >= 0) return std::max(marginA, marginB);
    if (marginA < 0 && marginB < 0) return std::min(marginA, marginB);
    return marginA + marginB;
}

// ==================================================================
// Core block layout pass
// ==================================================================

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
            yOffset -= prevMarginBottom;
            yOffset += effectiveMarginTop;
        } else {
            yOffset += childMargin.top;
        }

        // Available width minus horizontal margins
        float childAvailWidth = contentWidth - childMargin.left - childMargin.right;

        // Measure child
        Size childSize = child->measure(Size(childAvailWidth, container.height));

        // Block-level children fill available width by default
        float childWidth = childAvailWidth;

        // Auto-margin centering: if child is narrower and margins are "auto" 
        // (approximated by child being narrower than available)
        float childX = contentX + childMargin.left;

        // If child's natural width is less than available, check for auto-margin centering
        if (childSize.width < childAvailWidth && childSize.width > 0) {
            // Check if this child wants to be centered (cssWidth > 0 + auto margins)
            // We approximate auto margins by centering narrower children
            // Only if both margins are 0 (auto placeholder)
            if (childMargin.left == 0 && childMargin.right == 0) {
                // Default block: use full width
                childWidth = childAvailWidth;
            } else {
                childWidth = childSize.width;
            }
        }

        float childY = contentY + yOffset;

        // Min/max width constraints
        if (childSize.width > childAvailWidth) {
            childWidth = childAvailWidth;
        }

        // Ensure non-negative dimensions
        childWidth = std::max(0.0f, childWidth);
        float childHeight = std::max(0.0f, childSize.height);

        child->setBounds(Rect(childX, childY, childWidth, childHeight));

        yOffset += childHeight + childMargin.bottom;
        prevMarginBottom = childMargin.bottom;
    }
}

// ==================================================================
// Block layout measurement
// ==================================================================

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

// ==================================================================
// Intrinsic sizing (shrink-to-fit)
// ==================================================================

float BlockLayout::intrinsicMinWidth(std::vector<Widget*>& children,
                                      const EdgeInsets& padding) {
    float minWidth = 0;
    for (Widget* child : children) {
        if (!child->isVisible()) continue;
        EdgeInsets cm = child->margin();
        Size childMin = child->measure(Size(0, 0));
        minWidth = std::max(minWidth, childMin.width + cm.left + cm.right);
    }
    return minWidth + padding.left + padding.right;
}

float BlockLayout::intrinsicMaxWidth(std::vector<Widget*>& children,
                                      const EdgeInsets& padding) {
    float maxWidth = 0;
    for (Widget* child : children) {
        if (!child->isVisible()) continue;
        EdgeInsets cm = child->margin();
        Size childMax = child->measure(Size(1e6f, 1e6f));
        maxWidth = std::max(maxWidth, childMax.width + cm.left + cm.right);
    }
    return maxWidth + padding.left + padding.right;
}

// ==================================================================
// Center block child
// ==================================================================

void BlockLayout::centerChild(Widget* child, const Rect& container) {
    if (!child) return;
    Rect bounds = child->bounds();
    float excessWidth = container.width - bounds.width;
    if (excessWidth > 0) {
        bounds.x = container.x + excessWidth / 2.0f;
        child->setBounds(bounds);
    }
}

// ==================================================================
// Layout with float clearing
// ==================================================================

void BlockLayout::layoutWithClearance(std::vector<Widget*>& children,
                                       const Rect& container,
                                       const EdgeInsets& padding,
                                       float clearanceHeight) {
    // Standard layout first
    layout(children, container, padding);

    // If clearance specified, push the first child below floats
    if (clearanceHeight > 0 && !children.empty()) {
        for (Widget* child : children) {
            if (!child->isVisible()) continue;
            Rect bounds = child->bounds();
            if (bounds.y < container.y + padding.top + clearanceHeight) {
                float shift = (container.y + padding.top + clearanceHeight) - bounds.y;
                // Shift this and all subsequent children down
                for (Widget* c : children) {
                    if (!c->isVisible()) continue;
                    Rect cb = c->bounds();
                    cb.y += shift;
                    c->setBounds(cb);
                }
                break;
            }
        }
    }
}

} // namespace NXRender
