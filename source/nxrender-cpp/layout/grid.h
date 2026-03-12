// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file grid.h
 * @brief CSS Grid layout implementation
 */

#pragma once

#include "layout.h"
#include "../nxgfx/primitives.h"
#include <vector>
#include <string>
#include <variant>

namespace NXRender {

class Widget;

/**
 * @brief Grid track sizing
 */
struct GridTrackSize {
    enum class Type {
        Fixed,      // Fixed pixel size
        Fraction,   // fr unit (flexible)
        Auto,       // Auto-size based on content
        MinContent, // Minimum content size
        MaxContent, // Maximum content size
        MinMax      // minmax(min, max)
    };
    
    Type type = Type::Auto;
    float value = 0;      // For Fixed or Fraction
    float minValue = 0;   // For MinMax
    float maxValue = 0;   // For MinMax
    
    static GridTrackSize fixed(float px) {
        return {Type::Fixed, px, 0, 0};
    }
    static GridTrackSize fr(float fraction = 1) {
        return {Type::Fraction, fraction, 0, 0};
    }
    static GridTrackSize autoSize() {
        return {Type::Auto, 0, 0, 0};
    }
    static GridTrackSize minmax(float min, float max) {
        return {Type::MinMax, 0, min, max};
    }
};

/**
 * @brief Grid line (for placement)
 */
struct GridLine {
    int line = 0;       // Line number (1-based, negative counts from end)
    std::string name;   // Named line
    bool isSpan = false;
    
    static GridLine at(int n) { return {n, "", false}; }
    static GridLine named(const std::string& name) { return {0, name, false}; }
    static GridLine span(int n) { return {n, "", true}; }
};

/**
 * @brief Grid item placement
 */
struct GridPlacement {
    GridLine rowStart;
    GridLine rowEnd;
    GridLine columnStart;
    GridLine columnEnd;
    
    // Default: auto placement
    GridPlacement() = default;
    
    // Explicit placement
    GridPlacement(int row, int col) {
        rowStart = GridLine::at(row);
        columnStart = GridLine::at(col);
    }
    
    // Spanning
    GridPlacement& spanRows(int n) { rowEnd = GridLine::span(n); return *this; }
    GridPlacement& spanColumns(int n) { columnEnd = GridLine::span(n); return *this; }
};

/**
 * @brief CSS Grid layout engine
 */
class GridLayout {
public:
    GridLayout();
    ~GridLayout() = default;
    
    // Template definitions
    void setTemplateColumns(const std::vector<GridTrackSize>& tracks);
    void setTemplateRows(const std::vector<GridTrackSize>& tracks);
    
    // Parse CSS-like syntax: "1fr 2fr auto 100px"
    void setTemplateColumns(const std::string& definition);
    void setTemplateRows(const std::string& definition);
    
    // Repeat helper: repeat(3, 1fr) = "1fr 1fr 1fr"
    static std::vector<GridTrackSize> repeat(int count, const GridTrackSize& size);
    
    // Gaps
    float rowGap = 0;
    float columnGap = 0;
    void setGap(float g) { rowGap = columnGap = g; }
    
    // Auto placement
    enum class AutoFlow { Row, Column, RowDense, ColumnDense };
    AutoFlow autoFlow = AutoFlow::Row;
    
    // Auto track sizing (for implicit tracks)
    GridTrackSize autoRows;
    GridTrackSize autoColumns;
    
    // Content alignment
    enum class ContentAlign { Start, End, Center, Stretch, SpaceBetween, SpaceAround, SpaceEvenly };
    ContentAlign justifyContent = ContentAlign::Stretch;
    ContentAlign alignContent = ContentAlign::Stretch;
    
    // Item alignment (default for items)
    enum class ItemAlign { Start, End, Center, Stretch, Baseline };
    ItemAlign justifyItems = ItemAlign::Stretch;
    ItemAlign alignItems = ItemAlign::Stretch;
    
    // Layout computation
    void layout(std::vector<Widget*>& children, const Rect& container);
    Size measure(std::vector<Widget*>& children, const Size& available);
    
    // Get resolved track sizes after layout
    const std::vector<float>& resolvedColumnSizes() const { return resolvedColumns_; }
    const std::vector<float>& resolvedRowSizes() const { return resolvedRows_; }
    
    // Get cell position
    Rect getCellRect(int row, int column) const;
    Rect getAreaRect(int rowStart, int columnStart, int rowEnd, int columnEnd) const;
    
private:
    std::vector<GridTrackSize> templateColumns_;
    std::vector<GridTrackSize> templateRows_;
    
    // Computed values
    std::vector<float> resolvedColumns_;
    std::vector<float> resolvedRows_;
    std::vector<float> columnPositions_;
    std::vector<float> rowPositions_;
    
    // Track sizing algorithm
    void resolveTrackSizes(const std::vector<GridTrackSize>& tracks,
                           float available, float gap,
                           std::vector<float>& output);
    
    // Parse track definition
    std::vector<GridTrackSize> parseTracks(const std::string& def);
    
    // Auto-placement
    struct GridCell {
        int row = -1, col = -1;
        int rowSpan = 1, colSpan = 1;
        Widget* widget = nullptr;
    };
    std::vector<GridCell> placementGrid_;
    void autoPlace(std::vector<Widget*>& children);
    bool findEmptyCell(int& row, int& col, int rowSpan, int colSpan);
};

/**
 * @brief Grid item properties (attached to widgets)
 */
struct GridItemProps {
    GridPlacement placement;
    
    // Self alignment
    enum class Align { Auto, Start, End, Center, Stretch };
    Align justifySelf = Align::Auto;
    Align alignSelf = Align::Auto;
    
    // Order (affects auto-placement order)
    int order = 0;
};

} // namespace NXRender
