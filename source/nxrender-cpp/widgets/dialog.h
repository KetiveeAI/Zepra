// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file dialog.h
 * @brief Modal dialog widget with title bar, close button, and action buttons.
 */

#pragma once

#include "widget.h"
#include "button.h"
#include "label.h"
#include <functional>

namespace NXRender {

class Dialog : public Widget {
public:
    Dialog();

    void setTitle(const std::string& title) { title_ = title; }
    const std::string& title() const { return title_; }

    void setMessage(const std::string& message) { message_ = message; }

    // Action buttons
    void setPrimaryButton(const std::string& text, std::function<void()> callback);
    void setSecondaryButton(const std::string& text, std::function<void()> callback);
    void setCancelButton(const std::string& text, std::function<void()> callback);

    // Control
    void show();
    void dismiss();
    bool isShowing() const { return showing_; }

    // Close behavior
    void setCloseOnBackdropClick(bool enable) { closeOnBackdrop_ = enable; }
    void setCloseOnEscape(bool enable) { closeOnEscape_ = enable; }

    // Sizing
    void setMinWidth(float w) { minWidth_ = w; }
    void setMaxWidth(float w) { maxWidth_ = w; }

    // Callbacks
    void setOnDismiss(std::function<void()> callback) { onDismiss_ = std::move(callback); }

    // Rendering
    void render(GpuContext* ctx) override;
    Size measure(const Size& available) override;

    // Events
    EventResult onMouseDown(float x, float y, MouseButton button) override;
    EventResult onMouseMove(float x, float y) override;
    EventResult onMouseUp(float x, float y, MouseButton button) override;
    EventResult onKeyDown(KeyCode key, Modifiers mods) override;

private:
    void positionCenter();
    bool isInTitleBar(float x, float y) const;
    bool isInCloseButton(float x, float y) const;

    std::string title_;
    std::string message_;
    bool showing_ = false;
    bool closeOnBackdrop_ = true;
    bool closeOnEscape_ = true;
    float minWidth_ = 300.0f;
    float maxWidth_ = 600.0f;

    struct ActionButton {
        std::string text;
        std::function<void()> callback;
        Color color;
        bool hovered = false;
    };

    ActionButton primaryBtn_{"OK", nullptr, Color(0x2196F3), false};
    ActionButton secondaryBtn_;
    ActionButton cancelBtn_;
    bool hasPrimary_ = true;
    bool hasSecondary_ = false;
    bool hasCancel_ = false;

    std::function<void()> onDismiss_;

    // Drag state
    bool dragging_ = false;
    float dragStartX_ = 0, dragStartY_ = 0;
    float dialogStartX_ = 0, dialogStartY_ = 0;

    // Animation
    float backdropOpacity_ = 0.0f;
    float dialogScale_ = 0.9f;

    // Layout constants
    static constexpr float kTitleBarHeight = 40.0f;
    static constexpr float kCloseButtonSize = 24.0f;
    static constexpr float kPadding = 20.0f;
    static constexpr float kButtonHeight = 36.0f;
    static constexpr float kButtonSpacing = 8.0f;
    static constexpr float kBorderRadius = 8.0f;
};

} // namespace NXRender
