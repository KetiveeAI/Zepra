// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file table_layout.h
 * @brief CSS table layout algorithm.
 *
 * Implements the fixed and auto table layout algorithms per CSS 2.1 §17.5.
 * Handles colspan/rowspan, border-collapse, and caption positioning.
 */

#pragma once

#include "layout.h"
#include "../nxgfx/primitives.h"
#include <vector>
#include <cstdint>
#include <string>

namespace NXRender {

enum class TableLayoutMode : uint8_t {
    Auto,   // Column widths determined by content
    Fixed   // Column widths determined by first row / explicit widths
};

enum class BorderCollapse : uint8_t {
    Separate,
    Collapse
};

enum class CaptionSide : uint8_t {
    Top,
    Bottom
};

struct TableCell {
    int row = 0;
    int col = 0;
    int rowSpan = 1;
    int colSpan = 1;

    float minWidth = 0;
    float maxWidth = 0;
    float preferredWidth = 0;
    float specifiedWidth = -1; // -1 = auto

    float minHeight = 0;
    float preferredHeight = 0;
    float specifiedHeight = -1;

    EdgeInsets padding;
    EdgeInsets border;

    float computedX = 0;
    float computedY = 0;
    float computedWidth = 0;
    float computedHeight = 0;

    bool isSpanned = false; // Covered by another cell's rowspan/colspan
    int spanOwnRow = -1;    // Row of the cell that spans into this
    int spanOwnCol = -1;    // Col of the cell that spans into this
};

struct TableColumn {
    float minWidth = 0;
    float maxWidth = 1e6f;
    float preferredWidth = 0;
    float specifiedWidth = -1;
    float computedWidth = 0;
    float percentWidth = -1; // -1 = no percentage
};

struct TableRow {
    float minHeight = 0;
    float preferredHeight = 0;
    float specifiedHeight = -1;
    float computedHeight = 0;
    float computedY = 0;
};

struct TableLayoutInput {
    int rows = 0;
    int cols = 0;
    float availableWidth = 0;

    TableLayoutMode mode = TableLayoutMode::Auto;
    BorderCollapse borderCollapse = BorderCollapse::Separate;
    CaptionSide captionSide = CaptionSide::Top;

    float borderSpacingH = 2; // Horizontal border spacing
    float borderSpacingV = 2; // Vertical border spacing

    float tableWidth = -1;  // Specified table width (-1 = auto)
    EdgeInsets tablePadding;
    EdgeInsets tableBorder;

    float captionHeight = 0;
};

struct TableLayoutResult {
    std::vector<TableColumn> columns;
    std::vector<TableRow> rowResults;
    std::vector<std::vector<TableCell>> cells;
    float totalWidth = 0;
    float totalHeight = 0;
    float captionY = 0;
    float tableContentY = 0;
};

/**
 * @brief CSS table layout engine.
 */
class TableLayout {
public:
    TableLayout();

    /**
     * @brief Configure the table dimensions.
     */
    void init(int rows, int cols);

    /**
     * @brief Set cell content metrics.
     */
    void setCell(int row, int col, float minWidth, float maxWidth,
                 float minHeight, const EdgeInsets& padding = EdgeInsets());

    /**
     * @brief Set cell spanning.
     */
    void setCellSpan(int row, int col, int rowSpan, int colSpan);

    /**
     * @brief Set explicit column width.
     */
    void setColumnWidth(int col, float width);

    /**
     * @brief Set column width as percentage.
     */
    void setColumnPercent(int col, float percent);

    /**
     * @brief Set explicit row height.
     */
    void setRowHeight(int row, float height);

    /**
     * @brief Run the layout algorithm.
     */
    TableLayoutResult compute(const TableLayoutInput& input);

private:
    void computeColumnWidths(const TableLayoutInput& input, TableLayoutResult& result);
    void computeAutoColumnWidths(const TableLayoutInput& input, TableLayoutResult& result);
    void computeFixedColumnWidths(const TableLayoutInput& input, TableLayoutResult& result);
    void distributeExcessWidth(float excess, TableLayoutResult& result);
    void computeRowHeights(const TableLayoutInput& input, TableLayoutResult& result);
    void positionCells(const TableLayoutInput& input, TableLayoutResult& result);
    void resolveSpans(TableLayoutResult& result);

    int rows_ = 0;
    int cols_ = 0;
    std::vector<std::vector<TableCell>> cells_;
    std::vector<TableColumn> columns_;
    std::vector<TableRow> rows_data_;
};

} // namespace NXRender
