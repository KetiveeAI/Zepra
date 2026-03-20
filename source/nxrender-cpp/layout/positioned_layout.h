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

    /**
     * @brief Apply positioning to a widget
     * @param widget     Widget to position
     * @param props      Positioning properties
     * @param container  Containing block (parent for absolute, viewport for fixed)
     */
    static void apply(Widget* widget, const PositionedProps& props, const Rect& container);

    /**
     * @brief Sort widgets by stacking context (z-index)
     */
    static void sortByZIndex(std::vector<Widget*>& widgets,
                             const std::vector<PositionedProps>& props);
};

} // namespace NXRender
