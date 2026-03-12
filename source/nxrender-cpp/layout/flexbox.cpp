// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file flexbox.cpp
 * @brief Flexbox layout implementation
 */

#include "layout/flexbox.h"
#include "widgets/widget.h"
#include <algorithm>
#include <numeric>

namespace NXRender {

FlexLayout::FlexLayout() 
    : direction(FlexDirection::Row)
    , justifyContent(JustifyContent::FlexStart)
    , alignItems(AlignItems::Stretch)
    , wrap(FlexWrap::NoWrap)
    , gap(0)
    , rowGap(0)
    , columnGap(0) {}

void FlexLayout::layout(std::vector<Widget*>& children, const Rect& container) {
    if (children.empty()) return;
    
    bool horizontal = isMainAxisHorizontal();
    float mainSize = horizontal ? container.width : container.height;
    float crossSize = horizontal ? container.height : container.width;
    float mainGapSize = mainGap();
    
    // Measure all children
    std::vector<Size> sizes;
    float totalMain = 0;
    float maxCross = 0;
    
    for (Widget* child : children) {
        Size s = child->measure(Size(mainSize, crossSize));
        sizes.push_back(s);
        float childMain = horizontal ? s.width : s.height;
        float childCross = horizontal ? s.height : s.width;
        totalMain += childMain;
        maxCross = std::max(maxCross, childCross);
    }
    
    // Add gaps
    totalMain += mainGapSize * (children.size() - 1);
    
    // Calculate starting position and spacing
    float freeSpace = mainSize - totalMain;
    float mainOffset = 0;
    float itemSpacing = mainGapSize;
    
    switch (justifyContent) {
        case JustifyContent::FlexStart:
            mainOffset = 0;
            break;
        case JustifyContent::FlexEnd:
            mainOffset = freeSpace;
            break;
        case JustifyContent::Center:
            mainOffset = freeSpace / 2;
            break;
        case JustifyContent::SpaceBetween:
            if (children.size() > 1) {
                itemSpacing = freeSpace / (children.size() - 1) + mainGapSize;
            }
            break;
        case JustifyContent::SpaceAround:
            if (children.size() > 0) {
                float space = freeSpace / children.size();
                mainOffset = space / 2;
                itemSpacing = space + mainGapSize;
            }
            break;
        case JustifyContent::SpaceEvenly:
            if (children.size() > 0) {
                float space = freeSpace / (children.size() + 1);
                mainOffset = space;
                itemSpacing = space + mainGapSize;
            }
            break;
    }
    
    // Position children
    float pos = mainOffset;
    for (size_t i = 0; i < children.size(); i++) {
        Widget* child = children[i];
        const Size& s = sizes[i];
        
        float childMain = horizontal ? s.width : s.height;
        float childCross = horizontal ? s.height : s.width;
        
        // Calculate cross-axis position
        float crossOffset = 0;
        switch (alignItems) {
            case AlignItems::FlexStart:
                crossOffset = 0;
                break;
            case AlignItems::FlexEnd:
                crossOffset = crossSize - childCross;
                break;
            case AlignItems::Center:
                crossOffset = (crossSize - childCross) / 2;
                break;
            case AlignItems::Stretch:
                childCross = crossSize;
                crossOffset = 0;
                break;
            default:
                break;
        }
        
        // Set bounds
        Rect bounds;
        if (horizontal) {
            bounds = Rect(container.x + pos, container.y + crossOffset, s.width, childCross);
        } else {
            bounds = Rect(container.x + crossOffset, container.y + pos, childCross, s.height);
        }
        
        // Handle reverse
        if (direction == FlexDirection::RowReverse) {
            bounds.x = container.x + container.width - pos - bounds.width;
        } else if (direction == FlexDirection::ColumnReverse) {
            bounds.y = container.y + container.height - pos - bounds.height;
        }
        
        child->setBounds(bounds);
        pos += childMain + itemSpacing;
    }
}

Size FlexLayout::measure(std::vector<Widget*>& children, const Size& available) {
    if (children.empty()) return Size(0, 0);
    
    bool horizontal = isMainAxisHorizontal();
    float totalMain = 0;
    float maxCross = 0;
    
    for (Widget* child : children) {
        Size s = child->measure(available);
        float childMain = horizontal ? s.width : s.height;
        float childCross = horizontal ? s.height : s.width;
        totalMain += childMain;
        maxCross = std::max(maxCross, childCross);
    }
    
    totalMain += mainGap() * (children.size() - 1);
    
    if (horizontal) {
        return Size(totalMain, maxCross);
    } else {
        return Size(maxCross, totalMain);
    }
}

} // namespace NXRender
