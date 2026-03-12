// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file label.cpp
 * @brief Label widget implementation
 */

#include "widgets/label.h"
#include "nxgfx/context.h"
#include "theme/theme.h"

namespace NXRender {

Label::Label() : text_("") {}

Label::Label(const std::string& text) : text_(text) {}

Label::~Label() = default;

void Label::render(GpuContext* ctx) {
    if (!isVisible() || text_.empty()) return;
    
    // Use theme text color if not set
    Color color = textColor_;
    if (color.a == 0) {
        Theme* t = currentTheme();
        color = t ? t->colors.textPrimary : Color::black();
    }
    
    // Calculate text position based on alignment
    Size textSize = ctx->measureText(text_, fontSize_);
    float textX = bounds_.x + padding_.left;
    
    switch (alignment_) {
        case TextAlign::Center:
            textX = bounds_.x + (bounds_.width - textSize.width) / 2;
            break;
        case TextAlign::Right:
            textX = bounds_.x + bounds_.width - textSize.width - padding_.right;
            break;
        default:
            break;
    }
    
    float textY = bounds_.y + padding_.top + textSize.height * 0.8f;
    
    ctx->drawText(text_, textX, textY, color, fontSize_);
}

Size Label::measure(const Size& available) {
    (void)available;
    Size textSize = gpu()->measureText(text_, fontSize_);
    return Size(
        textSize.width + padding_.horizontal(),
        textSize.height + padding_.vertical()
    );
}

} // namespace NXRender
