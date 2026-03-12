// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file keyboard.h
 * @brief Keyboard input handling
 */

#pragma once

#include "events.h"
#include <unordered_set>

namespace NXRender {

/**
 * @brief Keyboard state tracker
 */
class KeyboardState {
public:
    bool isKeyDown(KeyCode key) const;
    Modifiers modifiers() const { return modifiers_; }
    
    // Update from events
    void onKeyDown(KeyCode key);
    void onKeyUp(KeyCode key);
    void setModifiers(const Modifiers& mods) { modifiers_ = mods; }
    
private:
    std::unordered_set<int> pressedKeys_;
    Modifiers modifiers_;
};

/**
 * @brief Convert X11/Win32 keycode to NXRender KeyCode
 */
KeyCode translateKeyCode(int platformKeyCode);

} // namespace NXRender
