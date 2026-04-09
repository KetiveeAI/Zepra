// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "layout/multi_column.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace NXRender {

// ==================================================================
// Column resolution (CSS Multi-column §3)
// ==================================================================

ColumnLayoutResult MultiColumnLayout::resolveColumns(float availableWidth,
                                                       const ColumnConfig& config) {
    ColumnLayoutResult result;

    float gap = config.columnGap;

    if (config.columnCount > 0 && config.columnWidth > 0) {
        // Both specified: use min of the two
        int byCount = config.columnCount;
        int byWidth = std::max(1, static_cast<int>(
            std::floor((availableWidth + gap) / (config.columnWidth + gap))
        ));
        result.actualColumnCount = std::min(byCount, byWidth);
    } else if (config.columnCount > 0) {
        result.actualColumnCount = config.columnCount;
    } else if (config.columnWidth > 0) {
        result.actualColumnCount = std::max(1, static_cast<int>(
            std::floor((availableWidth + gap) / (config.columnWidth + gap))
        ));
    } else {
        result.actualColumnCount = 1;
    }

    // Actual column width
    float totalGaps = gap * (result.actualColumnCount - 1);
    result.actualColumnWidth = (availableWidth - totalGaps) / result.actualColumnCount;
    result.actualColumnWidth = std::max(0.0f, result.actualColumnWidth);

    // Column rects
    float x = 0;
    for (int i = 0; i < result.actualColumnCount; i++) {
        ColumnLayoutResult::ColumnRect cr;
        cr.x = x;
        cr.y = 0;
        cr.width = result.actualColumnWidth;
        cr.height = 0; // Filled during layout
        result.columns.push_back(cr);
        x += result.actualColumnWidth + gap;
    }

    return result;
}

// ==================================================================
// Multi-column layout
// ==================================================================

void MultiColumnLayout::layout(std::vector<Widget*>& children,
                                const Rect& container,
                                const EdgeInsets& padding,
                                const ColumnConfig& config) {
    config_ = config;

    float contentWidth = container.width - padding.left - padding.right;
    result_ = resolveColumns(contentWidth, config);

    float contentX = container.x + padding.left;
    float contentY = container.y + padding.top;

    // Update column positions relative to container
    for (auto& col : result_.columns) {
        col.x += contentX;
        col.y = contentY;
    }

    if (config.balanceColumns) {
        distributeBalanced(children, container, padding, result_);
    } else {
        distributeFillAvailable(children, container, padding, result_);
    }
}

Size MultiColumnLayout::measure(std::vector<Widget*>& children,
                                 const Size& available,
                                 const EdgeInsets& padding,
                                 const ColumnConfig& config) {
    float contentWidth = available.width - padding.left - padding.right;
    auto layout = resolveColumns(contentWidth, config);

    // Measure total content height
    float totalHeight = 0;
    for (Widget* child : children) {
        if (!child->isVisible()) continue;
        Size childSize = child->measure(Size(layout.actualColumnWidth, available.height));
        totalHeight += childSize.height + child->margin().top + child->margin().bottom;
    }

    // Divide across columns
    float columnHeight = totalHeight / layout.actualColumnCount;
    return Size(available.width,
                columnHeight + padding.top + padding.bottom);
}

// ==================================================================
// Balanced distribution
// ==================================================================

float MultiColumnLayout::findBalancedHeight(std::vector<Widget*>& children,
                                              float columnWidth,
                                              int columns,
                                              float maxHeight) {
    // Binary search for optimal column height
    float lo = 0, hi = maxHeight;
    float bestHeight = maxHeight;

    for (int iter = 0; iter < 20; iter++) {
        float mid = (lo + hi) / 2;
        float currentY = 0;
        int currentCol = 0;

        for (Widget* child : children) {
            if (!child->isVisible()) continue;
            Size childSize = child->measure(Size(columnWidth, mid));
            float childH = childSize.height + child->margin().top + child->margin().bottom;

            if (currentY + childH > mid && currentCol < columns - 1) {
                currentCol++;
                currentY = 0;
            }
            currentY += childH;
        }

        if (currentCol < columns) {
            bestHeight = mid;
            hi = mid;
        } else {
            lo = mid;
        }
    }

    return bestHeight;
}

