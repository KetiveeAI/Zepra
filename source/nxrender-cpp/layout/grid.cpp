// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file grid.cpp
 * @brief CSS Grid layout implementation
 */

#include "layout/grid.h"
#include "widgets/widget.h"
#include <algorithm>
#include <numeric>
#include <sstream>

namespace NXRender {

GridLayout::GridLayout()
    : autoRows(GridTrackSize::autoSize())
    , autoColumns(GridTrackSize::autoSize()) {}

// =============================================================================
// Track definition parsing
// =============================================================================

std::vector<GridTrackSize> GridLayout::parseTracks(const std::string& def) {
    std::vector<GridTrackSize> tracks;
    std::istringstream ss(def);
    std::string token;

    while (ss >> token) {
        if (token == "auto") {
            tracks.push_back(GridTrackSize::autoSize());
        } else if (token.back() == '%') {
            // Percentage — treat as fraction for simplicity
            float pct = std::stof(token.substr(0, token.size() - 1));
            tracks.push_back(GridTrackSize::fr(pct / 100.0f));
        } else if (token.size() > 2 && token.substr(token.size() - 2) == "fr") {
            float val = std::stof(token.substr(0, token.size() - 2));
            tracks.push_back(GridTrackSize::fr(val));
        } else if (token.size() > 2 && token.substr(token.size() - 2) == "px") {
            float val = std::stof(token.substr(0, token.size() - 2));
            tracks.push_back(GridTrackSize::fixed(val));
        } else {
            // Bare number — treat as px
            float val = std::stof(token);
            tracks.push_back(GridTrackSize::fixed(val));
        }
    }

    return tracks;
}

void GridLayout::setTemplateColumns(const std::vector<GridTrackSize>& tracks) {
    templateColumns_ = tracks;
}

void GridLayout::setTemplateRows(const std::vector<GridTrackSize>& tracks) {
    templateRows_ = tracks;
}

void GridLayout::setTemplateColumns(const std::string& definition) {
    templateColumns_ = parseTracks(definition);
}

void GridLayout::setTemplateRows(const std::string& definition) {
    templateRows_ = parseTracks(definition);
}

std::vector<GridTrackSize> GridLayout::repeat(int count, const GridTrackSize& size) {
    return std::vector<GridTrackSize>(count, size);
}

// =============================================================================
// Track sizing algorithm
// =============================================================================

void GridLayout::resolveTrackSizes(const std::vector<GridTrackSize>& tracks,
                                   float available, float gap,
                                   std::vector<float>& output) {
    output.resize(tracks.size());
    if (tracks.empty()) return;

    float totalGaps = gap * (tracks.size() - 1);
    float remaining = available - totalGaps;
    float totalFr = 0;
    float usedFixed = 0;

    // First pass: resolve fixed and auto sizes
    for (size_t i = 0; i < tracks.size(); i++) {
        switch (tracks[i].type) {
            case GridTrackSize::Type::Fixed:
                output[i] = tracks[i].value;
                usedFixed += output[i];
                break;
            case GridTrackSize::Type::Auto:
            case GridTrackSize::Type::MinContent:
            case GridTrackSize::Type::MaxContent:
                output[i] = 0;  // Will be sized by content
                break;
            case GridTrackSize::Type::Fraction:
                totalFr += tracks[i].value;
                output[i] = 0;
                break;
            case GridTrackSize::Type::MinMax:
                output[i] = tracks[i].minValue;
                usedFixed += output[i];
                break;
        }
    }

    // Second pass: distribute remaining space to fr tracks
    float frSpace = remaining - usedFixed;
    if (frSpace > 0 && totalFr > 0) {
        float perFr = frSpace / totalFr;
        for (size_t i = 0; i < tracks.size(); i++) {
            if (tracks[i].type == GridTrackSize::Type::Fraction) {
                output[i] = perFr * tracks[i].value;
            }
        }
    }

    // Third pass: auto tracks get equal share of any remaining space
    int autoCount = 0;
    float autoSpace = remaining;
    for (size_t i = 0; i < tracks.size(); i++) {
        if (tracks[i].type == GridTrackSize::Type::Auto ||
            tracks[i].type == GridTrackSize::Type::MinContent ||
            tracks[i].type == GridTrackSize::Type::MaxContent) {
            autoCount++;
        } else {
            autoSpace -= output[i];
        }
    }
    if (autoCount > 0 && autoSpace > 0) {
        float perAuto = autoSpace / autoCount;
        for (size_t i = 0; i < tracks.size(); i++) {
            if (tracks[i].type == GridTrackSize::Type::Auto ||
                tracks[i].type == GridTrackSize::Type::MinContent ||
                tracks[i].type == GridTrackSize::Type::MaxContent) {
                output[i] = perAuto;
            }
        }
    }
}

// =============================================================================
// Auto-placement
// =============================================================================

void GridLayout::autoPlace(std::vector<Widget*>& children) {
    placementGrid_.clear();
    int numCols = std::max(1, static_cast<int>(templateColumns_.size()));
    int numRows = std::max(1, static_cast<int>(templateRows_.size()));

    // Ensure enough rows for all children
    int needed = (static_cast<int>(children.size()) + numCols - 1) / numCols;
    numRows = std::max(numRows, needed);

    int row = 0, col = 0;
    for (Widget* child : children) {
        GridCell cell;
        cell.row = row;
        cell.col = col;
        cell.widget = child;
        placementGrid_.push_back(cell);

        if (autoFlow == AutoFlow::Column || autoFlow == AutoFlow::ColumnDense) {
            row++;
            if (row >= numRows) { row = 0; col++; }
        } else {
            col++;
            if (col >= numCols) { col = 0; row++; }
        }
    }

    // Ensure template rows covers all used rows
    while (static_cast<int>(templateRows_.size()) < numRows) {
        templateRows_.push_back(autoRows);
    }
}

bool GridLayout::findEmptyCell(int& row, int& col, int rowSpan, int colSpan) {
    int numCols = std::max(1, static_cast<int>(templateColumns_.size()));
    // Simple linear scan
    for (int r = 0; r < 100; r++) {
        for (int c = 0; c <= numCols - colSpan; c++) {
            bool occupied = false;
            for (const auto& cell : placementGrid_) {
                if (cell.row >= r && cell.row < r + rowSpan &&
                    cell.col >= c && cell.col < c + colSpan) {
                    occupied = true;
                    break;
                }
            }
            if (!occupied) {
                row = r;
                col = c;
                return true;
            }
        }
    }
    return false;
}

// =============================================================================
// Layout
// =============================================================================

void GridLayout::layout(std::vector<Widget*>& children, const Rect& container) {
    if (children.empty()) return;

    // Ensure at least one column
    if (templateColumns_.empty()) {
        templateColumns_.push_back(GridTrackSize::fr(1));
    }

    // Auto-place children
    autoPlace(children);

    // Resolve column and row sizes
    resolveTrackSizes(templateColumns_, container.width, columnGap, resolvedColumns_);
    resolveTrackSizes(templateRows_, container.height, rowGap, resolvedRows_);

    // Compute track positions
    columnPositions_.resize(resolvedColumns_.size());
    float x = container.x;
    for (size_t i = 0; i < resolvedColumns_.size(); i++) {
        columnPositions_[i] = x;
        x += resolvedColumns_[i] + columnGap;
    }

    rowPositions_.resize(resolvedRows_.size());
    float y = container.y;
    for (size_t i = 0; i < resolvedRows_.size(); i++) {
        rowPositions_[i] = y;
        y += resolvedRows_[i] + rowGap;
    }

    // Position each child in its grid cell
    for (const auto& cell : placementGrid_) {
        if (!cell.widget) continue;
        if (cell.col >= static_cast<int>(columnPositions_.size())) continue;
        if (cell.row >= static_cast<int>(rowPositions_.size())) continue;

        float cx = columnPositions_[cell.col];
        float cy = rowPositions_[cell.row];
        float cw = resolvedColumns_[cell.col];
        float ch = resolvedRows_[cell.row];

        // Handle spanning
        for (int s = 1; s < cell.colSpan && cell.col + s < static_cast<int>(resolvedColumns_.size()); s++) {
            cw += resolvedColumns_[cell.col + s] + columnGap;
        }
        for (int s = 1; s < cell.rowSpan && cell.row + s < static_cast<int>(resolvedRows_.size()); s++) {
            ch += resolvedRows_[cell.row + s] + rowGap;
        }

        cell.widget->setBounds(Rect(cx, cy, cw, ch));
    }
}

Size GridLayout::measure(std::vector<Widget*>& children, const Size& available) {
    if (children.empty()) return Size(0, 0);

    if (templateColumns_.empty()) {
        templateColumns_.push_back(GridTrackSize::fr(1));
    }

    std::vector<float> cols, rows;
    resolveTrackSizes(templateColumns_, available.width, columnGap, cols);

    int numCols = static_cast<int>(cols.size());
    int numRows = (static_cast<int>(children.size()) + numCols - 1) / numCols;

    std::vector<GridTrackSize> rowDefs(numRows, autoRows);
    resolveTrackSizes(rowDefs, available.height, rowGap, rows);

    float totalWidth = std::accumulate(cols.begin(), cols.end(), 0.0f) +
                       columnGap * std::max(0, static_cast<int>(cols.size()) - 1);
    float totalHeight = std::accumulate(rows.begin(), rows.end(), 0.0f) +
                        rowGap * std::max(0, static_cast<int>(rows.size()) - 1);

    return Size(totalWidth, totalHeight);
}

Rect GridLayout::getCellRect(int row, int column) const {
    if (column >= static_cast<int>(columnPositions_.size()) ||
        row >= static_cast<int>(rowPositions_.size())) {
        return Rect(0, 0, 0, 0);
    }
    return Rect(columnPositions_[column], rowPositions_[row],
                resolvedColumns_[column], resolvedRows_[row]);
}

Rect GridLayout::getAreaRect(int rowStart, int columnStart, int rowEnd, int columnEnd) const {
    Rect start = getCellRect(rowStart, columnStart);
    Rect end = getCellRect(rowEnd - 1, columnEnd - 1);
    return Rect(start.x, start.y,
                end.x + end.width - start.x,
                end.y + end.height - start.y);
}

} // namespace NXRender
