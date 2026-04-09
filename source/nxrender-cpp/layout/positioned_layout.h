// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file positioned_layout.h
 * @brief CSS positioned element layout (absolute, fixed, sticky)
 */

#pragma once

#include "layout.h"
#include "../nxgfx/primitives.h"
#include <vector>

namespace NXRender {

class Widget;

/**
 * @brief Position type matching CSS position property
 */
enum class PositionType {
    Static,
    Relative,
    Absolute,
    Fixed,
    Sticky
};

/**
 * @brief Positioning offsets (top/right/bottom/left)
 */
struct PositionOffsets {
    float top = 0;
    float right = 0;
    float bottom = 0;
    float left = 0;
    bool hasTop = false;
    bool hasRight = false;
    bool hasBottom = false;
    bool hasLeft = false;
};

/**
 * @brief Positioned element properties (attached to widgets)
 */
struct PositionedProps {
    PositionType type = PositionType::Static;
    PositionOffsets offsets;
    int zIndex = 0;
    bool zIndexAuto = true;
};

/**
 * @brief Positioned layout engine
 *
 * Handles position:absolute/fixed/sticky/relative.
 * Call after normal layout pass to adjust positioned elements.
 */
class PositionedLayout {
public:
    PositionedLayout() = default;

    static void apply(Widget* widget, const PositionedProps& props, const Rect& container);

    // Apply with scroll offset (for sticky elements)
    static void applySticky(Widget* widget, const PositionedProps& props,
                             const Rect& container, Rect& bounds,
                             float scrollX, float scrollY);

    // Find the containing block for a positioned widget
    static Rect findContainingBlock(Widget* widget, PositionType posType);

    // Centering utilities
    static void centerInContainer(Widget* widget, const Rect& container);
    static void centerHorizontally(Widget* widget, const Rect& container);
    static void centerVertically(Widget* widget, const Rect& container);

    static void sortByZIndex(std::vector<Widget*>& widgets,
                             const std::vector<PositionedProps>& props);

private:
    static void applyRelative(Widget* widget, const PositionedProps& props, Rect& bounds);
    static void applyAbsolute(Widget* widget, const PositionedProps& props,
                               const Rect& container, Rect& bounds);
};

} // namespace NXRender
