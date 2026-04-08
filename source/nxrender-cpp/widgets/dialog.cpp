// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "widgets/dialog.h"
#include "nxgfx/context.h"
#include "platform/display_info.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace NXRender {

Dialog::Dialog() {
    backgroundColor_ = Color(255, 255, 255);
    state_.visible = false;
}

void Dialog::setPrimaryButton(const std::string& text, std::function<void()> callback) {
    primaryBtn_.text = text;
    primaryBtn_.callback = std::move(callback);
    hasPrimary_ = true;
}

void Dialog::setSecondaryButton(const std::string& text, std::function<void()> callback) {
    secondaryBtn_.text = text;
    secondaryBtn_.callback = std::move(callback);
    hasSecondary_ = true;
}

void Dialog::setCancelButton(const std::string& text, std::function<void()> callback) {
    cancelBtn_.text = text;
    cancelBtn_.callback = std::move(callback);
    hasCancel_ = true;
}

void Dialog::show() {
    showing_ = true;
    state_.visible = true;
    backdropOpacity_ = 0.0f;
    dialogScale_ = 0.9f;
    positionCenter();
    invalidate();
}

void Dialog::dismiss() {
    showing_ = false;
    state_.visible = false;
    if (onDismiss_) onDismiss_();
    invalidate();
}

void Dialog::positionCenter() {
    const auto& di = DisplayInfo::instance().metrics();
    Size sz = measure(Size(static_cast<float>(di.windowWidth), static_cast<float>(di.windowHeight)));
    float x = (static_cast<float>(di.windowWidth) - sz.width) * 0.5f;
    float y = (static_cast<float>(di.windowHeight) - sz.height) * 0.5f;
    setBounds(Rect(x, y, sz.width, sz.height));
}

Size Dialog::measure(const Size& available) {
    float charWidth = 14.0f * 0.6f;
    float messageWidth = static_cast<float>(message_.size()) * charWidth;
    float titleWidth = static_cast<float>(title_.size()) * 16.0f * 0.6f;

    float contentWidth = std::max(messageWidth, titleWidth);
    contentWidth = std::clamp(contentWidth + kPadding * 2, minWidth_, maxWidth_);

    // Height: title bar + message + spacing + buttons + padding
    float messageHeight = 0;
    if (!message_.empty()) {
        // Word wrap estimate
        float lineWidth = contentWidth - kPadding * 2;
        int charPerLine = std::max(1, static_cast<int>(lineWidth / charWidth));
        int lineCount = (static_cast<int>(message_.size()) + charPerLine - 1) / charPerLine;
        messageHeight = static_cast<float>(lineCount) * 20.0f;
    }

    float buttonRow = hasPrimary_ ? (kButtonHeight + kPadding) : 0;
    float totalHeight = kTitleBarHeight + kPadding + messageHeight + buttonRow + kPadding;

    return Size(contentWidth, totalHeight);
}

bool Dialog::isInTitleBar(float x, float y) const {
    const Rect& b = bounds();
    return x >= b.x && x <= b.x + b.width && y >= b.y && y <= b.y + kTitleBarHeight;
}

bool Dialog::isInCloseButton(float x, float y) const {
    const Rect& b = bounds();
    float cx = b.x + b.width - kPadding - kCloseButtonSize * 0.5f;
    float cy = b.y + kTitleBarHeight * 0.5f;
    float dx = x - cx;
    float dy = y - cy;
    return (dx * dx + dy * dy) <= (kCloseButtonSize * 0.5f * kCloseButtonSize * 0.5f);
}

