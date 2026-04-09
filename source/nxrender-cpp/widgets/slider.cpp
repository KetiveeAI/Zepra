// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "widgets/slider.h"
#include "nxgfx/context.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace NXRender {

Slider::Slider() {
    backgroundColor_ = Color::transparent();
}

Slider::Slider(float min, float max, float value)
    : minValue_(min), maxValue_(max), value_(std::clamp(value, min, max)) {
    backgroundColor_ = Color::transparent();
}

void Slider::setValue(float value) {
    float clamped = std::clamp(value, minValue_, maxValue_);

    // Snap to step
    if (step_ > 0) {
        float steps = std::round((clamped - minValue_) / step_);
        clamped = minValue_ + steps * step_;
        clamped = std::clamp(clamped, minValue_, maxValue_);
    }

    if (std::abs(clamped - value_) > 0.0001f) {
        value_ = clamped;
        if (onValueChanged_) onValueChanged_(value_);
        invalidate();
    }
}

void Slider::setRange(float min, float max) {
    if (min >= max) return;
    minValue_ = min;
    maxValue_ = max;
    value_ = std::clamp(value_, min, max);
    invalidate();
}

float Slider::normalizedValue() const {
    float range = maxValue_ - minValue_;
    if (range <= 0.0f) return 0.0f;
    return (value_ - minValue_) / range;
}

Rect Slider::trackRect() const {
    const Rect& b = bounds();
    if (orientation_ == SliderOrientation::Horizontal) {
        float trackY = b.y + (b.height - trackHeight_) * 0.5f;
        float inset = thumbRadius_;
        return Rect(b.x + inset, trackY, b.width - inset * 2, trackHeight_);
    } else {
        float trackX = b.x + (b.width - trackHeight_) * 0.5f;
        float inset = thumbRadius_;
        return Rect(trackX, b.y + inset, trackHeight_, b.height - inset * 2);
    }
}

Rect Slider::thumbRect() const {
    Rect track = trackRect();
    float norm = normalizedValue();

    if (orientation_ == SliderOrientation::Horizontal) {
        float thumbX = track.x + norm * track.width - thumbRadius_;
        float thumbY = track.y + track.height * 0.5f - thumbRadius_;
        return Rect(thumbX, thumbY, thumbRadius_ * 2, thumbRadius_ * 2);
    } else {
        float thumbX = track.x + track.width * 0.5f - thumbRadius_;
        // Vertical: 0 at bottom, 1 at top
        float thumbY = track.y + (1.0f - norm) * track.height - thumbRadius_;
        return Rect(thumbX, thumbY, thumbRadius_ * 2, thumbRadius_ * 2);
    }
}

void Slider::setValueFromPosition(float pos) {
    Rect track = trackRect();
    float norm;

    if (orientation_ == SliderOrientation::Horizontal) {
        norm = (pos - track.x) / track.width;
    } else {
        norm = 1.0f - (pos - track.y) / track.height;
    }

    norm = std::clamp(norm, 0.0f, 1.0f);
    float newValue = minValue_ + norm * (maxValue_ - minValue_);
    setValue(newValue);
}

Size Slider::preferredSize() const {
    if (orientation_ == SliderOrientation::Horizontal) {
        return Size(200.0f, std::max(thumbRadius_ * 2 + 8, 32.0f));
    } else {
        return Size(std::max(thumbRadius_ * 2 + 8, 32.0f), 200.0f);
    }
}

