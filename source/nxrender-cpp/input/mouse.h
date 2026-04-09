// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "events.h"
#include "../nxgfx/primitives.h"
#include <chrono>

namespace NXRender {

enum class CursorStyle : uint8_t {
    Default,
    Pointer,
    Text,
    Crosshair,
    Move,
    Resize,
    ResizeNS,
    ResizeEW,
    ResizeNESW,
    ResizeNWSE,
    NotAllowed,
    Wait,
    Grab,
    Grabbing,
    None
};

class MouseState {
public:
    // Position
    float x() const { return x_; }
    float y() const { return y_; }
    float deltaX() const { return deltaX_; }
    float deltaY() const { return deltaY_; }

    // Buttons
    bool isButtonDown(MouseButton button) const;
    bool isAnyButtonDown() const { return buttons_ != 0; }
    Modifiers modifiers() const { return modifiers_; }

    // Drag state
    bool isDragging() const { return dragging_; }
    float dragStartX() const { return dragStartX_; }
    float dragStartY() const { return dragStartY_; }
    float dragDistanceX() const;
    float dragDistanceY() const;
    float dragDistance() const;
    Rect dragRect() const;

    // Click detection
    int clickCount() const { return clickCount_; }
    bool isDoubleClick() const;
    bool isTripleClick() const;

    // Wheel
    float wheelDeltaX() const { return wheelDeltaX_; }
    float wheelDeltaY() const { return wheelDeltaY_; }
    float totalScrollX() const { return totalScrollX_; }
    float totalScrollY() const { return totalScrollY_; }

    // Window presence
    bool isInsideWindow() const { return insideWindow_; }

    // Cursor
    CursorStyle resolveCursor(CursorStyle widgetCursor) const;

    // Update from events
    void onMouseMove(float x, float y);
    void onMouseDown(MouseButton button);
    void onMouseUp(MouseButton button);
    void onMouseWheel(float deltaX, float deltaY);
    void onMouseEnterWindow();
    void onMouseLeaveWindow();
    void setModifiers(const Modifiers& mods) { modifiers_ = mods; }

    // Configuration
    void setDragThreshold(float px) { dragThreshold_ = px; }
    void setDoubleClickThreshold(int ms) { doubleClickThresholdMs_ = ms; }

    void reset();

private:
    float x_ = 0;
    float y_ = 0;
    float deltaX_ = 0;
    float deltaY_ = 0;
    uint8_t buttons_ = 0;
    Modifiers modifiers_;

    // Drag tracking
    bool dragging_ = false;
    float dragStartX_ = 0;
    float dragStartY_ = 0;
    float dragThreshold_ = 4.0f;

    // Click detection
    int clickCount_ = 0;
    float lastClickX_ = 0;
    float lastClickY_ = 0;
    std::chrono::steady_clock::time_point lastClickTime_{};
    int doubleClickThresholdMs_ = 400;
    float doubleClickRadius_ = 5.0f;

    // Wheel
    float wheelDeltaX_ = 0;
    float wheelDeltaY_ = 0;
    float totalScrollX_ = 0;
    float totalScrollY_ = 0;

    // Window
    bool insideWindow_ = false;
};

} // namespace NXRender