void MultiColumnLayout::distributeBalanced(std::vector<Widget*>& children,
                                             const Rect& container,
                                             const EdgeInsets& padding,
                                             const ColumnLayoutResult& layout) {
    // Calculate total content height for binary search upper bound
    float totalHeight = 0;
    for (Widget* child : children) {
        if (!child->isVisible()) continue;
        Size s = child->measure(Size(layout.actualColumnWidth, container.height));
        totalHeight += s.height + child->margin().top + child->margin().bottom;
    }

    float targetHeight = findBalancedHeight(children, layout.actualColumnWidth,
                                             layout.actualColumnCount, totalHeight);

    // Now lay out with that height
    int col = 0;
    float yOffset = 0;
    float maxColHeight = 0;

    for (Widget* child : children) {
        if (!child->isVisible()) continue;

        Size childSize = child->measure(Size(layout.actualColumnWidth, targetHeight));
        EdgeInsets cm = child->margin();
        float childH = childSize.height + cm.top + cm.bottom;

        // Check column overflow
        if (yOffset + childH > targetHeight && col < layout.actualColumnCount - 1) {
            maxColHeight = std::max(maxColHeight, yOffset);
            col++;
            yOffset = 0;
        }

        if (col < static_cast<int>(result_.columns.size())) {
            float x = result_.columns[col].x + cm.left;
            float y = result_.columns[col].y + yOffset + cm.top;
            float w = layout.actualColumnWidth - cm.left - cm.right;

            child->setBounds(Rect(x, y, std::max(0.0f, w), childSize.height));
        }

        yOffset += childH;
    }

    maxColHeight = std::max(maxColHeight, yOffset);

    // Set column heights
    for (auto& c : result_.columns) {
        c.height = maxColHeight;
    }
    result_.totalHeight = maxColHeight;
}

void MultiColumnLayout::distributeFillAvailable(std::vector<Widget*>& children,
                                                  const Rect& container,
                                                  const EdgeInsets& padding,
                                                  const ColumnLayoutResult& layout) {
    float maxHeight = container.height - padding.top - padding.bottom;
    if (maxHeight <= 0) maxHeight = 1e6f;

    int col = 0;
    float yOffset = 0;
    float globalMaxHeight = 0;

    for (Widget* child : children) {
        if (!child->isVisible()) continue;

        Size childSize = child->measure(Size(layout.actualColumnWidth, maxHeight));
        EdgeInsets cm = child->margin();
        float childH = childSize.height + cm.top + cm.bottom;

        if (yOffset + childH > maxHeight && col < layout.actualColumnCount - 1) {
            globalMaxHeight = std::max(globalMaxHeight, yOffset);
            col++;
            yOffset = 0;
        }

        if (col < static_cast<int>(result_.columns.size())) {
            float x = result_.columns[col].x + cm.left;
            float y = result_.columns[col].y + yOffset + cm.top;
            float w = layout.actualColumnWidth - cm.left - cm.right;

            child->setBounds(Rect(x, y, std::max(0.0f, w), childSize.height));
        }

        yOffset += childH;
    }

    globalMaxHeight = std::max(globalMaxHeight, yOffset);
    for (auto& c : result_.columns) {
        c.height = globalMaxHeight;
    }
    result_.totalHeight = globalMaxHeight;
}

// ==================================================================
// Fragmentation helpers
// ==================================================================

bool MultiColumnLayout::canBreakBefore(Widget* /*child*/, const FragmentationConfig& frag) {
    return frag.breakBefore != 2; // Not avoid
}

bool MultiColumnLayout::canBreakAfter(Widget* /*child*/, const FragmentationConfig& frag) {
    return frag.breakAfter != 2;
}

bool MultiColumnLayout::canBreakInside(Widget* /*child*/, const FragmentationConfig& frag) {
    return frag.breakInside != 1; // Not avoid
}

// ==================================================================
// Container Query Evaluator
// ==================================================================

bool ContainerQueryEvaluator::evaluate(const ContainerQuery& query,
                                         float containerWidth,
                                         float containerHeight,
                                         ContainerType containerType) {
    // Check container type compatibility
    if (query.requiredType == ContainerType::Size && containerType != ContainerType::Size) {
        return false;
    }
    if (query.requiredType == ContainerType::InlineSize &&
        containerType == ContainerType::Normal) {
        return false;
    }

    if (query.conditions.empty()) return true;

    bool result = (query.logicalOp == ContainerQuery::LogicalOp::And);

    for (const auto& cond : query.conditions) {
        bool matches = evaluateCondition(cond, containerWidth, containerHeight);

        if (query.logicalOp == ContainerQuery::LogicalOp::And) {
            result = result && matches;
        } else if (query.logicalOp == ContainerQuery::LogicalOp::Or) {
            result = result || matches;
        } else if (query.logicalOp == ContainerQuery::LogicalOp::Not) {
            result = !matches;
        }
    }

    return result;
}

