// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file flexbox.h
 * @brief Flexbox layout implementation
 */

#pragma once

#include "layout.h"
#include "../nxgfx/primitives.h"
#include <vector>

namespace NXRender {

class Widget;

/**
 * @brief Flex direction
 */
enum class FlexDirection {
    Row,
    RowReverse,
    Column,
    ColumnReverse
};

/**
 * @brief Justify content (main axis)
 */
enum class JustifyContent {
    FlexStart,
    FlexEnd,
    Center,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};

/**
 * @brief Align items (cross axis)
 */
enum class AlignItems {
    FlexStart,
    FlexEnd,
    Center,
    Stretch,
    Baseline
};

/**
 * @brief Flex wrap
 */
enum class FlexWrap {
    NoWrap,
    Wrap,
    WrapReverse
};

/**
 * @brief Flexbox layout engine
 */
class FlexLayout {
public:
    FlexLayout();
    ~FlexLayout() = default;
    
    // Configuration
    FlexDirection direction = FlexDirection::Row;
    JustifyContent justifyContent = JustifyContent::FlexStart;
    AlignItems alignItems = AlignItems::Stretch;
    FlexWrap wrap = FlexWrap::NoWrap;
    float gap = 0;
    float rowGap = 0;
    float columnGap = 0;
    
    // Builder pattern
    FlexLayout& setDirection(FlexDirection dir) { direction = dir; return *this; }
    FlexLayout& setJustify(JustifyContent j) { justifyContent = j; return *this; }
    FlexLayout& setAlignItems(AlignItems a) { alignItems = a; return *this; }
    FlexLayout& setWrap(FlexWrap w) { wrap = w; return *this; }
    FlexLayout& setGap(float g) { gap = g; rowGap = g; columnGap = g; return *this; }
    
    // Layout computation
    void layout(std::vector<Widget*>& children, const Rect& container);
    Size measure(std::vector<Widget*>& children, const Size& available);
    
    // Convenience constructors
    static FlexLayout row() { FlexLayout f; f.direction = FlexDirection::Row; return f; }
    static FlexLayout column() { FlexLayout f; f.direction = FlexDirection::Column; return f; }
    
private:
    bool isMainAxisHorizontal() const {
        return direction == FlexDirection::Row || direction == FlexDirection::RowReverse;
    }
    
    float mainGap() const { return isMainAxisHorizontal() ? columnGap : rowGap; }
    float crossGap() const { return isMainAxisHorizontal() ? rowGap : columnGap; }
};

/**
 * @brief Flex item properties (attached to widgets)
 */
struct FlexItemProps {
    float flexGrow = 0;
    float flexShrink = 1;
    float flexBasis = -1;  // -1 = auto
    AlignItems alignSelf = AlignItems::Stretch;
    int order = 0;
};

} // namespace NXRender