EventResult Dialog::onMouseDown(float x, float y, MouseButton button) {
    if (!showing_) return EventResult::Ignored;

    // Check if click is outside dialog (backdrop click)
    if (!bounds().contains(x, y)) {
        if (closeOnBackdrop_) {
            dismiss();
            return EventResult::NeedsRedraw;
        }
        return EventResult::Handled;
    }

    // Close button
    if (isInCloseButton(x, y)) {
        dismiss();
        return EventResult::NeedsRedraw;
    }

    // Title bar drag
    if (isInTitleBar(x, y)) {
        dragging_ = true;
        dragStartX_ = x;
        dragStartY_ = y;
        dialogStartX_ = bounds().x;
        dialogStartY_ = bounds().y;
        return EventResult::Handled;
    }

    // Button hits
    const Rect& b = bounds();
    float buttonY = b.y + b.height - kPadding - kButtonHeight;
    float buttonX = b.x + b.width - kPadding;

    if (hasPrimary_) {
        float bw = std::max(80.0f, static_cast<float>(primaryBtn_.text.size()) * 8.0f + 32);
        Rect btnRect(buttonX - bw, buttonY, bw, kButtonHeight);
        if (btnRect.contains(x, y)) {
            if (primaryBtn_.callback) primaryBtn_.callback();
            dismiss();
            return EventResult::NeedsRedraw;
        }
        buttonX -= bw + kButtonSpacing;
    }

    if (hasSecondary_) {
        float bw = std::max(80.0f, static_cast<float>(secondaryBtn_.text.size()) * 8.0f + 32);
        Rect btnRect(buttonX - bw, buttonY, bw, kButtonHeight);
        if (btnRect.contains(x, y)) {
            if (secondaryBtn_.callback) secondaryBtn_.callback();
            dismiss();
            return EventResult::NeedsRedraw;
        }
        buttonX -= bw + kButtonSpacing;
    }

    if (hasCancel_) {
        float bw = std::max(80.0f, static_cast<float>(cancelBtn_.text.size()) * 8.0f + 32);
        Rect btnRect(buttonX - bw, buttonY, bw, kButtonHeight);
        if (btnRect.contains(x, y)) {
            if (cancelBtn_.callback) cancelBtn_.callback();
            dismiss();
            return EventResult::NeedsRedraw;
        }
    }

    return EventResult::Handled;
}

EventResult Dialog::onMouseMove(float x, float y) {
    if (!showing_) return EventResult::Ignored;

    if (dragging_) {
        float dx = x - dragStartX_;
        float dy = y - dragStartY_;
        setPosition(dialogStartX_ + dx, dialogStartY_ + dy);
        return EventResult::NeedsRedraw;
    }

    return EventResult::Handled;
}

EventResult Dialog::onMouseUp(float x, float y, MouseButton button) {
    if (dragging_) {
        dragging_ = false;
        return EventResult::Handled;
    }
    return EventResult::Ignored;
}

EventResult Dialog::onKeyDown(KeyCode key, Modifiers mods) {
    if (!showing_) return EventResult::Ignored;

    if (key == KeyCode::Escape && closeOnEscape_) {
        dismiss();
        return EventResult::NeedsRedraw;
    }

    if (key == KeyCode::Enter && hasPrimary_) {
        if (primaryBtn_.callback) primaryBtn_.callback();
        dismiss();
        return EventResult::NeedsRedraw;
    }

    return EventResult::Handled;
}