bool ContainerQueryEvaluator::evaluateCondition(
    const ContainerQuery::Condition& cond,
    float containerWidth,
    float containerHeight) {

    float actual = 0;
    switch (cond.feature) {
        case ContainerQuery::Condition::Feature::Width:
        case ContainerQuery::Condition::Feature::InlineSize:
            actual = containerWidth;
            break;
        case ContainerQuery::Condition::Feature::MinWidth:
            return containerWidth >= cond.value;
        case ContainerQuery::Condition::Feature::MaxWidth:
            return containerWidth <= cond.value;
        case ContainerQuery::Condition::Feature::Height:
        case ContainerQuery::Condition::Feature::BlockSize:
            actual = containerHeight;
            break;
        case ContainerQuery::Condition::Feature::MinHeight:
            return containerHeight >= cond.value;
        case ContainerQuery::Condition::Feature::MaxHeight:
            return containerHeight <= cond.value;
        case ContainerQuery::Condition::Feature::AspectRatio:
            actual = (containerHeight > 0) ? containerWidth / containerHeight : 0;
            break;
        case ContainerQuery::Condition::Feature::Orientation:
            if (cond.keyword == "portrait") return containerHeight >= containerWidth;
            if (cond.keyword == "landscape") return containerWidth > containerHeight;
            return false;
    }

    switch (cond.op) {
        case ContainerQuery::Condition::Op::Equal:
            return std::abs(actual - cond.value) < 0.5f;
        case ContainerQuery::Condition::Op::GreaterThan:
            return actual > cond.value;
        case ContainerQuery::Condition::Op::LessThan:
            return actual < cond.value;
        case ContainerQuery::Condition::Op::GreaterEqual:
            return actual >= cond.value;
        case ContainerQuery::Condition::Op::LessEqual:
            return actual <= cond.value;
    }

    return false;
}

ContainerQueryEvaluator::ContainerInfo
ContainerQueryEvaluator::findContainer(Widget* element,
                                         const std::string& /*name*/,
                                         ContainerType /*requiredType*/) {
    ContainerInfo info;
    // Walk up the widget tree to find the nearest container
    Widget* current = element ? element->parent() : nullptr;
    while (current) {
        // Check if current widget is a container
        // In production, we'd check the computed container-type property
        Rect bounds = current->bounds();
        if (bounds.width > 0 && bounds.height > 0) {
            info.container = current;
            info.width = bounds.width;
            info.height = bounds.height;
            return info;
        }
        current = current->parent();
    }
    return info;
}

// ==================================================================
// Aspect Ratio
// ==================================================================

float AspectRatioLayout::resolveWidth(float height, const AspectRatioConfig& config) {
    if (config.ratio <= 0) return 0;
    return height * config.ratio;
}

float AspectRatioLayout::resolveHeight(float width, const AspectRatioConfig& config) {
    if (config.ratio <= 0) return 0;
    return width / config.ratio;
}

void AspectRatioLayout::applyAspectRatio(float& width, float& height,
                                           bool widthAuto, bool heightAuto,
                                           float minWidth, float maxWidth,
                                           float minHeight, float maxHeight,
                                           const AspectRatioConfig& config) {
    if (config.ratio <= 0) return;

    if (widthAuto && heightAuto) {
        // Both auto — prefer transferring from inline dimension
        // Don't apply (let intrinsic size or containing block decide)
        return;
    }

    if (widthAuto && !heightAuto) {
        width = height * config.ratio;
    } else if (!widthAuto && heightAuto) {
        height = width / config.ratio;
    }

    // Apply min/max constraints
    width = std::clamp(width, minWidth, maxWidth);
    height = std::clamp(height, minHeight, maxHeight);

    // Re-check ratio after clamping
    if (widthAuto) {
        width = height * config.ratio;
        width = std::clamp(width, minWidth, maxWidth);
    } else if (heightAuto) {
        height = width / config.ratio;
        height = std::clamp(height, minHeight, maxHeight);
    }
}

