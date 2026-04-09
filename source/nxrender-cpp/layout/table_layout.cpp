// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "layout/table_layout.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace NXRender {

TableLayout::TableLayout() {}

void TableLayout::init(int rows, int cols) {
    rows_ = rows;
    cols_ = cols;

    cells_.resize(static_cast<size_t>(rows));
    for (auto& row : cells_) {
        row.resize(static_cast<size_t>(cols));
    }

    columns_.resize(static_cast<size_t>(cols));
    rows_data_.resize(static_cast<size_t>(rows));

    // Initialize cell coordinates
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            cells_[r][c].row = r;
            cells_[r][c].col = c;
        }
    }
}

void TableLayout::setCell(int row, int col, float minWidth, float maxWidth,
                           float minHeight, const EdgeInsets& padding) {
    if (row < 0 || row >= rows_ || col < 0 || col >= cols_) return;
    auto& cell = cells_[static_cast<size_t>(row)][static_cast<size_t>(col)];
    cell.minWidth = minWidth;
    cell.maxWidth = maxWidth;
    cell.preferredWidth = maxWidth;
    cell.minHeight = minHeight;
    cell.preferredHeight = minHeight;
    cell.padding = padding;
}

void TableLayout::setCellSpan(int row, int col, int rowSpan, int colSpan) {
    if (row < 0 || row >= rows_ || col < 0 || col >= cols_) return;
    auto& cell = cells_[static_cast<size_t>(row)][static_cast<size_t>(col)];
    cell.rowSpan = std::max(1, std::min(rowSpan, rows_ - row));
    cell.colSpan = std::max(1, std::min(colSpan, cols_ - col));

    // Mark spanned cells
    for (int r = row; r < row + cell.rowSpan; r++) {
        for (int c = col; c < col + cell.colSpan; c++) {
            if (r == row && c == col) continue;
            if (r < rows_ && c < cols_) {
                cells_[r][c].isSpanned = true;
                cells_[r][c].spanOwnRow = row;
                cells_[r][c].spanOwnCol = col;
            }
        }
    }
}

void TableLayout::setColumnWidth(int col, float width) {
    if (col < 0 || col >= cols_) return;
    columns_[static_cast<size_t>(col)].specifiedWidth = width;
}

void TableLayout::setColumnPercent(int col, float percent) {
    if (col < 0 || col >= cols_) return;
    columns_[static_cast<size_t>(col)].percentWidth = percent;
}

void TableLayout::setRowHeight(int row, float height) {
    if (row < 0 || row >= rows_) return;
    rows_data_[static_cast<size_t>(row)].specifiedHeight = height;
}

TableLayoutResult TableLayout::compute(const TableLayoutInput& input) {
    TableLayoutResult result;
    result.columns = columns_;
    result.rowResults = rows_data_;
    result.cells = cells_;

    if (rows_ == 0 || cols_ == 0) return result;

    // Phase 1: Compute column widths
    computeColumnWidths(input, result);

    // Phase 2: Resolve spans
    resolveSpans(result);

    // Phase 3: Compute row heights
    computeRowHeights(input, result);

    // Phase 4: Position cells
    positionCells(input, result);

    return result;
}

void TableLayout::computeColumnWidths(const TableLayoutInput& input, TableLayoutResult& result) {
    if (input.mode == TableLayoutMode::Fixed) {
        computeFixedColumnWidths(input, result);
    } else {
        computeAutoColumnWidths(input, result);
    }
}

