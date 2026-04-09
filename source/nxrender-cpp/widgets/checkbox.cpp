// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "widgets/checkbox.h"
#include "nxgfx/context.h"
#include <cmath>

namespace NXRender {

// ==================================================================
// Checkbox
// ==================================================================

Checkbox::Checkbox() {
    backgroundColor_ = Color::transparent();
}

Checkbox::Checkbox(const std::string& label) : label_(label) {
    backgroundColor_ = Color::transparent();
}

Checkbox::Checkbox(const std::string& label, bool checked) : label_(label) {
    backgroundColor_ = Color::transparent();
    if (checked) state_ = CheckboxState::Checked;
}

void Checkbox::setChecked(bool checked) {
    CheckboxState newState = checked ? CheckboxState::Checked : CheckboxState::Unchecked;
    if (newState != state_) {
        state_ = newState;
        invalidate();
    }
}

void Checkbox::setState(CheckboxState state) {
    if (state != state_) {
        state_ = state;
        invalidate();
    }
}

void Checkbox::toggle() {
    if (triState_) {
        switch (state_) {
            case CheckboxState::Unchecked:
                state_ = CheckboxState::Checked;
                break;
            case CheckboxState::Checked:
                state_ = CheckboxState::Indeterminate;
                break;
            case CheckboxState::Indeterminate:
                state_ = CheckboxState::Unchecked;
                break;
        }
    } else {
        state_ = (state_ == CheckboxState::Checked)
                 ? CheckboxState::Unchecked
                 : CheckboxState::Checked;
    }

    if (onChanged_) {
        onChanged_(state_ == CheckboxState::Checked);
    }
    invalidate();
}

Rect Checkbox::boxRect() const {
    const Rect& b = bounds();
    float centerY = b.y + (b.height - boxSize_) * 0.5f;
    return Rect(b.x, centerY, boxSize_, boxSize_);
}

Size Checkbox::preferredSize() const {
    float textWidth = 0;
    if (!label_.empty()) {
        textWidth = labelGap_ + static_cast<float>(label_.size()) * 8.0f;
    }
    return Size(boxSize_ + textWidth, std::max(boxSize_, 20.0f));
}

bool Checkbox::handleEvent(const Event& event) {
    if (!isEnabled()) return false;

    if (event.type == EventType::MouseEnter) {
        isHovered_ = true;
        invalidate();
        return true;
    }
    if (event.type == EventType::MouseLeave) {
        isHovered_ = false;
        invalidate();
        return true;
    }
    if (event.type == EventType::MouseDown) {
        toggle();
        return true;
    }
    return false;
}

void Checkbox::drawCheckmark(GpuContext* gpu, const Rect& box) {
    // Draw a checkmark (two lines forming a check)
    float pad = box.width * 0.2f;
    float x0 = box.x + pad;
    float y0 = box.y + box.height * 0.5f;
    float x1 = box.x + box.width * 0.4f;
    float y1 = box.y + box.height - pad;
    float x2 = box.x + box.width - pad;
    float y2 = box.y + pad;

    float lineWidth = std::max(2.0f, box.width * 0.12f);
    gpu->drawLine(x0, y0, x1, y1, checkColor_, lineWidth);
    gpu->drawLine(x1, y1, x2, y2, checkColor_, lineWidth);
}

void Checkbox::drawIndeterminate(GpuContext* gpu, const Rect& box) {
    float pad = box.width * 0.25f;
    float lineY = box.y + box.height * 0.5f;
    float lineWidth = std::max(2.0f, box.width * 0.12f);
    gpu->drawLine(box.x + pad, lineY, box.x + box.width - pad, lineY, checkColor_, lineWidth);
}

void Checkbox::render(GpuContext* gpu) {
    if (!gpu || !isVisible()) return;

    Rect box = boxRect();

    // Box background
    Color bgColor;
    switch (state_) {
        case CheckboxState::Checked:
        case CheckboxState::Indeterminate:
            bgColor = checkedBoxColor_;
            break;
        default:
            bgColor = boxColor_;
            break;
    }

    if (isHovered_) {
        // Lighten on hover
        bgColor.r = std::min(255, bgColor.r + 20);
        bgColor.g = std::min(255, bgColor.g + 20);
        bgColor.b = std::min(255, bgColor.b + 20);
    }

    gpu->fillRoundedRect(box, bgColor, cornerRadius_);

    // Border
    Color borderColor = (state_ != CheckboxState::Unchecked)
                        ? checkedBoxColor_
                        : Color(180, 180, 180);
    gpu->strokeRoundedRect(box, borderColor, cornerRadius_, 1.5f);

    // Check/indeterminate mark
    if (state_ == CheckboxState::Checked) {
        drawCheckmark(gpu, box);
    } else if (state_ == CheckboxState::Indeterminate) {
        drawIndeterminate(gpu, box);
    }

    // Label
    if (!label_.empty()) {
        float textX = box.x + box.width + labelGap_;
        float textY = bounds().y + (bounds().height - 14.0f) * 0.5f;
        Color textColor = isEnabled() ? Color(0x212121) : Color(0x9E9E9E);
        gpu->drawText(label_, textX, textY, textColor, 14.0f);
    }
}

// ==================================================================
// RadioButton
// ==================================================================

RadioButton::RadioButton() {
    backgroundColor_ = Color::transparent();
}

RadioButton::RadioButton(const std::string& label) : label_(label) {
    backgroundColor_ = Color::transparent();
}

RadioButton::RadioButton(const std::string& label, bool selected)
    : label_(label), selected_(selected) {
    backgroundColor_ = Color::transparent();
}

void RadioButton::setSelected(bool selected) {
    if (selected != selected_) {
        selected_ = selected;
        if (onSelected_) onSelected_(selected_);
        invalidate();
    }
}

Size RadioButton::preferredSize() const {
    float diameter = circleRadius_ * 2.0f;
    float textWidth = 0;
    if (!label_.empty()) {
        textWidth = labelGap_ + static_cast<float>(label_.size()) * 8.0f;
    }
    return Size(diameter + textWidth, std::max(diameter, 20.0f));
}

bool RadioButton::handleEvent(const Event& event) {
    if (!isEnabled()) return false;

    if (event.type == EventType::MouseEnter) {
        isHovered_ = true;
        invalidate();
        return true;
    }
    if (event.type == EventType::MouseLeave) {
        isHovered_ = false;
        invalidate();
        return true;
    }
    if (event.type == EventType::MouseDown) {
        if (!selected_) {
            setSelected(true);
        }
        return true;
    }
    return false;
}

void RadioButton::render(GpuContext* gpu) {
    if (!gpu || !isVisible()) return;

    const Rect& b = bounds();
    float cx = b.x + circleRadius_;
    float cy = b.y + b.height * 0.5f;

    // Outer circle
    Color outerColor = selected_ ? selectedCircleColor_ : circleColor_;
    if (isHovered_) {
        outerColor.r = std::min(255, outerColor.r + 15);
        outerColor.g = std::min(255, outerColor.g + 15);
        outerColor.b = std::min(255, outerColor.b + 15);
    }

    gpu->fillCircle(cx, cy, circleRadius_, Color(255, 255, 255));
    gpu->strokeCircle(cx, cy, circleRadius_, outerColor, 1.5f);

    // Inner dot (when selected)
    if (selected_) {
        gpu->fillCircle(cx, cy, dotRadius_, selectedCircleColor_);
    }

    // Label
    if (!label_.empty()) {
        float textX = b.x + circleRadius_ * 2.0f + labelGap_;
        float textY = b.y + (b.height - 14.0f) * 0.5f;
        Color textColor = isEnabled() ? Color(0x212121) : Color(0x9E9E9E);
        gpu->drawText(label_, textX, textY, textColor, 14.0f);
    }
}

} // namespace NXRender
