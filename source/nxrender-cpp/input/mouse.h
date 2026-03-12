// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file mouse.h
 * @brief Mouse input handling
 */

#pragma once

#include "events.h"

namespace NXRender {

/**
 * @brief Mouse state tracker
 */
class MouseState {
public:
    float x() const { return x_; }
    float y() const { return y_; }
    bool isButtonDown(MouseButton button) const;
    Modifiers modifiers() const { return modifiers_; }
    
    // Update from events
    void onMouseMove(float x, float y);
    void onMouseDown(MouseButton button);
    void onMouseUp(MouseButton button);
    void setModifiers(const Modifiers& mods) { modifiers_ = mods; }
    
private:
    float x_ = 0;
    float y_ = 0;
    uint8_t buttons_ = 0;  // Bitmask
    Modifiers modifiers_;
};

} // namespace NXRender