void TableLayout::computeAutoColumnWidths(const TableLayoutInput& input, TableLayoutResult& result) {
    float spacingTotal = static_cast<float>(cols_ + 1) * input.borderSpacingH;
    float availWidth = input.availableWidth - input.tablePadding.horizontal()
                       - input.tableBorder.horizontal() - spacingTotal;

    // Step 1: Gather min/max widths from non-spanning cells
    for (int c = 0; c < cols_; c++) {
        float colMin = 0;
        float colMax = 0;

        for (int r = 0; r < rows_; r++) {
            const auto& cell = result.cells[r][c];
            if (cell.isSpanned || cell.colSpan > 1) continue;

            float cellPad = cell.padding.horizontal();
            colMin = std::max(colMin, cell.minWidth + cellPad);
            colMax = std::max(colMax, cell.maxWidth + cellPad);
        }

        result.columns[c].minWidth = colMin;
        result.columns[c].maxWidth = colMax;
        result.columns[c].preferredWidth = colMax;
    }

    // Step 2: Handle percentage widths
    for (int c = 0; c < cols_; c++) {
        if (result.columns[c].percentWidth >= 0) {
            float pctWidth = availWidth * result.columns[c].percentWidth / 100.0f;
            result.columns[c].preferredWidth = std::max(pctWidth, result.columns[c].minWidth);
        }
    }

    // Step 3: Handle specified widths
    for (int c = 0; c < cols_; c++) {
        if (result.columns[c].specifiedWidth >= 0) {
            result.columns[c].preferredWidth =
                std::max(result.columns[c].specifiedWidth, result.columns[c].minWidth);
        }
    }

    // Step 4: Distribute width to columns
    float totalPreferred = 0;
    for (int c = 0; c < cols_; c++) {
        totalPreferred += result.columns[c].preferredWidth;
    }

    if (totalPreferred <= availWidth) {
        // Distribute excess
        float excess = availWidth - totalPreferred;
        distributeExcessWidth(excess, result);
    } else {
        // Shrink columns proportionally (but not below min)
        float totalMin = 0;
        for (int c = 0; c < cols_; c++) totalMin += result.columns[c].minWidth;

        if (availWidth <= totalMin) {
            for (int c = 0; c < cols_; c++) {
                result.columns[c].computedWidth = result.columns[c].minWidth;
            }
        } else {
            float shrinkable = totalPreferred - totalMin;
            float shrinkNeeded = totalPreferred - availWidth;

            for (int c = 0; c < cols_; c++) {
                float colShrinkable = result.columns[c].preferredWidth - result.columns[c].minWidth;
                float colShrink = (shrinkable > 0)
                    ? shrinkNeeded * (colShrinkable / shrinkable) : 0;
                result.columns[c].computedWidth = result.columns[c].preferredWidth - colShrink;
            }
        }
        return;
    }

    for (int c = 0; c < cols_; c++) {
        if (result.columns[c].computedWidth <= 0) {
            result.columns[c].computedWidth = result.columns[c].preferredWidth;
        }
    }
}

void TableLayout::computeFixedColumnWidths(const TableLayoutInput& input, TableLayoutResult& result) {
    float spacingTotal = static_cast<float>(cols_ + 1) * input.borderSpacingH;
    float availWidth = input.availableWidth - input.tablePadding.horizontal()
                       - input.tableBorder.horizontal() - spacingTotal;

    int fixedCols = 0;
    float fixedWidth = 0;

    // Use first row and specified widths
    for (int c = 0; c < cols_; c++) {
        float w = -1;
        if (result.columns[c].specifiedWidth >= 0) {
            w = result.columns[c].specifiedWidth;
        } else if (result.columns[c].percentWidth >= 0) {
            w = availWidth * result.columns[c].percentWidth / 100.0f;
        } else if (rows_ > 0 && !result.cells[0][c].isSpanned) {
            if (result.cells[0][c].specifiedWidth >= 0) {
                w = result.cells[0][c].specifiedWidth;
            }
        }

        if (w >= 0) {
            result.columns[c].computedWidth = w;
            fixedWidth += w;
            fixedCols++;
        }
    }

    // Distribute remaining width to auto columns
    int autoCols = cols_ - fixedCols;
    if (autoCols > 0) {
        float remaining = std::max(0.0f, availWidth - fixedWidth);
        float perCol = remaining / static_cast<float>(autoCols);
        for (int c = 0; c < cols_; c++) {
            if (result.columns[c].computedWidth <= 0) {
                result.columns[c].computedWidth = perCol;
            }
        }
    }
}

void TableLayout::distributeExcessWidth(float excess, TableLayoutResult& result) {
    if (excess <= 0 || cols_ == 0) return;

    // Give excess to auto-width columns proportionally
    int autoColCount = 0;
    for (int c = 0; c < cols_; c++) {
        if (result.columns[c].specifiedWidth < 0 && result.columns[c].percentWidth < 0) {
            autoColCount++;
        }
    }

    if (autoColCount > 0) {
        float perCol = excess / static_cast<float>(autoColCount);
        for (int c = 0; c < cols_; c++) {
            if (result.columns[c].specifiedWidth < 0 && result.columns[c].percentWidth < 0) {
                result.columns[c].computedWidth = result.columns[c].preferredWidth + perCol;
            } else {
                result.columns[c].computedWidth = result.columns[c].preferredWidth;
            }
        }
    } else {
        // All columns have explicit widths, distribute evenly
        float perCol = excess / static_cast<float>(cols_);
        for (int c = 0; c < cols_; c++) {
            result.columns[c].computedWidth = result.columns[c].preferredWidth + perCol;
        }
    }
}

