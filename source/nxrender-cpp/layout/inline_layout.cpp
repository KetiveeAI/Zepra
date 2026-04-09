// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "layout/line_layout.h"
#include "widgets/widget.h"
#include "nxgfx/text.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

namespace NXRender {

// ==================================================================
// Whitespace classification
// ==================================================================

enum class WhitespaceMode {
    Normal,     // Collapse sequences, wrap at container edge
    Nowrap,     // Collapse sequences, no wrapping
    Pre,        // Preserve all whitespace, no wrapping
    PreWrap,    // Preserve all whitespace, wrap at container edge
    PreLine     // Collapse spaces, preserve newlines, wrap
};

static bool shouldCollapse(WhitespaceMode mode) {
    return mode == WhitespaceMode::Normal || mode == WhitespaceMode::Nowrap;
}

static bool shouldWrap(WhitespaceMode mode) {
    return mode == WhitespaceMode::Normal ||
           mode == WhitespaceMode::PreWrap ||
           mode == WhitespaceMode::PreLine;
}

// ==================================================================
// Fragment metadata
// ==================================================================

struct InlineWidgetFragment {
    Widget* widget = nullptr;
    float width = 0;
    float height = 0;
    float ascent = 0;
    float descent = 0;
    bool isLineBreak = false;
    bool isWhitespace = false;
    float margin_left = 0;
    float margin_right = 0;
};

// ==================================================================
// Inline layout entry point
// ==================================================================

float layoutInline(std::vector<Widget*>& children, const Rect& container,
                   TextAlign textAlign, float lineHeight) {
    if (children.empty()) return 0;

    LineLayout ll;
    ll.setTextAlign(textAlign);
    if (lineHeight > 0) ll.setLineHeight(lineHeight);

    ll.beginLayout(container.width);
    ll.beginLine(0);

    std::vector<InlineWidgetFragment> fragments;
    fragments.reserve(children.size());

    for (Widget* child : children) {
        if (!child->isVisible()) continue;

        InlineWidgetFragment frag;
        frag.widget = child;

        EdgeInsets childMargin = child->margin();
        frag.margin_left = childMargin.left;
        frag.margin_right = childMargin.right;

        Size childSize = child->measure(Size(container.width, container.height));
        frag.width = childSize.width + frag.margin_left + frag.margin_right;
        frag.height = childSize.height;
        frag.ascent = childSize.height * 0.8f;
        frag.descent = childSize.height * 0.2f;

        InlineFragment lineFrag;
        lineFrag.width = frag.width;
        lineFrag.height = frag.height;
        lineFrag.ascent = frag.ascent;
        lineFrag.descent = frag.descent;
        lineFrag.isText = true;

        if (!ll.placeFragment(lineFrag)) {
            ll.finishLine();
            ll.beginLine(ll.totalHeight());
            ll.placeFragment(lineFrag);
        }

        fragments.push_back(frag);
    }

    const auto& lines = ll.endLayout();

    // Position widgets
    size_t fragIdx = 0;
    for (const auto& line : lines) {
        for (const auto& lineFrag : line.fragments) {
            if (fragIdx >= fragments.size()) break;

            InlineWidgetFragment& fw = fragments[fragIdx];
            Widget* w = fw.widget;

            float x = container.x + lineFrag.xOffset + fw.margin_left;
            float y = container.y + line.y + lineFrag.yOffset;
            float childWidth = fw.width - fw.margin_left - fw.margin_right;
            w->setBounds(Rect(x, y, childWidth, lineFrag.height));
            fragIdx++;
        }
    }

    return ll.totalHeight();
}

// ==================================================================
// Multi-line text layout helper
// ==================================================================

float layoutTextRuns(const std::vector<std::string>& textRuns,
                     const std::vector<float>& runWidths,
                     float containerWidth,
                     TextAlign align,
                     float lineHeight,
                     std::vector<Rect>& outPositions) {
    outPositions.clear();
    if (textRuns.empty()) return 0;

    float currentX = 0;
    float currentY = 0;
    float lineTop = 0;
    float maxLineHeight = lineHeight;

    // Line accumulator
    struct LineRun {
        size_t index;
        float x;
        float width;
    };
    std::vector<std::vector<LineRun>> allLines;
    std::vector<LineRun> currentLine;

    for (size_t i = 0; i < textRuns.size(); i++) {
        float runW = (i < runWidths.size()) ? runWidths[i] : 0;

        if (currentX + runW > containerWidth && !currentLine.empty()) {
            // Line break
            allLines.push_back(std::move(currentLine));
            currentLine.clear();
            currentX = 0;
            currentY += maxLineHeight;
        }

        LineRun run;
        run.index = i;
        run.x = currentX;
        run.width = runW;
        currentLine.push_back(run);
        currentX += runW;
    }
    if (!currentLine.empty()) {
        allLines.push_back(std::move(currentLine));
    }

    // Position all runs with alignment
    outPositions.resize(textRuns.size());
    float y = 0;
    for (const auto& line : allLines) {
        float lineWidth = 0;
        for (const auto& run : line) lineWidth += run.width;

        float alignOffset = 0;
        if (align == TextAlign::Center) {
            alignOffset = (containerWidth - lineWidth) / 2.0f;
        } else if (align == TextAlign::Right) {
            alignOffset = containerWidth - lineWidth;
        }

        for (const auto& run : line) {
            outPositions[run.index] = Rect(
                run.x + alignOffset, y,
                run.width, maxLineHeight
            );
        }
        y += maxLineHeight;
    }

    return y;
}

// ==================================================================
// Text-indent support
// ==================================================================

float layoutInlineWithIndent(std::vector<Widget*>& children, const Rect& container,
                              TextAlign textAlign, float lineHeight,
                              float textIndent) {
    if (children.empty()) return 0;

    // Adjust first line width
    Rect adjustedContainer = container;
    float firstLineIndent = textIndent;

    LineLayout ll;
    ll.setTextAlign(textAlign);
    if (lineHeight > 0) ll.setLineHeight(lineHeight);

    ll.beginLayout(container.width);
    ll.beginLine(0);

    bool firstLine = true;
    std::vector<InlineWidgetFragment> fragments;

    for (Widget* child : children) {
        if (!child->isVisible()) continue;

        Size childSize = child->measure(Size(container.width, container.height));

        InlineWidgetFragment frag;
        frag.widget = child;
        frag.width = childSize.width;
        frag.height = childSize.height;
        frag.ascent = childSize.height * 0.8f;
        frag.descent = childSize.height * 0.2f;

        float effectiveWidth = frag.width;
        if (firstLine) {
            effectiveWidth += firstLineIndent;
        }

        InlineFragment lineFrag;
        lineFrag.width = effectiveWidth;
        lineFrag.height = frag.height;
        lineFrag.ascent = frag.ascent;
        lineFrag.descent = frag.descent;
        lineFrag.isText = true;

        if (!ll.placeFragment(lineFrag)) {
            ll.finishLine();
            ll.beginLine(ll.totalHeight());
            firstLine = false;
            lineFrag.width = frag.width; // No indent on subsequent lines
            ll.placeFragment(lineFrag);
        }

        fragments.push_back(frag);
    }

    const auto& lines = ll.endLayout();

    size_t fragIdx = 0;
    for (size_t lineIdx = 0; lineIdx < lines.size(); lineIdx++) {
        const auto& line = lines[lineIdx];
        for (const auto& lineFrag : line.fragments) {
            if (fragIdx >= fragments.size()) break;

            Widget* w = fragments[fragIdx].widget;
            float xOffset = (lineIdx == 0) ? firstLineIndent : 0;
            float x = container.x + lineFrag.xOffset + xOffset;
            float y = container.y + line.y + lineFrag.yOffset;
            w->setBounds(Rect(x, y, lineFrag.width, lineFrag.height));
            fragIdx++;
        }
    }

    return ll.totalHeight();
}

} // namespace NXRender
