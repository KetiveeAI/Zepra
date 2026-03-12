// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file textfield.h
 * @brief Text input field widget
 */

#pragma once

#include "widget.h"
#include <functional>

namespace NXRender {

/**
 * @brief Text input field widget
 */
class TextField : public Widget {
public:
    TextField();
    explicit TextField(const std::string& placeholder);
    ~TextField() override;
    
    // Value
    const std::string& value() const { return value_; }
    void setValue(const std::string& value);
    
    // Placeholder
    const std::string& placeholder() const { return placeholder_; }
    void setPlaceholder(const std::string& placeholder) { placeholder_ = placeholder; }
    
    // Properties
    bool isPassword() const { return isPassword_; }
    void setPassword(bool password) { isPassword_ = password; }
    
    bool isReadOnly() const { return isReadOnly_; }
    void setReadOnly(bool readOnly) { isReadOnly_ = readOnly; }
    
    int maxLength() const { return maxLength_; }
    void setMaxLength(int length) { maxLength_ = length; }
    
    // Selection
    int selectionStart() const { return selectionStart_; }
    int selectionEnd() const { return selectionEnd_; }
    void setSelection(int start, int end);
    void selectAll();
    std::string selectedText() const;
    
    // Cursor
    int cursorPosition() const { return cursorPos_; }
    void setCursorPosition(int pos);
    
    // Style
    float cornerRadius() const { return cornerRadius_; }
    void setCornerRadius(float radius) { cornerRadius_ = radius; }
    
    Color textColor() const { return textColor_; }
    void setTextColor(const Color& color) { textColor_ = color; }
    
    Color placeholderColor() const { return placeholderColor_; }
    void setPlaceholderColor(const Color& color) { placeholderColor_ = color; }
    
    // Callbacks
    using ChangeHandler = std::function<void(const std::string&)>;
    using SubmitHandler = std::function<void(const std::string&)>;
    
    void onChange(ChangeHandler handler) { changeHandler_ = handler; }
    void onSubmit(SubmitHandler handler) { submitHandler_ = handler; }
    
    // Rendering
    void render(GpuContext* ctx) override;
    Size measure(const Size& available) override;
    
    // Events
    EventResult onMouseDown(float x, float y, MouseButton button) override;
    EventResult onKeyDown(KeyCode key, Modifiers mods) override;
    EventResult onTextInput(const std::string& text) override;
    EventResult onFocus() override;
    EventResult onBlur() override;
    
private:
    std::string value_;
    std::string placeholder_;
    bool isPassword_ = false;
    bool isReadOnly_ = false;
    int maxLength_ = -1;  // -1 = unlimited
    int cursorPos_ = 0;
    int selectionStart_ = 0;
    int selectionEnd_ = 0;
    float cornerRadius_ = 4.0f;
    Color textColor_ = Color::black();
    Color placeholderColor_ = Color(150, 150, 150);
    
    int cursorBlinkTimer_ = 0;
    bool cursorVisible_ = true;
    
    ChangeHandler changeHandler_;
    SubmitHandler submitHandler_;
    
    void insertText(const std::string& text);
    void deleteSelection();
    void moveCursor(int delta, bool extend);
};

} // namespace NXRender