AspectRatioConfig AspectRatioLayout::parse(const std::string& value) {
    AspectRatioConfig config;
    if (value.empty() || value == "auto") return config;

    std::string s = value;
    // Check for "auto 16/9" pattern
    if (s.substr(0, 5) == "auto ") {
        config.preferIntrinsic = true;
        s = s.substr(5);
    } else {
        config.preferIntrinsic = false;
    }

    // Parse ratio "16/9" or single number "1.778"
    size_t slash = s.find('/');
    if (slash != std::string::npos) {
        float w = std::strtof(s.c_str(), nullptr);
        float h = std::strtof(s.c_str() + slash + 1, nullptr);
        if (h > 0) config.ratio = w / h;
    } else {
        config.ratio = std::strtof(s.c_str(), nullptr);
    }

    return config;
}

// ==================================================================
// Logical Property Mapper
// ==================================================================

EdgeInsets LogicalPropertyMapper::toPhysical(const LogicalEdges& logical,
                                               WritingDirection writing,
                                               TextDirection direction) {
    EdgeInsets physical;

    switch (writing) {
        case WritingDirection::HorizontalTB:
            physical.top = logical.blockStart;
            physical.bottom = logical.blockEnd;
            if (direction == TextDirection::LTR) {
                physical.left = logical.inlineStart;
                physical.right = logical.inlineEnd;
            } else {
                physical.right = logical.inlineStart;
                physical.left = logical.inlineEnd;
            }
            break;

        case WritingDirection::VerticalRL:
            physical.right = logical.blockStart;
            physical.left = logical.blockEnd;
            if (direction == TextDirection::LTR) {
                physical.top = logical.inlineStart;
                physical.bottom = logical.inlineEnd;
            } else {
                physical.bottom = logical.inlineStart;
                physical.top = logical.inlineEnd;
            }
            break;

        case WritingDirection::VerticalLR:
            physical.left = logical.blockStart;
            physical.right = logical.blockEnd;
            if (direction == TextDirection::LTR) {
                physical.top = logical.inlineStart;
                physical.bottom = logical.inlineEnd;
            } else {
                physical.bottom = logical.inlineStart;
                physical.top = logical.inlineEnd;
            }
            break;
    }

    return physical;
}

LogicalEdges LogicalPropertyMapper::toLogical(const EdgeInsets& physical,
                                                WritingDirection writing,
                                                TextDirection direction) {
    LogicalEdges logical;

    switch (writing) {
        case WritingDirection::HorizontalTB:
            logical.blockStart = physical.top;
            logical.blockEnd = physical.bottom;
            if (direction == TextDirection::LTR) {
                logical.inlineStart = physical.left;
                logical.inlineEnd = physical.right;
            } else {
                logical.inlineStart = physical.right;
                logical.inlineEnd = physical.left;
            }
            break;

        case WritingDirection::VerticalRL:
            logical.blockStart = physical.right;
            logical.blockEnd = physical.left;
            if (direction == TextDirection::LTR) {
                logical.inlineStart = physical.top;
                logical.inlineEnd = physical.bottom;
            } else {
                logical.inlineStart = physical.bottom;
                logical.inlineEnd = physical.top;
            }
            break;

        case WritingDirection::VerticalLR:
            logical.blockStart = physical.left;
            logical.blockEnd = physical.right;
            if (direction == TextDirection::LTR) {
                logical.inlineStart = physical.top;
                logical.inlineEnd = physical.bottom;
            } else {
                logical.inlineStart = physical.bottom;
                logical.inlineEnd = physical.top;
            }
            break;
    }

    return logical;
}

void LogicalPropertyMapper::resolveLogicalSizes(float inlineSize, float blockSize,
                                                  float& width, float& height,
                                                  WritingDirection writing) {
    switch (writing) {
        case WritingDirection::HorizontalTB:
            width = inlineSize;
            height = blockSize;
            break;
        case WritingDirection::VerticalRL:
        case WritingDirection::VerticalLR:
            width = blockSize;
            height = inlineSize;
            break;
    }
}

void LogicalPropertyMapper::resolveLogicalPosition(float insetBlockStart, float insetBlockEnd,
                                                     float insetInlineStart, float insetInlineEnd,
                                                     float& top, float& right,
                                                     float& bottom, float& left,
                                                     WritingDirection writing,
                                                     TextDirection direction) {
    LogicalEdges logical;
    logical.blockStart = insetBlockStart;
    logical.blockEnd = insetBlockEnd;
    logical.inlineStart = insetInlineStart;
    logical.inlineEnd = insetInlineEnd;

    EdgeInsets physical = toPhysical(logical, writing, direction);
    top = physical.top;
    right = physical.right;
    bottom = physical.bottom;
    left = physical.left;
}

} // namespace NXRender
