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

struct FlexItem {
    Widget* widget;
    Size minSize;
    float mainSize;
    float crossSize;
    float flexGrow;
    float flexShrink;
    float flexBasis;
};

struct FlexLine {
    std::vector<FlexItem> items;
    float mainSize = 0;
    float crossSize = 0;
    float totalFlexGrow = 0;
    float totalFlexShrink = 0;
};

void FlexLayout::layout(std::vector<Widget*>& children, const Rect& container) {
    if (children.empty()) return;
    
    bool horizontal = isMainAxisHorizontal();
    float mainSize = horizontal ? container.width : container.height;
    float crossSize = horizontal ? container.height : container.width;
    float mainGapSize = mainGap();
    float crossGapSize = crossGap();
    
    std::vector<FlexLine> lines;
    lines.push_back(FlexLine());
    
    // Pass 1: Measure and wrap lines
    for (Widget* child : children) {
        if (!child->isVisible()) continue;
        
        const auto& props = child->flexProps();
        Size available = Size(horizontal ? (props.flexBasis >= 0 ? props.flexBasis : mainSize) : crossSize,
                              horizontal ? crossSize : (props.flexBasis >= 0 ? props.flexBasis : mainSize));
        Size s = child->measure(available);
        
        float childMain = horizontal ? s.width : s.height;
        float childCross = horizontal ? s.height : s.width;
        
        if (props.flexBasis >= 0) {
            childMain = props.flexBasis;
        }
        
        FlexItem item{child, s, childMain, childCross, props.flexGrow, props.flexShrink, props.flexBasis};
        FlexLine* currentLine = &lines.back();
        
        float gapRequired = currentLine->items.empty() ? 0 : mainGapSize;
        
        if (wrap != FlexWrap::NoWrap && !currentLine->items.empty() && 
            currentLine->mainSize + gapRequired + childMain > mainSize) {
            lines.push_back(FlexLine());
            currentLine = &lines.back();
            gapRequired = 0;
        }
        
        currentLine->items.push_back(item);
        currentLine->mainSize += gapRequired + childMain;
        currentLine->crossSize = std::max(currentLine->crossSize, childCross);
        currentLine->totalFlexGrow += props.flexGrow;
        currentLine->totalFlexShrink += props.flexShrink;
    }
    
    // Pass 2: Grow & Shrink, Align, Position
    float currentCrossPos = 0;
    
    for (size_t l = 0; l < lines.size(); l++) {
        FlexLine& line = lines[l];
        if (line.items.empty()) continue;
        
        float freeSpace = mainSize - line.mainSize;
        if (freeSpace > 0 && line.totalFlexGrow > 0) {
            for (auto& item : line.items) {
                float extra = freeSpace * (item.flexGrow / line.totalFlexGrow);
                item.mainSize += extra;
            }
            line.mainSize = mainSize;
        } else if (freeSpace < 0 && line.totalFlexShrink > 0) {
            float totalShrinkScaled = 0;
            for (const auto& item : line.items) {
                totalShrinkScaled += item.flexShrink * item.mainSize;
            }
            if (totalShrinkScaled > 0) {
                float targetShrink = -freeSpace;
                for (auto& item : line.items) {
                    float shrinkAmt = targetShrink * ((item.flexShrink * item.mainSize) / totalShrinkScaled);
                    item.mainSize -= shrinkAmt;
                    if (item.mainSize < 0) item.mainSize = 0;
                }
            }
            line.mainSize = mainSize;
        }
        
        float mainOffset = 0;
        float itemSpacing = mainGapSize;
        float actualFreeSpace = mainSize - line.mainSize;
        if (actualFreeSpace < 0) actualFreeSpace = 0;
        
        switch (justifyContent) {
            case JustifyContent::FlexStart: mainOffset = 0; break;
            case JustifyContent::FlexEnd: mainOffset = actualFreeSpace; break;
            case JustifyContent::Center: mainOffset = actualFreeSpace / 2; break;
            case JustifyContent::SpaceBetween:
                if (line.items.size() > 1) itemSpacing = actualFreeSpace / (line.items.size() - 1) + mainGapSize;
                break;
            case JustifyContent::SpaceAround:
                if (line.items.size() > 0) {
                    float space = actualFreeSpace / line.items.size();
                    mainOffset = space / 2;
                    itemSpacing = space + mainGapSize;
                }
                break;
            case JustifyContent::SpaceEvenly:
                if (line.items.size() > 0) {
                    float space = actualFreeSpace / (line.items.size() + 1);
                    mainOffset = space;
                    itemSpacing = space + mainGapSize;
                }
                break;
        }
        
        float pos = mainOffset;
        float lineCrossSize = line.crossSize;
        if (lines.size() == 1 && alignItems == AlignItems::Stretch) {
            lineCrossSize = crossSize;
        }
        
        for (auto& item : line.items) {
            float itemCrossOffset = 0;
            float finalChildCross = item.crossSize;
            
            AlignItems effectiveAlign = item.widget->flexProps().alignSelf;
            if (effectiveAlign == AlignItems::Auto) effectiveAlign = alignItems;
            
            switch (effectiveAlign) {
                case AlignItems::FlexStart: break;
                case AlignItems::FlexEnd: itemCrossOffset = lineCrossSize - item.crossSize; break;
                case AlignItems::Center: itemCrossOffset = (lineCrossSize - item.crossSize) / 2; break;
                case AlignItems::Stretch: 
                    finalChildCross = lineCrossSize; 
                    itemCrossOffset = 0;
                    break;
                default: break;
            }
            
            Rect bounds;
            if (horizontal) {
                bounds = Rect(container.x + pos, container.y + currentCrossPos + itemCrossOffset, item.mainSize, finalChildCross);
            } else {
                bounds = Rect(container.x + currentCrossPos + itemCrossOffset, container.y + pos, finalChildCross, item.mainSize);
            }
            
            if (direction == FlexDirection::RowReverse) {
                bounds.x = container.x + container.width - (pos - mainOffset) - bounds.width - mainOffset;
            } else if (direction == FlexDirection::ColumnReverse) {
                bounds.y = container.y + container.height - (pos - mainOffset) - bounds.height - mainOffset;
            }
            
            item.widget->setBounds(bounds);
            pos += item.mainSize + itemSpacing;
        }
        currentCrossPos += lineCrossSize + crossGapSize;
    }
}

