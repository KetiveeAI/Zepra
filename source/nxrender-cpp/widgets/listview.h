// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file listview.h
 * @brief ListView widget with virtual scrolling and cell reuse
 */

#pragma once

#include "widget.h"
#include "../nxgfx/color.h"
#include <functional>
#include <vector>

namespace NXRender {

/**
 * @brief Cell configuration callback
 */
using CellConfigCallback = std::function<void(Widget* cell, size_t index)>;

/**
 * @brief Selection callback
 */
using SelectionCallback = std::function<void(size_t index)>;

/**
 * @brief ListView with virtual scrolling
 */
class ListView : public Widget {
public:
    ListView(size_t itemCount = 0, float rowHeight = 32);
    ~ListView() override = default;
    
    // Data
    size_t itemCount() const { return itemCount_; }
    void setItemCount(size_t count);
    void reloadData();
    
    // Row height
    float rowHeight() const { return rowHeight_; }
    void setRowHeight(float h) { rowHeight_ = h; }
    
    // Selection
    size_t selectedIndex() const { return selectedIndex_; }
    void setSelectedIndex(size_t index);
    void clearSelection() { selectedIndex_ = SIZE_MAX; }
    
    bool multiSelectEnabled() const { return multiSelect_; }
    void setMultiSelectEnabled(bool enabled) { multiSelect_ = enabled; }
    
    // Scrolling
    float scrollOffset() const { return scrollOffset_; }
    void scrollTo(size_t index);
    void scrollBy(float delta);
    
    // Styling
    Color backgroundColor() const { return backgroundColor_; }
    void setBackgroundColor(const Color& c) { backgroundColor_ = c; }
    
    Color selectedColor() const { return selectedColor_; }
    void setSelectedColor(const Color& c) { selectedColor_ = c; }
    
    Color separatorColor() const { return separatorColor_; }
    void setSeparatorColor(const Color& c) { separatorColor_ = c; }
    
    bool showsSeparators() const { return showsSeparators_; }
    void setShowsSeparators(bool show) { showsSeparators_ = show; }
    
    // Callbacks
    void onCellConfig(CellConfigCallback cb) { onCellConfig_ = cb; }
    void onSelect(SelectionCallback cb) { onSelect_ = cb; }
    
    // Widget overrides
    void render(GpuContext* gpu) override;
    EventResult handleEvent(const Event& event) override;
    Size measure(const Size& available) override;
    
private:
    size_t itemCount_ = 0;
    float rowHeight_ = 32;
    float scrollOffset_ = 0;
    size_t selectedIndex_ = SIZE_MAX;
    bool multiSelect_ = false;
    
    Color backgroundColor_ = Color(30, 30, 35);
    Color selectedColor_ = Color(0, 100, 200);
    Color hoverColor_ = Color(60, 60, 70);
    Color separatorColor_ = Color(50, 50, 55);
    bool showsSeparators_ = true;
    
    size_t hoveredIndex_ = SIZE_MAX;
    
    CellConfigCallback onCellConfig_;
    SelectionCallback onSelect_;
    
    size_t indexAtPoint(float x, float y) const;
    float contentHeight() const { return itemCount_ * rowHeight_; }
};

} // namespace NXRender
