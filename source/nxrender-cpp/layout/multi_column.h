// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "widgets/widget.h"
#include <vector>
#include <string>

namespace NXRender {

// ==================================================================
// Multi-column layout engine
// CSS Multi-column Layout Module Level 1
// ==================================================================

struct ColumnLayoutResult {
    int actualColumnCount = 1;
    float actualColumnWidth = 0;
    float totalHeight = 0;
    
    struct ColumnRect {
        float x = 0, y = 0;
        float width = 0, height = 0;
    };
    std::vector<ColumnRect> columns;
};

struct ColumnConfig {
    int columnCount = 0;         // column-count (0 = auto)
    float columnWidth = 0;       // column-width (0 = auto)
    float columnGap = 16.0f;     // column-gap (default 1em ≈ 16px)
    float columnRuleWidth = 0;   // column-rule-width
    uint32_t columnRuleColor = 0;
    uint8_t columnRuleStyle = 0; // 0=none, 1=solid, 2=dashed, 3=dotted
    bool balanceColumns = true;  // column-fill: balance vs auto
};

struct FragmentationConfig {
    uint8_t breakBefore = 0;     // 0=auto, 1=always, 2=avoid, 3=column, 4=page
    uint8_t breakAfter = 0;
    uint8_t breakInside = 0;     // 0=auto, 1=avoid, 2=avoid-column, 3=avoid-page
    int orphans = 2;
    int widows = 2;
};

class MultiColumnLayout {
public:
    // Resolve column count and width from CSS properties
    static ColumnLayoutResult resolveColumns(float availableWidth,
                                              const ColumnConfig& config);

    // Layout children across columns
    void layout(std::vector<Widget*>& children,
                const Rect& container,
                const EdgeInsets& padding,
                const ColumnConfig& config);

    // Measure total height including all columns
    Size measure(std::vector<Widget*>& children,
                 const Size& available,
                 const EdgeInsets& padding,
                 const ColumnConfig& config);

    // Get column rects for rule painting
    const std::vector<ColumnLayoutResult::ColumnRect>& columnRects() const { return result_.columns; }
    const ColumnConfig& currentConfig() const { return config_; }
    const ColumnLayoutResult& result() const { return result_; }

private:
    ColumnLayoutResult result_;
    ColumnConfig config_;

    // Distribute content across columns with balanced height
    void distributeBalanced(std::vector<Widget*>& children,
                            const Rect& container,
                            const EdgeInsets& padding,
                            const ColumnLayoutResult& layout);

    // Distribute content fill-available (greedy)
    void distributeFillAvailable(std::vector<Widget*>& children,
                                  const Rect& container,
                                  const EdgeInsets& padding,
                                  const ColumnLayoutResult& layout);

    // Check if we can break before/after/inside an element
    bool canBreakBefore(Widget* child, const FragmentationConfig& frag);
    bool canBreakAfter(Widget* child, const FragmentationConfig& frag);
    bool canBreakInside(Widget* child, const FragmentationConfig& frag);

    // Find optimal column height (binary search)
    float findBalancedHeight(std::vector<Widget*>& children,
                             float columnWidth,
                             int columns,
                             float maxHeight);
};

// ==================================================================
// Container Queries
// CSS Containment Module Level 3
// ==================================================================

enum class ContainerType : uint8_t {
    Normal = 0,      // Not a container
    InlineSize = 1,   // Responds to inline-size queries
    Size = 2,         // Responds to size queries (inline + block)
};

struct ContainerQuery {
    std::string containerName;   // Optional: target specific container
    ContainerType requiredType = ContainerType::InlineSize;
    
    struct Condition {
        enum class Feature {
            MinWidth, MaxWidth,
            MinHeight, MaxHeight,
            Width, Height,
            AspectRatio,
            Orientation,
            InlineSize, BlockSize,
        } feature;
        
        float value = 0;
        std::string keyword;  // For orientation: "portrait", "landscape"
        
        enum class Op {
            Equal, GreaterThan, LessThan,
            GreaterEqual, LessEqual,
        } op = Op::GreaterEqual;
    };
    
    std::vector<Condition> conditions;
    enum class LogicalOp { And, Or, Not } logicalOp = LogicalOp::And;
};

class ContainerQueryEvaluator {
public:
    // Evaluate a container query against a container's dimensions
    static bool evaluate(const ContainerQuery& query,
                         float containerWidth,
                         float containerHeight,
                         ContainerType containerType);

    // Find the nearest ancestor container matching name/type
    struct ContainerInfo {
        Widget* container = nullptr;
        float width = 0;
        float height = 0;
        ContainerType type = ContainerType::Normal;
        std::string name;
    };

    static ContainerInfo findContainer(Widget* element,
                                        const std::string& name = "",
                                        ContainerType requiredType = ContainerType::InlineSize);

private:
    static bool evaluateCondition(const ContainerQuery::Condition& cond,
                                   float containerWidth,
                                   float containerHeight);
};

// ==================================================================
// Aspect Ratio
// CSS Box Sizing Module Level 4
// ==================================================================

struct AspectRatioConfig {
    float ratio = 0;            // Width / Height (0 = none)
    bool preferIntrinsic = true; // aspect-ratio: auto 16/9
};

class AspectRatioLayout {
public:
    // Resolve width/height given one dimension and aspect ratio
    static float resolveWidth(float height, const AspectRatioConfig& config);
    static float resolveHeight(float width, const AspectRatioConfig& config);

    // Apply aspect ratio to a box, respecting min/max constraints
    static void applyAspectRatio(float& width, float& height,
                                  bool widthAuto, bool heightAuto,
                                  float minWidth, float maxWidth,
                                  float minHeight, float maxHeight,
                                  const AspectRatioConfig& config);

    // Parse "auto 16/9" string
    static AspectRatioConfig parse(const std::string& value);
};

// ==================================================================
// Logical Properties
// CSS Logical Properties and Values Level 1
// ==================================================================

enum class WritingDirection : uint8_t {
    HorizontalTB = 0,   // writing-mode: horizontal-tb
    VerticalRL = 1,      // writing-mode: vertical-rl
    VerticalLR = 2,      // writing-mode: vertical-lr
};

enum class TextDirection : uint8_t {
    LTR = 0,
    RTL = 1,
};

struct LogicalEdges {
    float blockStart = 0;
    float blockEnd = 0;
    float inlineStart = 0;
    float inlineEnd = 0;
};

class LogicalPropertyMapper {
public:
    // Map logical to physical edges
    static EdgeInsets toPhysical(const LogicalEdges& logical,
                                 WritingDirection writing,
                                 TextDirection direction);

    // Map physical to logical edges
    static LogicalEdges toLogical(const EdgeInsets& physical,
                                   WritingDirection writing,
                                   TextDirection direction);

    // Resolve inline-size / block-size to width/height
    static void resolveLogicalSizes(float inlineSize, float blockSize,
                                     float& width, float& height,
                                     WritingDirection writing);

    // Map logical position properties to physical
    static void resolveLogicalPosition(float insetBlockStart, float insetBlockEnd,
                                        float insetInlineStart, float insetInlineEnd,
                                        float& top, float& right,
                                        float& bottom, float& left,
                                        WritingDirection writing,
                                        TextDirection direction);
};

} // namespace NXRender