Size FlexLayout::measure(std::vector<Widget*>& children, const Size& available) {
    if (children.empty()) return Size(0, 0);
    
    bool horizontal = isMainAxisHorizontal();
    float availableMain = horizontal ? available.width : available.height;
    float availableCross = horizontal ? available.height : available.width;
    
    float mainGapSize = mainGap();
    float crossGapSize = crossGap();
    
    std::vector<FlexLine> lines;
    lines.push_back(FlexLine());
    
    for (Widget* child : children) {
        if (!child->isVisible()) continue;
        
        Size s = child->measure(Size(availableMain, availableCross));
        const auto& props = child->flexProps();
        float childMain = horizontal ? s.width : s.height;
        float childCross = horizontal ? s.height : s.width;
        if (props.flexBasis >= 0) childMain = props.flexBasis;
        
        FlexItem item{child, s, childMain, childCross, props.flexGrow, props.flexShrink, props.flexBasis};
        FlexLine* currentLine = &lines.back();
        
        float gapRequired = currentLine->items.empty() ? 0 : mainGapSize;
        
        if (wrap != FlexWrap::NoWrap && !currentLine->items.empty() && 
            currentLine->mainSize + gapRequired + childMain > availableMain) {
            lines.push_back(FlexLine());
            currentLine = &lines.back();
            gapRequired = 0;
        }
        
        currentLine->items.push_back(item);
        currentLine->mainSize += gapRequired + childMain;
        currentLine->crossSize = std::max(currentLine->crossSize, childCross);
    }
    
    float maxLineMain = 0;
    float totalLinesCross = 0;
    int lineCount = 0;
    
    for (const auto& line : lines) {
        if (line.items.empty()) continue;
        maxLineMain = std::max(maxLineMain, line.mainSize);
        totalLinesCross += line.crossSize;
        lineCount++;
    }
    
    if (lineCount > 1) {
        totalLinesCross += crossGapSize * (lineCount - 1);
    }
    
    return horizontal ? Size(maxLineMain, totalLinesCross) : Size(totalLinesCross, maxLineMain);
}

} // namespace NXRender
