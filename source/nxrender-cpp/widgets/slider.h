// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file slider.h
 * @brief Slider widget for value input
 */

#pragma once

#include "widget.h"
#include "../nxgfx/color.h"
#include <functional>

namespace NXRender {

/**
 * @brief Slider orientation
 */
enum class SliderOrientation {
    Horizontal,
    Vertical
};

/**
 * @brief Slider widget
 */
class Slider : public Widget {
public:
    using ValueChangedCallback = std::function<void(float)>;
    using ValueCommittedCallback = std::function<void(float)>;
    
    Slider();
    explicit Slider(float min, float max, float value = 0);
    ~Slider() override = default;
    
    // Value
    float value() const { return value_; }
    void setValue(float value);
    
    float minValue() const { return minValue_; }
    float maxValue() const { return maxValue_; }
    void setRange(float min, float max);
    
    float step() const { return step_; }
    void setStep(float step) { step_ = step; }
    
    // Orientation
    SliderOrientation orientation() const { return orientation_; }
    void setOrientation(SliderOrientation o) { orientation_ = o; }
    
    // Styling
    float trackHeight() const { return trackHeight_; }
    void setTrackHeight(float h) { trackHeight_ = h; }
    
    float thumbRadius() const { return thumbRadius_; }
    void setThumbRadius(float r) { thumbRadius_ = r; }
    
    Color trackColor() const { return trackColor_; }
    void setTrackColor(const Color& c) { trackColor_ = c; }
    
    Color trackFilledColor() const { return trackFilledColor_; }
    void setTrackFilledColor(const Color& c) { trackFilledColor_ = c; }
    
    Color thumbColor() const { return thumbColor_; }
    void setThumbColor(const Color& c) { thumbColor_ = c; }
    
    // Show value tooltip while dragging
    bool showsValueTooltip() const { return showsValueTooltip_; }
    void setShowsValueTooltip(bool show) { showsValueTooltip_ = show; }
    
    // Callbacks
    void onValueChanged(ValueChangedCallback cb) { onValueChanged_ = cb; }
    void onValueCommitted(ValueCommittedCallback cb) { onValueCommitted_ = cb; }
    
    // Widget overrides
    void render(GpuContext* gpu) override;
    bool handleEvent(const Event& event) override;
    Size preferredSize() const override;
    
private:
    float value_ = 0;
    float minValue_ = 0;
    float maxValue_ = 100;
    float step_ = 0;  // 0 = continuous
    
    SliderOrientation orientation_ = SliderOrientation::Horizontal;
    float trackHeight_ = 4;
    float thumbRadius_ = 8;
    
    Color trackColor_ = Color(200, 200, 200);
    Color trackFilledColor_ = Color(0, 122, 255);
    Color thumbColor_ = Color(255, 255, 255);
    
    bool showsValueTooltip_ = true;
    bool isDragging_ = false;
    
    ValueChangedCallback onValueChanged_;
    ValueCommittedCallback onValueCommitted_;
    
    float normalizedValue() const;
    void setValueFromPosition(float pos);
    Rect thumbRect() const;
    Rect trackRect() const;
};

} // namespace NXRender