bool Slider::handleEvent(const Event& event) {
    if (!isEnabled()) return false;

    if (event.type == EventType::MouseDown) {
        Rect thumb = thumbRect();
        float dx = event.mouseX - (thumb.x + thumb.width * 0.5f);
        float dy = event.mouseY - (thumb.y + thumb.height * 0.5f);
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist <= thumbRadius_ * 1.5f) {
            isDragging_ = true;
            return true;
        }

        // Click on track — jump to position
        Rect track = trackRect();
        if (track.contains(event.mouseX, event.mouseY)) {
            float pos = (orientation_ == SliderOrientation::Horizontal)
                        ? event.mouseX : event.mouseY;
            setValueFromPosition(pos);
            isDragging_ = true;
            return true;
        }
    }

    if (event.type == EventType::MouseMove && isDragging_) {
        float pos = (orientation_ == SliderOrientation::Horizontal)
                    ? event.mouseX : event.mouseY;
        setValueFromPosition(pos);
        return true;
    }

    if (event.type == EventType::MouseUp && isDragging_) {
        isDragging_ = false;
        if (onValueCommitted_) onValueCommitted_(value_);
        return true;
    }

    if (event.type == EventType::KeyDown) {
        float delta = (step_ > 0) ? step_ : (maxValue_ - minValue_) * 0.01f;
        if (event.keyCode == KeyCode::Left || event.keyCode == KeyCode::Down) {
            setValue(value_ - delta);
            return true;
        }
        if (event.keyCode == KeyCode::Right || event.keyCode == KeyCode::Up) {
            setValue(value_ + delta);
            return true;
        }
        if (event.keyCode == KeyCode::Home) {
            setValue(minValue_);
            return true;
        }
        if (event.keyCode == KeyCode::End) {
            setValue(maxValue_);
            return true;
        }
    }

    return false;
}

void Slider::render(GpuContext* gpu) {
    if (!gpu || !isVisible()) return;

    Rect track = trackRect();
    float norm = normalizedValue();

    // Track background
    gpu->fillRoundedRect(track, trackColor_, trackHeight_ * 0.5f);

    // Filled portion
    Rect filled;
    if (orientation_ == SliderOrientation::Horizontal) {
        filled = Rect(track.x, track.y, track.width * norm, track.height);
    } else {
        float fillHeight = track.height * norm;
        filled = Rect(track.x, track.y + track.height - fillHeight, track.width, fillHeight);
    }
    if (filled.width > 0 && filled.height > 0) {
        gpu->fillRoundedRect(filled, trackFilledColor_, trackHeight_ * 0.5f);
    }

    // Thumb
    Rect thumb = thumbRect();
    float thumbCX = thumb.x + thumb.width * 0.5f;
    float thumbCY = thumb.y + thumb.height * 0.5f;

    // Shadow under thumb
    gpu->fillCircle(thumbCX, thumbCY + 1.0f, thumbRadius_ + 1.0f, Color(0, 0, 0, 40));

    // Thumb circle
    Color tc = thumbColor_;
    if (isDragging_) {
        tc = trackFilledColor_;
    }
    gpu->fillCircle(thumbCX, thumbCY, thumbRadius_, tc);
    gpu->strokeCircle(thumbCX, thumbCY, thumbRadius_, trackFilledColor_, 2.0f);

    // Value tooltip while dragging
    if (isDragging_ && showsValueTooltip_) {
        std::ostringstream oss;
        if (step_ >= 1.0f) {
            oss << static_cast<int>(value_);
        } else {
            oss << std::fixed << std::setprecision(1) << value_;
        }
        std::string valueStr = oss.str();

        float tipWidth = static_cast<float>(valueStr.size()) * 8.0f + 12;
        float tipHeight = 22.0f;
        float tipX, tipY;

        if (orientation_ == SliderOrientation::Horizontal) {
            tipX = thumbCX - tipWidth * 0.5f;
            tipY = thumbCY - thumbRadius_ - tipHeight - 6;
        } else {
            tipX = thumbCX + thumbRadius_ + 6;
            tipY = thumbCY - tipHeight * 0.5f;
        }

        Rect tipRect(tipX, tipY, tipWidth, tipHeight);
        gpu->fillRoundedRect(tipRect, Color(50, 50, 50, 220), 4.0f);
        gpu->drawText(valueStr, tipX + 6, tipY + 4, Color(255, 255, 255), 12.0f);
    }
}

} // namespace NXRender
