// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "widgets/form_controls.h"
#include "nxgfx/context.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <cctype>

namespace NXRender {

// ==================================================================
// FormControl
// ==================================================================

FormControl::FormControl() {}

void FormControl::notifyChange() {
    if (onChange_) onChange_(this);
}

// ==================================================================
// InputWidget
// ==================================================================

InputWidget::InputWidget(InputType type) : type_(type) {}

void InputWidget::setValue(const std::string& val) {
    if (readOnly_) return;
    value_ = val;
    if (maxLength_ >= 0 && static_cast<int>(value_.size()) > maxLength_) {
        value_ = value_.substr(0, maxLength_);
    }
    cursorPos_ = static_cast<int>(value_.size());
    notifyChange();
}

double InputWidget::numericValue() const {
    if (type_ == InputType::Number || type_ == InputType::Range) {
        return std::strtod(value_.c_str(), nullptr);
    }
    return 0;
}

void InputWidget::setChecked(bool c) {
    checked_ = c;
    notifyChange();
}

void InputWidget::setSelectionRange(int start, int end) {
    selectionStart_ = std::clamp(start, 0, static_cast<int>(value_.size()));
    selectionEnd_ = std::clamp(end, 0, static_cast<int>(value_.size()));
}

bool InputWidget::checkValidity() const {
    if (!customValidity_.empty()) return false;
    if (required_ && value_.empty()) return false;
    if (minLength_ >= 0 && static_cast<int>(value_.size()) < minLength_) return false;
    if (maxLength_ >= 0 && static_cast<int>(value_.size()) > maxLength_) return false;
    if (type_ == InputType::Email && !value_.empty()) {
        return value_.find('@') != std::string::npos;
    }
    if (type_ == InputType::URL && !value_.empty()) {
        return value_.find("://") != std::string::npos;
    }
    return true;
}

Size InputWidget::measure(const Size& available) {
    float height = 30;
    float width = std::min(200.0f, available.width);

    switch (type_) {
        case InputType::Checkbox:
        case InputType::Radio:
            return Size(20, 20);
        case InputType::Color:
            return Size(44, 36);
        case InputType::Range:
            return Size(std::min(160.0f, available.width), 24);
        case InputType::Hidden:
            return Size(0, 0);
        default:
            return Size(width, height);
    }
}

void InputWidget::render(GpuContext* ctx) {
    if (!ctx) return;
    Rect b = bounds();

    switch (type_) {
        case InputType::Checkbox: {
            Color borderColor = disabled_ ? Color(0xCC, 0xCC, 0xCC) : Color(0x66, 0x66, 0x66);
            Color fillColor = checked_ ? Color(0x42, 0x85, 0xF4) : Color::white();
            ctx->fillRoundedRect(b, fillColor, 3);
            // Border via slightly inset rect
            if (checked_) {
                float cx = b.x + b.width / 2;
                float cy = b.y + b.height / 2;
                ctx->drawLine(cx - 4, cy, cx - 1, cy + 3, Color::white(), 2);
                ctx->drawLine(cx - 1, cy + 3, cx + 5, cy - 3, Color::white(), 2);
            }
            (void)borderColor;
            break;
        }
        case InputType::Radio: {
            float cx = b.x + b.width / 2, cy = b.y + b.height / 2;
            float r = b.width / 2;
            ctx->fillCircle(cx, cy, r, Color::white());
            if (checked_) {
                ctx->fillCircle(cx, cy, r * 0.5f, Color(0x42, 0x85, 0xF4));
            }
            break;
        }
        case InputType::Range: {
            float trackY = b.y + b.height / 2;
            double normalized = (max_ > min_) ? (numericValue() - min_) / (max_ - min_) : 0;
            normalized = std::clamp(normalized, 0.0, 1.0);
            float thumbX = b.x + static_cast<float>(normalized) * b.width;
            // Track
            Rect trackRect(b.x, trackY - 2, b.width, 4);
            ctx->fillRoundedRect(trackRect, Color(0xDD, 0xDD, 0xDD), 2);
            // Filled portion
            Rect fillRect(b.x, trackY - 2, thumbX - b.x, 4);
            ctx->fillRoundedRect(fillRect, Color(0x42, 0x85, 0xF4), 2);
            // Thumb
            ctx->fillCircle(thumbX, trackY, 8, Color::white());
            ctx->drawLine(thumbX - 8, trackY, thumbX + 8, trackY, Color(0x42, 0x85, 0xF4), 2);
            break;
        }
        case InputType::Hidden:
            break;
        default: {
            Color bgColor = disabled_ ? Color(0xF0, 0xF0, 0xF0) : Color::white();
            ctx->fillRoundedRect(b, bgColor, 4);

            float textX = b.x + 8;
            float textY = b.y + b.height / 2 + 5;

            if (value_.empty() && !placeholder_.empty()) {
                ctx->drawText(placeholder_, textX, textY, Color(0x99, 0x99, 0x99), 14);
            } else {
                std::string displayValue = value_;
                if (type_ == InputType::Password) {
                    displayValue = std::string(value_.size(), '*');
                }
                Color textColor = disabled_ ? Color(0x99, 0x99, 0x99) : Color(0x33, 0x33, 0x33);
                ctx->drawText(displayValue, textX, textY, textColor, 14);
            }

            if (isFocused() && cursorVisible_ && !readOnly_) {
                float cursorX = textX + cursorPos_ * 8.4f;
                ctx->drawLine(cursorX, b.y + 6, cursorX, b.y + b.height - 6,
                              Color(0x33, 0x33, 0x33), 1);
            }
            break;
        }
    }
}

EventResult InputWidget::onMouseDown(float x, float y, MouseButton /*button*/) {
    if (disabled_) return EventResult::Ignored;
    if (type_ == InputType::Checkbox) { setChecked(!checked_); return EventResult::Handled; }
    if (type_ == InputType::Radio) { setChecked(true); return EventResult::Handled; }
    if (type_ == InputType::Range) {
        Rect b = bounds();
        float normalized = (x - b.x) / b.width;
        normalized = std::clamp(normalized, 0.0f, 1.0f);
        double val = min_ + normalized * (max_ - min_);
        val = std::round(val / step_) * step_;
        value_ = std::to_string(val);
        notifyChange();
        return EventResult::Handled;
    }
    Rect b = bounds();
    float relX = x - b.x - 8;
    cursorPos_ = std::clamp(static_cast<int>(relX / 8.4f), 0, static_cast<int>(value_.size()));
    (void)y;
    return EventResult::Handled;
}

EventResult InputWidget::onKeyDown(KeyCode key, Modifiers /*mods*/) {
    if (disabled_ || readOnly_) return EventResult::Ignored;
    if (type_ == InputType::Checkbox || type_ == InputType::Radio) {
        if (key == KeyCode::Space || key == KeyCode::Enter) {
            if (type_ == InputType::Checkbox) setChecked(!checked_);
            else setChecked(true);
            return EventResult::Handled;
        }
        return EventResult::Ignored;
    }
    if (key == KeyCode::Backspace && cursorPos_ > 0) {
        value_.erase(cursorPos_ - 1, 1); cursorPos_--; notifyChange();
        return EventResult::Handled;
    }
    if (key == KeyCode::Delete && cursorPos_ < static_cast<int>(value_.size())) {
        value_.erase(cursorPos_, 1); notifyChange();
        return EventResult::Handled;
    }
    if (key == KeyCode::Left && cursorPos_ > 0) { cursorPos_--; return EventResult::Handled; }
    if (key == KeyCode::Right && cursorPos_ < static_cast<int>(value_.size())) { cursorPos_++; return EventResult::Handled; }
    if (key == KeyCode::Home) { cursorPos_ = 0; return EventResult::Handled; }
    if (key == KeyCode::End) { cursorPos_ = static_cast<int>(value_.size()); return EventResult::Handled; }
    return EventResult::Ignored;
}

EventResult InputWidget::onTextInput(const std::string& text) {
    if (disabled_ || readOnly_) return EventResult::Ignored;
    if (type_ == InputType::Checkbox || type_ == InputType::Radio) return EventResult::Ignored;
    for (char ch : text) {
        if (std::isprint(ch)) {
            if (maxLength_ < 0 || static_cast<int>(value_.size()) < maxLength_) {
                value_.insert(cursorPos_, 1, ch);
                cursorPos_++;
            }
        }
    }
    notifyChange();
    return EventResult::Handled;
}

// ==================================================================
// TextareaWidget
// ==================================================================

TextareaWidget::TextareaWidget() {}

void TextareaWidget::setSelectionRange(int start, int end) {
    selectionStart_ = std::clamp(start, 0, static_cast<int>(value_.size()));
    selectionEnd_ = std::clamp(end, 0, static_cast<int>(value_.size()));
}

Size TextareaWidget::measure(const Size& available) {
    float charW = 8.4f;
    float lineH = 18;
    float width = std::min(cols_ * charW + 16.0f, available.width);
    float height = rows_ * lineH + 12;
    return Size(width, height);
}

void TextareaWidget::render(GpuContext* ctx) {
    if (!ctx) return;
    Rect b = bounds();
    Color bgColor = disabled_ ? Color(0xF0, 0xF0, 0xF0) : Color::white();
    ctx->fillRoundedRect(b, bgColor, 4);

    float textX = b.x + 8;
    float textY = b.y + 16;
    float lineH = 18;

    if (value_.empty() && !placeholder_.empty()) {
        ctx->drawText(placeholder_, textX, textY, Color(0x99, 0x99, 0x99), 14);
    } else {
        std::istringstream stream(value_);
        std::string line;
        int lineIdx = 0;
        Color textColor = disabled_ ? Color(0x99, 0x99, 0x99) : Color(0x33, 0x33, 0x33);
        while (std::getline(stream, line)) {
            if (lineIdx >= scrollY_ && textY < b.y + b.height - 6) {
                ctx->drawText(line, textX, textY, textColor, 14);
            }
            textY += lineH;
            lineIdx++;
        }
    }
}

EventResult TextareaWidget::onKeyDown(KeyCode key, Modifiers /*mods*/) {
    if (disabled_ || readOnly_) return EventResult::Ignored;
    if (key == KeyCode::Enter) {
        value_.insert(selectionStart_, "\n");
        selectionStart_++;
        selectionEnd_ = selectionStart_;
        notifyChange();
        return EventResult::Handled;
    }
    if (key == KeyCode::Backspace && selectionStart_ > 0) {
        value_.erase(selectionStart_ - 1, 1);
        selectionStart_--;
        selectionEnd_ = selectionStart_;
        notifyChange();
        return EventResult::Handled;
    }
    return EventResult::Ignored;
}

EventResult TextareaWidget::onTextInput(const std::string& text) {
    if (disabled_ || readOnly_) return EventResult::Ignored;
    for (char ch : text) {
        if (std::isprint(ch)) {
            if (maxLength_ < 0 || static_cast<int>(value_.size()) < maxLength_) {
                value_.insert(selectionStart_, 1, ch);
                selectionStart_++;
                selectionEnd_ = selectionStart_;
            }
        }
    }
    notifyChange();
    return EventResult::Handled;
}

EventResult TextareaWidget::onMouseDown(float /*x*/, float /*y*/, MouseButton /*button*/) {
    if (disabled_) return EventResult::Ignored;
    setFocused(true);
    return EventResult::Handled;
}

// ==================================================================
// SelectWidget
// ==================================================================

SelectWidget::SelectWidget() {}

void SelectWidget::addOption(const SelectOption& opt) { flatOptions_.push_back(opt); }

void SelectWidget::addOptGroup(const SelectOptGroup& group) {
    optGroups_.push_back(group);
    for (const auto& opt : group.options) flatOptions_.push_back(opt);
}

void SelectWidget::clearOptions() { flatOptions_.clear(); optGroups_.clear(); }

void SelectWidget::setSelectedIndex(int index) {
    for (size_t i = 0; i < flatOptions_.size(); i++)
        flatOptions_[i].selected = (static_cast<int>(i) == index);
    notifyChange();
}

int SelectWidget::selectedIndex() const {
    for (size_t i = 0; i < flatOptions_.size(); i++)
        if (flatOptions_[i].selected) return static_cast<int>(i);
    return -1;
}

std::string SelectWidget::selectedValue() const {
    int idx = selectedIndex();
    return (idx >= 0 && idx < static_cast<int>(flatOptions_.size())) ? flatOptions_[idx].value : "";
}

std::vector<int> SelectWidget::selectedIndices() const {
    std::vector<int> indices;
    for (size_t i = 0; i < flatOptions_.size(); i++)
        if (flatOptions_[i].selected) indices.push_back(static_cast<int>(i));
    return indices;
}

Size SelectWidget::measure(const Size& available) {
    float height = 30;
    if (multiple_ && visibleSize_ > 1) height = visibleSize_ * 22 + 8;
    return Size(std::min(200.0f, available.width), height);
}

void SelectWidget::render(GpuContext* ctx) {
    if (!ctx) return;
    Rect b = bounds();
    Color bgColor = disabled_ ? Color(0xF0, 0xF0, 0xF0) : Color::white();
    ctx->fillRoundedRect(b, bgColor, 4);

    if (!multiple_ || visibleSize_ <= 1) {
        int idx = selectedIndex();
        std::string label = (idx >= 0) ? flatOptions_[idx].label : "";
        if (label.empty() && idx >= 0) label = flatOptions_[idx].value;
        ctx->drawText(label, b.x + 8, b.y + b.height / 2 + 5, Color(0x33, 0x33, 0x33), 14);
        // Dropdown arrow
        float arrowX = b.x + b.width - 20;
        float arrowY = b.y + b.height / 2;
        ctx->drawLine(arrowX - 4, arrowY - 2, arrowX, arrowY + 3, Color(0x66, 0x66, 0x66), 1.5f);
        ctx->drawLine(arrowX, arrowY + 3, arrowX + 4, arrowY - 2, Color(0x66, 0x66, 0x66), 1.5f);
    } else {
        float y = b.y + 4;
        for (size_t i = 0; i < flatOptions_.size() && y < b.y + b.height - 4; i++) {
            if (flatOptions_[i].selected) {
                ctx->fillRect(Rect(b.x + 2, y - 2, b.width - 4, 20), Color(0x42, 0x85, 0xF4));
                ctx->drawText(flatOptions_[i].label, b.x + 8, y + 12, Color::white(), 13);
            } else {
                ctx->drawText(flatOptions_[i].label, b.x + 8, y + 12, Color(0x33, 0x33, 0x33), 13);
            }
            y += 22;
        }
    }
}

EventResult SelectWidget::onMouseDown(float x, float y, MouseButton /*button*/) {
    if (disabled_) return EventResult::Ignored;
    Rect b = bounds();
    if (multiple_ && visibleSize_ > 1) {
        float relY = y - b.y - 4;
        int index = static_cast<int>(relY / 22 + scrollOffset_);
        if (index >= 0 && index < static_cast<int>(flatOptions_.size())) {
            if (!flatOptions_[index].disabled) {
                flatOptions_[index].selected = !flatOptions_[index].selected;
                notifyChange();
            }
        }
    } else {
        dropdownOpen_ = !dropdownOpen_;
    }
    (void)x;
    return EventResult::Handled;
}

EventResult SelectWidget::onKeyDown(KeyCode key, Modifiers /*mods*/) {
    if (disabled_) return EventResult::Ignored;
    int idx = selectedIndex();
    if (key == KeyCode::Down && idx < static_cast<int>(flatOptions_.size()) - 1) {
        setSelectedIndex(idx + 1); return EventResult::Handled;
    }
    if (key == KeyCode::Up && idx > 0) {
        setSelectedIndex(idx - 1); return EventResult::Handled;
    }
    return EventResult::Ignored;
}

// ==================================================================
// ProgressWidget
// ==================================================================

ProgressWidget::ProgressWidget() {}

Size ProgressWidget::measure(const Size& available) {
    return Size(std::min(200.0f, available.width), 20);
}

void ProgressWidget::render(GpuContext* ctx) {
    if (!ctx) return;
    Rect b = bounds();
    ctx->fillRoundedRect(b, Color(0xE0, 0xE0, 0xE0), 4);
    if (indeterminate()) {
        float barWidth = b.width * 0.3f;
        float barX = b.x + std::fmod(animOffset_, b.width + barWidth) - barWidth;
        Rect fill(std::max(b.x, barX), b.y,
                  std::min(barWidth, b.x + b.width - barX), b.height);
        ctx->fillRoundedRect(fill, Color(0x42, 0x85, 0xF4), 4);
        animOffset_ += 2;
    } else {
        float fraction = std::clamp(value_ / max_, 0.0f, 1.0f);
        float fw = b.width * fraction;
        if (fw > 0) ctx->fillRoundedRect(Rect(b.x, b.y, fw, b.height), Color(0x42, 0x85, 0xF4), 4);
    }
}

// ==================================================================
// MeterWidget
// ==================================================================

MeterWidget::MeterWidget() {}

Size MeterWidget::measure(const Size& available) {
    return Size(std::min(200.0f, available.width), 16);
}

void MeterWidget::render(GpuContext* ctx) {
    if (!ctx) return;
    Rect b = bounds();
    ctx->fillRoundedRect(b, Color(0xE0, 0xE0, 0xE0), 3);
    float range = max_ - min_;
    if (range <= 0) return;
    float fraction = std::clamp((value_ - min_) / range, 0.0f, 1.0f);
    Color color;
    if (value_ < low_) color = (optimum_ >= high_) ? Color(0xEA, 0x43, 0x35) : Color(0xFB, 0xBC, 0x05);
    else if (value_ > high_) color = (optimum_ <= low_) ? Color(0xEA, 0x43, 0x35) : Color(0xFB, 0xBC, 0x05);
    else color = Color(0x34, 0xA8, 0x53);
    float fw = b.width * fraction;
    if (fw > 0) ctx->fillRoundedRect(Rect(b.x, b.y, fw, b.height), color, 3);
}

// ==================================================================
// RangeSliderWidget
// ==================================================================

RangeSliderWidget::RangeSliderWidget() {}

void RangeSliderWidget::setValue(double val) {
    value_ = snapToStep(std::clamp(val, min_, max_));
    if (onValueChange_) onValueChange_(value_);
}

double RangeSliderWidget::snapToStep(double val) const {
    if (step_ <= 0) return val;
    return std::clamp(std::round((val - min_) / step_) * step_ + min_, min_, max_);
}

float RangeSliderWidget::valueToPosition(double val) const {
    Rect b = bounds();
    double range = max_ - min_;
    if (range <= 0) return b.x;
    double n = (val - min_) / range;
    if (vertical_) return b.y + b.height - static_cast<float>(n) * b.height;
    return b.x + static_cast<float>(n) * b.width;
}

double RangeSliderWidget::positionToValue(float pos) const {
    Rect b = bounds();
    double n;
    if (vertical_) n = 1.0 - (pos - b.y) / b.height;
    else n = (pos - b.x) / b.width;
    return min_ + std::clamp(n, 0.0, 1.0) * (max_ - min_);
}

Size RangeSliderWidget::measure(const Size& available) {
    if (vertical_) return Size(24, std::min(160.0f, available.height));
    return Size(std::min(160.0f, available.width), 24);
}

void RangeSliderWidget::render(GpuContext* ctx) {
    if (!ctx) return;
    Rect b = bounds();
    Color trackColor(0xDD, 0xDD, 0xDD);
    Color fillColor(0x42, 0x85, 0xF4);

    if (vertical_) {
        float cx = b.x + b.width / 2;
        ctx->fillRoundedRect(Rect(cx - 2, b.y, 4, b.height), trackColor, 2);
        float ty = valueToPosition(value_);
        ctx->fillRoundedRect(Rect(cx - 2, ty, 4, b.y + b.height - ty), fillColor, 2);
        ctx->fillCircle(cx, ty, 8, Color::white());
    } else {
        float cy = b.y + b.height / 2;
        ctx->fillRoundedRect(Rect(b.x, cy - 2, b.width, 4), trackColor, 2);
        float tx = valueToPosition(value_);
        ctx->fillRoundedRect(Rect(b.x, cy - 2, tx - b.x, 4), fillColor, 2);
        ctx->fillCircle(tx, cy, 8, Color::white());
    }
}

EventResult RangeSliderWidget::onMouseDown(float x, float y, MouseButton /*button*/) {
    dragging_ = true;
    setValue(vertical_ ? positionToValue(y) : positionToValue(x));
    return EventResult::Handled;
}

EventResult RangeSliderWidget::onMouseMove(float x, float y) {
    if (!dragging_) return EventResult::Ignored;
    setValue(vertical_ ? positionToValue(y) : positionToValue(x));
    return EventResult::Handled;
}

EventResult RangeSliderWidget::onMouseUp(float /*x*/, float /*y*/, MouseButton /*button*/) {
    dragging_ = false;
    return EventResult::Handled;
}

} // namespace NXRender
