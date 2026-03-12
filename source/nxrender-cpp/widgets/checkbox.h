// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file checkbox.h
 * @brief Checkbox widget for boolean input
 */

#pragma once

#include "widget.h"
#include "../nxgfx/color.h"
#include <functional>
#include <string>

namespace NXRender {

/**
 * @brief Checkbox state
 */
enum class CheckboxState {
    Unchecked,
    Checked,
    Indeterminate  // For tri-state checkboxes
};

/**
 * @brief Checkbox widget
 */
class Checkbox : public Widget {
public:
    using ChangedCallback = std::function<void(bool)>;
    
    Checkbox();
    explicit Checkbox(const std::string& label);
    Checkbox(const std::string& label, bool checked);
    ~Checkbox() override = default;
    
    // State
    bool isChecked() const { return state_ == CheckboxState::Checked; }
    void setChecked(bool checked);
    
    CheckboxState state() const { return state_; }
    void setState(CheckboxState state);
    
    // Label
    const std::string& label() const { return label_; }
    void setLabel(const std::string& label) { label_ = label; }
    
    // Tri-state support
    bool isTriState() const { return triState_; }
    void setTriState(bool enabled) { triState_ = enabled; }
    
    // Styling
    float boxSize() const { return boxSize_; }
    void setBoxSize(float size) { boxSize_ = size; }
    
    float cornerRadius() const { return cornerRadius_; }
    void setCornerRadius(float r) { cornerRadius_ = r; }
    
    Color checkColor() const { return checkColor_; }
    void setCheckColor(const Color& c) { checkColor_ = c; }
    
    Color boxColor() const { return boxColor_; }
    void setBoxColor(const Color& c) { boxColor_ = c; }
    
    Color checkedBoxColor() const { return checkedBoxColor_; }
    void setCheckedBoxColor(const Color& c) { checkedBoxColor_ = c; }
    
    // Callback
    void onChanged(ChangedCallback cb) { onChanged_ = cb; }
    
    // Widget overrides
    void render(GpuContext* gpu) override;
    bool handleEvent(const Event& event) override;
    Size preferredSize() const override;
    
private:
    std::string label_;
    CheckboxState state_ = CheckboxState::Unchecked;
    bool triState_ = false;
    
    float boxSize_ = 18;
    float cornerRadius_ = 3;
    float labelGap_ = 8;  // Gap between box and label
    
    Color checkColor_ = Color(255, 255, 255);
    Color boxColor_ = Color(200, 200, 200);
    Color checkedBoxColor_ = Color(0, 122, 255);
    
    bool isHovered_ = false;
    
    ChangedCallback onChanged_;
    
    void toggle();
    Rect boxRect() const;
    void drawCheckmark(GpuContext* gpu, const Rect& box);
    void drawIndeterminate(GpuContext* gpu, const Rect& box);
};

/**
 * @brief Radio button widget
 */
class RadioButton : public Widget {
public:
    using SelectedCallback = std::function<void(bool)>;
    
    RadioButton();
    explicit RadioButton(const std::string& label);
    RadioButton(const std::string& label, bool selected);
    ~RadioButton() override = default;
    
    // State
    bool isSelected() const { return selected_; }
    void setSelected(bool selected);
    
    // Label
    const std::string& label() const { return label_; }
    void setLabel(const std::string& label) { label_ = label; }
    
    // Group (for mutual exclusion)
    int group() const { return group_; }
    void setGroup(int group) { group_ = group; }
    
    // Styling
    float circleRadius() const { return circleRadius_; }
    void setCircleRadius(float r) { circleRadius_ = r; }
    
    Color dotColor() const { return dotColor_; }
    void setDotColor(const Color& c) { dotColor_ = c; }
    
    Color circleColor() const { return circleColor_; }
    void setCircleColor(const Color& c) { circleColor_ = c; }
    
    Color selectedCircleColor() const { return selectedCircleColor_; }
    void setSelectedCircleColor(const Color& c) { selectedCircleColor_ = c; }
    
    // Callback
    void onSelected(SelectedCallback cb) { onSelected_ = cb; }
    
    // Widget overrides
    void render(GpuContext* gpu) override;
    bool handleEvent(const Event& event) override;
    Size preferredSize() const override;
    
private:
    std::string label_;
    bool selected_ = false;
    int group_ = 0;
    
    float circleRadius_ = 9;
    float dotRadius_ = 5;
    float labelGap_ = 8;
    
    Color dotColor_ = Color(255, 255, 255);
    Color circleColor_ = Color(200, 200, 200);
    Color selectedCircleColor_ = Color(0, 122, 255);
    
    bool isHovered_ = false;
    
    SelectedCallback onSelected_;
};

} // namespace NXRender
