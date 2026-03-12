// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file mouse.cpp
 * @brief Mouse input implementation
 */

#include "input/mouse.h"

namespace NXRender {

bool MouseState::isButtonDown(MouseButton button) const {
    return (buttons_ & (1 << static_cast<int>(button))) != 0;
}

void MouseState::onMouseMove(float x, float y) {
    x_ = x;
    y_ = y;
}

void MouseState::onMouseDown(MouseButton button) {
    buttons_ |= (1 << static_cast<int>(button));
}

void MouseState::onMouseUp(MouseButton button) {
    buttons_ &= ~(1 << static_cast<int>(button));
}

} // namespace NXRender
