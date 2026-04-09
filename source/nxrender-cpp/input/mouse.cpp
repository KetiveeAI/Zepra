// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "input/mouse.h"
#include <cmath>
#include <algorithm>

namespace NXRender {

bool MouseState::isButtonDown(MouseButton button) const {
    return (buttons_ & (1 << static_cast<int>(button))) != 0;
}

void MouseState::onMouseMove(float x, float y) {
    deltaX_ = x - x_;
    deltaY_ = y - y_;
    x_ = x;
    y_ = y;

    if (isButtonDown(MouseButton::Left) && !dragging_) {
        float dx = x - dragStartX_;
        float dy = y - dragStartY_;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > dragThreshold_) {
            dragging_ = true;
        }
    }
}

void MouseState::onMouseDown(MouseButton button) {
    buttons_ |= (1 << static_cast<int>(button));

    if (button == MouseButton::Left) {
        dragStartX_ = x_;
        dragStartY_ = y_;
        dragging_ = false;
    }

    // Double-click detection
    if (button == MouseButton::Left) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastClickTime_).count();

        float dx = x_ - lastClickX_;
        float dy = y_ - lastClickY_;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (elapsed < doubleClickThresholdMs_ && dist < doubleClickRadius_) {
            clickCount_++;
        } else {
            clickCount_ = 1;
        }

        lastClickTime_ = now;
        lastClickX_ = x_;
        lastClickY_ = y_;
    }
}

void MouseState::onMouseUp(MouseButton button) {
    buttons_ &= ~(1 << static_cast<int>(button));

    if (button == MouseButton::Left) {
        dragging_ = false;
    }
}

void MouseState::onMouseWheel(float deltaX, float deltaY) {
    wheelDeltaX_ = deltaX;
    wheelDeltaY_ = deltaY;
    totalScrollX_ += deltaX;
    totalScrollY_ += deltaY;
}

void MouseState::onMouseEnterWindow() {
    insideWindow_ = true;
}

void MouseState::onMouseLeaveWindow() {
    insideWindow_ = false;
    buttons_ = 0;
    dragging_ = false;
}

void MouseState::reset() {
    x_ = 0;
    y_ = 0;
    deltaX_ = 0;
    deltaY_ = 0;
    buttons_ = 0;
    dragging_ = false;
    clickCount_ = 0;
    wheelDeltaX_ = 0;
    wheelDeltaY_ = 0;
    insideWindow_ = false;
}

CursorStyle MouseState::resolveCursor(CursorStyle widgetCursor) const {
    if (dragging_) return CursorStyle::Grabbing;
    return widgetCursor;
}

bool MouseState::isDoubleClick() const {
    return clickCount_ == 2;
}

bool MouseState::isTripleClick() const {
    return clickCount_ >= 3;
}

float MouseState::dragDistanceX() const {
    if (!dragging_) return 0;
    return x_ - dragStartX_;
}

float MouseState::dragDistanceY() const {
    if (!dragging_) return 0;
    return y_ - dragStartY_;
}

float MouseState::dragDistance() const {
    float dx = dragDistanceX();
    float dy = dragDistanceY();
    return std::sqrt(dx * dx + dy * dy);
}

Rect MouseState::dragRect() const {
    if (!dragging_) return Rect(0, 0, 0, 0);
    float minX = std::min(dragStartX_, x_);
    float minY = std::min(dragStartY_, y_);
    float maxX = std::max(dragStartX_, x_);
    float maxY = std::max(dragStartY_, y_);
    return Rect(minX, minY, maxX - minX, maxY - minY);
}

} // namespace NXRender