void TableLayout::resolveSpans(TableLayoutResult& result) {
    // Distribute spanning cell widths to columns
    for (int r = 0; r < rows_; r++) {
        for (int c = 0; c < cols_; c++) {
            auto& cell = result.cells[r][c];
            if (cell.isSpanned || cell.colSpan <= 1) continue;

            // Sum the widths of spanned columns
            float spannedWidth = 0;
            for (int sc = c; sc < c + cell.colSpan && sc < cols_; sc++) {
                spannedWidth += result.columns[sc].computedWidth;
            }

            float cellNeed = cell.minWidth + cell.padding.horizontal();
            if (cellNeed > spannedWidth) {
                float deficit = cellNeed - spannedWidth;
                float perCol = deficit / static_cast<float>(cell.colSpan);
                for (int sc = c; sc < c + cell.colSpan && sc < cols_; sc++) {
                    result.columns[sc].computedWidth += perCol;
                }
            }
        }
    }
}

void TableLayout::computeRowHeights(const TableLayoutInput& input, TableLayoutResult& result) {
    for (int r = 0; r < rows_; r++) {
        float rowHeight = 0;

        for (int c = 0; c < cols_; c++) {
            auto& cell = result.cells[r][c];
            if (cell.isSpanned || cell.rowSpan > 1) continue;

            float cellHeight = cell.minHeight + cell.padding.vertical();
            rowHeight = std::max(rowHeight, cellHeight);
        }

        if (result.rowResults[r].specifiedHeight >= 0) {
            rowHeight = std::max(rowHeight, result.rowResults[r].specifiedHeight);
        }

        result.rowResults[r].computedHeight = rowHeight;
    }

    // Handle rowspan
    for (int r = 0; r < rows_; r++) {
        for (int c = 0; c < cols_; c++) {
            auto& cell = result.cells[r][c];
            if (cell.isSpanned || cell.rowSpan <= 1) continue;

            float spannedHeight = 0;
            for (int sr = r; sr < r + cell.rowSpan && sr < rows_; sr++) {
                spannedHeight += result.rowResults[sr].computedHeight;
            }
            spannedHeight += static_cast<float>(cell.rowSpan - 1) * input.borderSpacingV;

            float cellNeed = cell.minHeight + cell.padding.vertical();
            if (cellNeed > spannedHeight) {
                float deficit = cellNeed - spannedHeight;
                float perRow = deficit / static_cast<float>(cell.rowSpan);
                for (int sr = r; sr < r + cell.rowSpan && sr < rows_; sr++) {
                    result.rowResults[sr].computedHeight += perRow;
                }
            }
        }
    }
}

void TableLayout::positionCells(const TableLayoutInput& input, TableLayoutResult& result) {
    // Compute column X positions
    float x = input.tablePadding.left + input.tableBorder.left + input.borderSpacingH;
    for (int c = 0; c < cols_; c++) {
        for (int r = 0; r < rows_; r++) {
            result.cells[r][c].computedX = x;
        }
        x += result.columns[c].computedWidth + input.borderSpacingH;
    }

    // Compute row Y positions
    float captionOffset = 0;
    if (input.captionHeight > 0) {
        if (input.captionSide == CaptionSide::Top) {
            result.captionY = input.tablePadding.top;
            captionOffset = input.captionHeight + input.borderSpacingV;
        }
    }

    result.tableContentY = input.tablePadding.top + input.tableBorder.top + captionOffset;
    float y = result.tableContentY + input.borderSpacingV;

    for (int r = 0; r < rows_; r++) {
        result.rowResults[r].computedY = y;
        for (int c = 0; c < cols_; c++) {
            result.cells[r][c].computedY = y;
        }
        y += result.rowResults[r].computedHeight + input.borderSpacingV;
    }

    // Set cell computed dimensions
    for (int r = 0; r < rows_; r++) {
        for (int c = 0; c < cols_; c++) {
            auto& cell = result.cells[r][c];
            if (cell.isSpanned) continue;

            // Width: sum of spanned columns
            float w = 0;
            for (int sc = c; sc < c + cell.colSpan && sc < cols_; sc++) {
                w += result.columns[sc].computedWidth;
            }
            if (cell.colSpan > 1) {
                w += static_cast<float>(cell.colSpan - 1) * input.borderSpacingH;
            }
            cell.computedWidth = w;

            // Height: sum of spanned rows
            float h = 0;
            for (int sr = r; sr < r + cell.rowSpan && sr < rows_; sr++) {
                h += result.rowResults[sr].computedHeight;
            }
            if (cell.rowSpan > 1) {
                h += static_cast<float>(cell.rowSpan - 1) * input.borderSpacingV;
            }
            cell.computedHeight = h;
        }
    }

    // Total dimensions
    result.totalWidth = x + input.tablePadding.right + input.tableBorder.right;
    result.totalHeight = y + input.tablePadding.bottom + input.tableBorder.bottom;

    if (input.captionSide == CaptionSide::Bottom && input.captionHeight > 0) {
        result.captionY = result.totalHeight;
        result.totalHeight += input.captionHeight;
    }
}

} // namespace NXRender
