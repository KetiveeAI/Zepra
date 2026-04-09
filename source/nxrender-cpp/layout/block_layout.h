// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file block_layout.h
 * @brief Block formatting context layout
 *
 * Normal flow: children stack vertically, margin collapse between siblings.
 * Handles width/height auto-sizing, percentage resolution, and box model.
 */

#pragma once

#include "layout.h"
#include "../nxgfx/primitives.h"
#include <vector>

namespace NXRender {

class Widget;

/**
 * @brief Block layout engine — normal flow
 *
 * Stacks children vertically with margin collapse.
 * Applies padding to the content area.
 * Each child fills the available width by default (block-level behavior).
 */
class BlockLayout {
public:
    BlockLayout() = default;
    ~BlockLayout() = default;

    void layout(std::vector<Widget*>& children, const Rect& container,
                const EdgeInsets& padding = {});
    Size measure(std::vector<Widget*>& children, const Size& available,
                 const EdgeInsets& padding = {});

    // Intrinsic sizing (shrink-to-fit)
    static float intrinsicMinWidth(std::vector<Widget*>& children,
                                    const EdgeInsets& padding = {});
    static float intrinsicMaxWidth(std::vector<Widget*>& children,
                                    const EdgeInsets& padding = {});

    // Center a child horizontally within container
    static void centerChild(Widget* child, const Rect& container);

    // Layout with float clearance
    void layoutWithClearance(std::vector<Widget*>& children,
                             const Rect& container,
                             const EdgeInsets& padding,
                             float clearanceHeight);

    static float collapseMargin(float marginA, float marginB);
};

} // namespace NXRender
