// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file textfield.cpp
 * @brief Text input field implementation
 */

#include "widgets/textfield.h"
#include "nxgfx/context.h"
#include "theme/theme.h"
#include <algorithm>

namespace NXRender {

TextField::TextField() : placeholder_("") {
    setPadding(EdgeInsets(8, 12));
    setBackgroundColor(Color::white());
}

TextField::TextField(const std::string& placeholder) 
    : placeholder_(placeholder) {
    setPadding(EdgeInsets(8, 12));
    setBackgroundColor(Color::white());
}

TextField::~TextField() = default;

void TextField::setValue(const std::string& value) {
    value_ = value;
    if (maxLength_ > 0 && static_cast<int>(value_.length()) > maxLength_) {
        value_ = value_.substr(0, maxLength_);
    }
    cursorPos_ = std::min(cursorPos_, static_cast<int>(value_.length()));
    if (changeHandler_) {
        changeHandler_(value_);
    }
}

void TextField::setSelection(int start, int end) {
    selectionStart_ = std::clamp(start, 0, static_cast<int>(value_.length()));
    selectionEnd_ = std::clamp(end, 0, static_cast<int>(value_.length()));
}

void TextField::selectAll() {
    selectionStart_ = 0;
    selectionEnd_ = static_cast<int>(value_.length());
}

std::string TextField::selectedText() const {
    if (selectionStart_ == selectionEnd_) return "";
    int start = std::min(selectionStart_, selectionEnd_);
    int end = std::max(selectionStart_, selectionEnd_);
    return value_.substr(start, end - start);
}

void TextField::setCursorPosition(int pos) {
    cursorPos_ = std::clamp(pos, 0, static_cast<int>(value_.length()));
}

void TextField::render(GpuContext* ctx) {
    if (!isVisible()) return;
    
    Theme* t = currentTheme();
    
    // Draw background
    Color bgColor = backgroundColor_;
    if (bgColor.a == 0) {
        bgColor = t ? t->colors.surface : Color::white();
    }
    
    if (cornerRadius_ > 0) {
        ctx->fillRoundedRect(bounds_, bgColor, cornerRadius_);
    } else {
        ctx->fillRect(bounds_, bgColor);
    }
    
    // Draw border
    Color borderColor = t ? t->colors.border : Color(0xCCCCCC);
    if (isFocused()) {
        borderColor = t ? t->colors.primary : Color(0x2196F3);
    }
    if (cornerRadius_ > 0) {
        ctx->strokeRoundedRect(bounds_, borderColor, cornerRadius_, 1.0f);
    } else {
        ctx->strokeRect(bounds_, borderColor, 1.0f);
    }
    
    // Clip to text area
    Rect textArea = bounds_.inset(padding_.left, padding_.top);
    ctx->pushClip(textArea);
    
    // Determine what to draw
    std::string displayText;
    Color displayColor;
    
    if (value_.empty() && !isFocused()) {
        displayText = placeholder_;
        displayColor = placeholderColor_;
    } else if (isPassword_) {
        displayText = std::string(value_.length(), '*');
        displayColor = textColor_;
    } else {
        displayText = value_;
        displayColor = textColor_;
    }
    
    // Draw selection highlight
    if (selectionStart_ != selectionEnd_ && isFocused()) {
        int start = std::min(selectionStart_, selectionEnd_);
        int end = std::max(selectionStart_, selectionEnd_);
        
        std::string beforeSel = value_.substr(0, start);
        std::string selected = value_.substr(start, end - start);
        
        float startX = ctx->measureText(beforeSel, 14.0f).width;
        float selWidth = ctx->measureText(selected, 14.0f).width;
        
        Color selColor = t ? t->colors.primary.withAlpha(60) : Color(33, 150, 243, 60);
        ctx->fillRect(Rect(textArea.x + startX, textArea.y, selWidth, textArea.height), selColor);
    }
    
    // Draw text
    float textY = textArea.y + textArea.height / 2 + 4;
    ctx->drawText(displayText, textArea.x, textY, displayColor, 14.0f);
    
    // Draw cursor
    if (isFocused() && cursorVisible_) {
        std::string beforeCursor = value_.substr(0, cursorPos_);
        float cursorX = textArea.x + ctx->measureText(beforeCursor, 14.0f).width;
        ctx->drawLine(cursorX, textArea.y + 2, cursorX, textArea.y + textArea.height - 2,
                      textColor_, 1.0f);
    }
    
    ctx->popClip();
}

Size TextField::measure(const Size& available) {
    (void)available;
    return Size(200, 36);  // Default size
}

EventResult TextField::onMouseDown(float x, float y, MouseButton button) {
    if (button != MouseButton::Left) return EventResult::Ignored;
    
    // Calculate cursor position from click
    float textX = bounds_.x + padding_.left;
    float clickOffset = x - textX;
    
    // Simple: approximate character position
    float charWidth = 8.0f;  // Approximate
    int pos = static_cast<int>(clickOffset / charWidth);
    pos = std::clamp(pos, 0, static_cast<int>(value_.length()));
    
    cursorPos_ = pos;
    selectionStart_ = pos;
    selectionEnd_ = pos;
    
    return EventResult::NeedsRedraw;
}

EventResult TextField::onKeyDown(KeyCode key, Modifiers mods) {
    if (isReadOnly_) return EventResult::Ignored;
    
    switch (key) {
        case KeyCode::Backspace:
            if (selectionStart_ != selectionEnd_) {
                deleteSelection();
            } else if (cursorPos_ > 0) {
                value_.erase(cursorPos_ - 1, 1);
                cursorPos_--;
                if (changeHandler_) changeHandler_(value_);
            }
            return EventResult::NeedsRedraw;
            
        case KeyCode::Delete:
            if (selectionStart_ != selectionEnd_) {
                deleteSelection();
            } else if (cursorPos_ < static_cast<int>(value_.length())) {
                value_.erase(cursorPos_, 1);
                if (changeHandler_) changeHandler_(value_);
            }
            return EventResult::NeedsRedraw;
            
        case KeyCode::Left:
            moveCursor(-1, mods.shift);
            return EventResult::NeedsRedraw;
            
        case KeyCode::Right:
            moveCursor(1, mods.shift);
            return EventResult::NeedsRedraw;
            
        case KeyCode::Home:
            cursorPos_ = 0;
            if (!mods.shift) selectionStart_ = selectionEnd_ = cursorPos_;
            return EventResult::NeedsRedraw;
            
        case KeyCode::End:
            cursorPos_ = static_cast<int>(value_.length());
            if (!mods.shift) selectionStart_ = selectionEnd_ = cursorPos_;
            return EventResult::NeedsRedraw;
            
        case KeyCode::Enter:
            if (submitHandler_) submitHandler_(value_);
            return EventResult::Handled;
            
        case KeyCode::A:
            if (mods.ctrl) {
                selectAll();
                return EventResult::NeedsRedraw;
            }
            break;
            
        default:
            break;
    }
    
    return EventResult::Ignored;
}

EventResult TextField::onTextInput(const std::string& text) {
    if (isReadOnly_) return EventResult::Ignored;
    insertText(text);
    return EventResult::NeedsRedraw;
}

EventResult TextField::onFocus() {
    state_.focused = true;
    cursorVisible_ = true;
    return EventResult::NeedsRedraw;
}

EventResult TextField::onBlur() {
    state_.focused = false;
    selectionStart_ = selectionEnd_ = 0;
    return EventResult::NeedsRedraw;
}

void TextField::insertText(const std::string& text) {
    if (selectionStart_ != selectionEnd_) {
        deleteSelection();
    }
    
    value_.insert(cursorPos_, text);
    cursorPos_ += static_cast<int>(text.length());
    
    if (maxLength_ > 0 && static_cast<int>(value_.length()) > maxLength_) {
        value_ = value_.substr(0, maxLength_);
        cursorPos_ = std::min(cursorPos_, maxLength_);
    }
    
    if (changeHandler_) changeHandler_(value_);
}

void TextField::deleteSelection() {
    int start = std::min(selectionStart_, selectionEnd_);
    int end = std::max(selectionStart_, selectionEnd_);
    value_.erase(start, end - start);
    cursorPos_ = start;
    selectionStart_ = selectionEnd_ = cursorPos_;
    if (changeHandler_) changeHandler_(value_);
}

void TextField::moveCursor(int delta, bool extend) {
    cursorPos_ = std::clamp(cursorPos_ + delta, 0, static_cast<int>(value_.length()));
    if (extend) {
        selectionEnd_ = cursorPos_;
    } else {
        selectionStart_ = selectionEnd_ = cursorPos_;
    }
}

} // namespace NXRender
