// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file dropdown.h
 * @brief Dropdown/Select widget for option selection
 */

#pragma once

#include "widget.h"
#include "../nxgfx/color.h"
#include <functional>
#include <string>
#include <vector>

namespace NXRender {

/**
 * @brief Dropdown option
 */
struct DropdownOption {
    std::string value;      // Internal value
    std::string label;      // Display label
    bool disabled = false;
    bool separator = false; // Render as separator line
    
    DropdownOption() = default;
    DropdownOption(const std::string& v, const std::string& l = "")
        : value(v), label(l.empty() ? v : l) {}
    
    static DropdownOption Separator() {
        DropdownOption opt;
        opt.separator = true;
        return opt;
    }
};

/**
 * @brief Dropdown widget
 */
class Dropdown : public Widget {
public:
    using SelectCallback = std::function<void(int index, const DropdownOption&)>;
    
    Dropdown();
    explicit Dropdown(const std::vector<DropdownOption>& options);
    ~Dropdown() override = default;
    
    // Options
    const std::vector<DropdownOption>& options() const { return options_; }
    void setOptions(const std::vector<DropdownOption>& options);
    void addOption(const DropdownOption& option);
    void removeOption(int index);
    void clearOptions();
    
    // Selection
    int selectedIndex() const { return selectedIndex_; }
    void setSelectedIndex(int index);
    
    const DropdownOption* selectedOption() const;
    std::string selectedValue() const;
    void selectByValue(const std::string& value);
    
    // Placeholder (when nothing selected)
    const std::string& placeholder() const { return placeholder_; }
    void setPlaceholder(const std::string& p) { placeholder_ = p; }
    
    // Open state
    bool isOpen() const { return isOpen_; }
    void open();
    void close();
    void toggle();
    
    // Search/filter
    bool isSearchable() const { return searchable_; }
    void setSearchable(bool s) { searchable_ = s; }
    
    const std::string& searchText() const { return searchText_; }
    
    // Styling
    float maxDropdownHeight() const { return maxDropdownHeight_; }
    void setMaxDropdownHeight(float h) { maxDropdownHeight_ = h; }
    
    float itemHeight() const { return itemHeight_; }
    void setItemHeight(float h) { itemHeight_ = h; }
    
    Color dropdownBg() const { return dropdownBg_; }
    void setDropdownBg(const Color& c) { dropdownBg_ = c; }
    
    Color hoverBg() const { return hoverBg_; }
    void setHoverBg(const Color& c) { hoverBg_ = c; }
    
    Color selectedBg() const { return selectedBg_; }
    void setSelectedBg(const Color& c) { selectedBg_ = c; }
    
    // Callback
    void onSelect(SelectCallback cb) { onSelect_ = cb; }
    
    // Widget overrides
    void render(GpuContext* gpu) override;
    bool handleEvent(const Event& event) override;
    Size preferredSize() const override;
    
private:
    std::vector<DropdownOption> options_;
    int selectedIndex_ = -1;
    std::string placeholder_ = "Select...";
    bool isOpen_ = false;
    bool searchable_ = false;
    std::string searchText_;
    
    float maxDropdownHeight_ = 200;
    float itemHeight_ = 32;
    int hoveredIndex_ = -1;
    float scrollOffset_ = 0;
    
    Color dropdownBg_ = Color(255, 255, 255);
    Color hoverBg_ = Color(240, 240, 240);
    Color selectedBg_ = Color(0, 122, 255);
    
    SelectCallback onSelect_;
    
    void renderButton(GpuContext* gpu);
    void renderDropdown(GpuContext* gpu);
    void renderOption(GpuContext* gpu, const DropdownOption& opt, int index, float y);
    std::vector<int> filteredIndices() const;
    Rect dropdownRect() const;
};

/**
 * @brief Progress bar widget
 */
class ProgressBar : public Widget {
public:
    ProgressBar();
    ProgressBar(float min, float max, float value = 0);
    ~ProgressBar() override = default;
    
    // Value
    float value() const { return value_; }
    void setValue(float v);
    
    float minValue() const { return minValue_; }
    float maxValue() const { return maxValue_; }
    void setRange(float min, float max);
    
    float progress() const;  // Normalized [0, 1]
    
    // Indeterminate mode (loading spinner)
    bool isIndeterminate() const { return indeterminate_; }
    void setIndeterminate(bool i) { indeterminate_ = i; }
    
    // Styling
    float barHeight() const { return barHeight_; }
    void setBarHeight(float h) { barHeight_ = h; }
    
    float cornerRadius() const { return cornerRadius_; }
    void setCornerRadius(float r) { cornerRadius_ = r; }
    
    Color trackColor() const { return trackColor_; }
    void setTrackColor(const Color& c) { trackColor_ = c; }
    
    Color fillColor() const { return fillColor_; }
    void setFillColor(const Color& c) { fillColor_ = c; }
    
    // Show percentage text
    bool showsPercentage() const { return showsPercentage_; }
    void setShowsPercentage(bool show) { showsPercentage_ = show; }
    
    // Widget overrides
    void render(GpuContext* gpu) override;
    Size preferredSize() const override;
    
private:
    float value_ = 0;
    float minValue_ = 0;
    float maxValue_ = 100;
    bool indeterminate_ = false;
    
    float barHeight_ = 8;
    float cornerRadius_ = 4;
    
    Color trackColor_ = Color(200, 200, 200);
    Color fillColor_ = Color(0, 122, 255);
    
    bool showsPercentage_ = false;
    float indeterminateOffset_ = 0;  // For animation
};

/**
 * @brief Tooltip widget
 */
class Tooltip : public Widget {
public:
    Tooltip();
    explicit Tooltip(const std::string& text);
    ~Tooltip() override = default;
    
    // Text
    const std::string& text() const { return text_; }
    void setText(const std::string& t) { text_ = t; }
    
    // Position relative to target
    enum class Position { Top, Bottom, Left, Right };
    Position position() const { return position_; }
    void setPosition(Position p) { position_ = p; }
    
    // Target widget
    Widget* target() const { return target_; }
    void setTarget(Widget* w) { target_ = w; }
    
    // Show/hide
    void show();
    void hide();
    bool isVisible() const { return visible_; }
    
    // Delay before showing (ms)
    int showDelay() const { return showDelay_; }
    void setShowDelay(int ms) { showDelay_ = ms; }
    
    // Styling
    Color backgroundColor() const { return backgroundColor_; }
    void setBackgroundColor(const Color& c) { backgroundColor_ = c; }
    
    Color textColor() const { return textColor_; }
    void setTextColor(const Color& c) { textColor_ = c; }
    
    float padding() const { return padding_; }
    void setPadding(float p) { padding_ = p; }
    
    // Widget overrides
    void render(GpuContext* gpu) override;
    Size preferredSize() const override;
    
private:
    std::string text_;
    Position position_ = Position::Top;
    Widget* target_ = nullptr;
    bool visible_ = false;
    int showDelay_ = 500;
    
    Color backgroundColor_ = Color(50, 50, 50);
    Color textColor_ = Color(255, 255, 255);
    float padding_ = 8;
    float cornerRadius_ = 4;
    float arrowSize_ = 6;
    
    Rect calculatePosition() const;
    void drawArrow(GpuContext* gpu) const;
};

} // namespace NXRender
