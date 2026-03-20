// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file inline_layout.cpp
 * @brief Inline formatting context — connects LineLayout to the widget tree
 *
 * Breaks widget children into InlineFragments, feeds them through LineLayout,
 * and positions the resulting LineBoxes back onto the widget tree.
 */

#include "layout/line_layout.h"
#include "widgets/widget.h"
#include "nxgfx/text.h"
#include <string>
#include <vector>

namespace NXRender {

/**
 * @brief Lay out inline children within a container
 *
 * @param children     Widgets to lay out inline (Labels, inline containers)
 * @param container    Available rectangle
 * @param textAlign    Text alignment for lines
 * @param lineHeight   CSS line-height value (0 = auto from font metrics)
 *
 * @return Total content height consumed
 */
float layoutInline(std::vector<Widget*>& children, const Rect& container,
                   TextAlign textAlign, float lineHeight) {
    if (children.empty()) return 0;

    LineLayout ll;
    ll.setTextAlign(textAlign);
    if (lineHeight > 0) ll.setLineHeight(lineHeight);

    ll.beginLayout(container.width);
    ll.beginLine(0);

    // Map each child widget to an InlineFragment
    struct FragmentWidget {
        InlineFragment frag;
        Widget* widget;
    };
    std::vector<FragmentWidget> fragmentWidgets;

    for (Widget* child : children) {
        if (!child->isVisible()) continue;

        // Measure the child
        Size childSize = child->measure(Size(container.width, container.height));

        InlineFragment frag;
        frag.width = childSize.width;
        frag.height = childSize.height;
        frag.ascent = childSize.height * 0.8f;  // Approximate baseline at 80%
        frag.descent = childSize.height * 0.2f;
        frag.isText = true;

        // Try to place on current line
        if (!ll.placeFragment(frag)) {
            // Line break needed — start a new line
            ll.finishLine();
            ll.beginLine(ll.totalHeight());
            ll.placeFragment(frag);
        }

        fragmentWidgets.push_back({frag, child});
    }

    const auto& lines = ll.endLayout();

    // Position widgets based on line layout results
    size_t fragIdx = 0;
    for (const auto& line : lines) {
        for (const auto& lineFrag : line.fragments) {
            if (fragIdx >= fragmentWidgets.size()) break;

            Widget* w = fragmentWidgets[fragIdx].widget;
            float x = container.x + lineFrag.xOffset;
            float y = container.y + line.y + lineFrag.yOffset;
            w->setBounds(Rect(x, y, lineFrag.width, lineFrag.height));
            fragIdx++;
        }
    }

    return ll.totalHeight();
}

} // namespace NXRender
