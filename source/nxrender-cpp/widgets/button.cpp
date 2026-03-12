// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file button.cpp
 * @brief Button widget implementation
 */

#include "widgets/button.h"
#include "nxgfx/context.h"
#include "theme/theme.h"

namespace NXRender {

Button::Button() : label_("Button") {
    setPadding(EdgeInsets(8, 16));
}

Button::Button(const std::string& label) : label_(label) {
    setPadding(EdgeInsets(8, 16));
}

Button::~Button() = default;

void Button::render(GpuContext* ctx) {
    if (!isVisible()) return;
    
    // Determine background color based on state
    Color bgColor = backgroundColor_;
    if (bgColor.a == 0) {
        // Use theme primary color
        Theme* t = currentTheme();
        if (t) {
            if (isPressed()) {
                bgColor = t->colors.primaryPressed;
            } else if (isHovered()) {
                bgColor = t->colors.primaryHover;
            } else {
                bgColor = t->colors.primary;
            }
        } else {
            bgColor = isPressed() ? Color(0x1565C0) : 
                      isHovered() ? Color(0x1976D2) : Color(0x2196F3);
        }
    } else {
        if (isPressed()) {
            bgColor = bgColor.darken(0.2f);
        } else if (isHovered()) {
            bgColor = bgColor.lighten(0.1f);
        }
    }
    
    // Draw button background
    if (cornerRadius_ > 0) {
        ctx->fillRoundedRect(bounds_, bgColor, cornerRadius_);
    } else {
        ctx->fillRect(bounds_, bgColor);
    }
    
    // Draw label centered
    Size textSize = ctx->measureText(label_, 14.0f);
    float textX = bounds_.x + (bounds_.width - textSize.width) / 2;
    float textY = bounds_.y + (bounds_.height - textSize.height) / 2 + textSize.height * 0.8f;
    ctx->drawText(label_, textX, textY, textColor_, 14.0f);
}

Size Button::measure(const Size& available) {
    (void)available;
    Size textSize = gpu()->measureText(label_, 14.0f);
    return Size(
        textSize.width + padding_.horizontal(),
        textSize.height + padding_.vertical()
    );
}

EventResult Button::onMouseDown(float x, float y, MouseButton button) {
    (void)x; (void)y;
    if (button == MouseButton::Left) {
        state_.pressed = true;
        return EventResult::NeedsRedraw;
    }
    return EventResult::Ignored;
}

EventResult Button::onMouseUp(float x, float y, MouseButton button) {
    if (button == MouseButton::Left && state_.pressed) {
        state_.pressed = false;
        if (bounds_.contains(x, y) && clickHandler_) {
            clickHandler_();
        }
        return EventResult::NeedsRedraw;
    }
    return EventResult::Ignored;
}

EventResult Button::onMouseEnter() {
    state_.hovered = true;
    return EventResult::NeedsRedraw;
}

EventResult Button::onMouseLeave() {
    state_.hovered = false;
    state_.pressed = false;
    return EventResult::NeedsRedraw;
}

} // namespace NXRender