void Dialog::render(GpuContext* ctx) {
    if (!ctx || !showing_) return;

    const auto& di = DisplayInfo::instance().metrics();

    // Animate backdrop and scale
    if (backdropOpacity_ < 0.5f) {
        backdropOpacity_ = std::min(0.5f, backdropOpacity_ + 0.05f);
    }
    if (dialogScale_ < 1.0f) {
        dialogScale_ = std::min(1.0f, dialogScale_ + 0.02f);
    }

    // Backdrop
    Rect viewport(0, 0, static_cast<float>(di.windowWidth), static_cast<float>(di.windowHeight));
    uint8_t alpha = static_cast<uint8_t>(255 * backdropOpacity_);
    ctx->fillRect(viewport, Color(0, 0, 0, alpha));

    const Rect& b = bounds();

    // Shadow
    ctx->drawShadow(b, Color(0, 0, 0, 80), 20.0f, 0, 4);

    // Dialog background
    ctx->fillRoundedRect(b, backgroundColor_, kBorderRadius);

    // Title bar
    Rect titleBar(b.x, b.y, b.width, kTitleBarHeight);
    ctx->fillRoundedRect(Rect(b.x, b.y, b.width, kTitleBarHeight + kBorderRadius),
                         Color(0xF5F5F5), kBorderRadius);
    // Clip the bottom corners of title bar area
    ctx->fillRect(Rect(b.x, b.y + kTitleBarHeight - 1, b.width, 2), Color(0xE0E0E0));

    // Title text
    if (!title_.empty()) {
        ctx->drawText(title_, b.x + kPadding, b.y + (kTitleBarHeight - 16) * 0.5f,
                      Color(0x212121), 16.0f);
    }

    // Close button (X)
    float closeX = b.x + b.width - kPadding - kCloseButtonSize * 0.5f;
    float closeY = b.y + kTitleBarHeight * 0.5f;
    float cs = kCloseButtonSize * 0.3f;
    ctx->drawLine(closeX - cs, closeY - cs, closeX + cs, closeY + cs, Color(0x757575), 2.0f);
    ctx->drawLine(closeX + cs, closeY - cs, closeX - cs, closeY + cs, Color(0x757575), 2.0f);

    // Message text
    if (!message_.empty()) {
        float textY = b.y + kTitleBarHeight + kPadding;
        float lineWidth = b.width - kPadding * 2;
        float charWidth = 14.0f * 0.6f;
        int charPerLine = std::max(1, static_cast<int>(lineWidth / charWidth));

        // Simple word wrapping
        std::string remaining = message_;
        while (!remaining.empty()) {
            std::string line;
            if (static_cast<int>(remaining.size()) <= charPerLine) {
                line = remaining;
                remaining.clear();
            } else {
                // Find last space within charPerLine
                int breakAt = charPerLine;
                for (int i = charPerLine; i >= 0; i--) {
                    if (remaining[i] == ' ') { breakAt = i; break; }
                }
                line = remaining.substr(0, breakAt);
                remaining = remaining.substr(breakAt);
                if (!remaining.empty() && remaining[0] == ' ') remaining = remaining.substr(1);
            }
            ctx->drawText(line, b.x + kPadding, textY, Color(0x424242), 14.0f);
            textY += 20.0f;
        }
    }

    // Action buttons (right-aligned in bottom row)
    float buttonY = b.y + b.height - kPadding - kButtonHeight;
    float buttonX = b.x + b.width - kPadding;

    auto renderButton = [&](const ActionButton& btn, bool filled) {
        float bw = std::max(80.0f, static_cast<float>(btn.text.size()) * 8.0f + 32);
        Rect btnRect(buttonX - bw, buttonY, bw, kButtonHeight);

        if (filled) {
            ctx->fillRoundedRect(btnRect, btn.color, 4.0f);
            ctx->drawText(btn.text, btnRect.x + (bw - static_cast<float>(btn.text.size()) * 7.0f) * 0.5f,
                          btnRect.y + (kButtonHeight - 14) * 0.5f, Color(255, 255, 255), 14.0f);
        } else {
            ctx->strokeRoundedRect(btnRect, btn.color, 4.0f, 1.0f);
            ctx->drawText(btn.text, btnRect.x + (bw - static_cast<float>(btn.text.size()) * 7.0f) * 0.5f,
                          btnRect.y + (kButtonHeight - 14) * 0.5f, btn.color, 14.0f);
        }

        buttonX -= bw + kButtonSpacing;
    };

    if (hasPrimary_) renderButton(primaryBtn_, true);
    if (hasSecondary_) renderButton(secondaryBtn_, false);
    if (hasCancel_) renderButton(cancelBtn_, false);
}

} // namespace NXRender
