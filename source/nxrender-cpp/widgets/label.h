// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file label.h
 * @brief Label widget for displaying text
 */

#pragma once

#include "widget.h"

namespace NXRender {

/**
 * @brief Text alignment
 */
enum class TextAlign {
    Left,
    Center,
    Right
};

/**
 * @brief Label widget for text display
 */
class Label : public Widget {
public:
    Label();
    explicit Label(const std::string& text);
    ~Label() override;
    
    // Text
    const std::string& text() const { return text_; }
    void setText(const std::string& text) { text_ = text; }
    
    // Style
    Color textColor() const { return textColor_; }
    void setTextColor(const Color& color) { textColor_ = color; }
    
    float fontSize() const { return fontSize_; }
    void setFontSize(float size) { fontSize_ = size; }
    
    bool bold() const { return bold_; }
    void setBold(bool bold) { bold_ = bold; }
    
    bool italic() const { return italic_; }
    void setItalic(bool italic) { italic_ = italic; }
    
    TextAlign alignment() const { return alignment_; }
    void setAlignment(TextAlign align) { alignment_ = align; }
    
    bool wordWrap() const { return wordWrap_; }
    void setWordWrap(bool wrap) { wordWrap_ = wrap; }
    
    // Rendering
    void render(GpuContext* ctx) override;
    Size measure(const Size& available) override;
    
private:
    std::string text_;
    Color textColor_ = Color::black();
    float fontSize_ = 14.0f;
    bool bold_ = false;
    bool italic_ = false;
    TextAlign alignment_ = TextAlign::Left;
    bool wordWrap_ = false;
};

} // namespace NXRender
