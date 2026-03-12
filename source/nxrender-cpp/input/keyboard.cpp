// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file keyboard.cpp
 * @brief Keyboard input implementation
 */

#include "input/keyboard.h"

namespace NXRender {

bool KeyboardState::isKeyDown(KeyCode key) const {
    return pressedKeys_.count(static_cast<int>(key)) > 0;
}

void KeyboardState::onKeyDown(KeyCode key) {
    pressedKeys_.insert(static_cast<int>(key));
}

void KeyboardState::onKeyUp(KeyCode key) {
    pressedKeys_.erase(static_cast<int>(key));
}

KeyCode translateKeyCode(int platformKeyCode) {
    // X11 keycodes translation
    switch (platformKeyCode) {
        case 9: return KeyCode::Escape;
        case 22: return KeyCode::Backspace;
        case 23: return KeyCode::Tab;
        case 36: return KeyCode::Enter;
        case 50: case 62: return KeyCode::Shift;
        case 37: case 105: return KeyCode::Ctrl;
        case 64: case 108: return KeyCode::Alt;
        case 65: return KeyCode::Space;
        case 113: return KeyCode::Left;
        case 114: return KeyCode::Right;
        case 111: return KeyCode::Up;
        case 116: return KeyCode::Down;
        case 110: return KeyCode::Home;
        case 115: return KeyCode::End;
        case 112: return KeyCode::PageUp;
        case 117: return KeyCode::PageDown;
        case 119: return KeyCode::Delete;
        case 118: return KeyCode::Insert;
        // Letters A-Z (keycodes 38-58 on typical layouts)
        case 38: return KeyCode::A;
        case 56: return KeyCode::B;
        case 54: return KeyCode::C;
        case 40: return KeyCode::D;
        case 26: return KeyCode::E;
        case 41: return KeyCode::F;
        case 42: return KeyCode::G;
        case 43: return KeyCode::H;
        case 31: return KeyCode::I;
        case 44: return KeyCode::J;
        case 45: return KeyCode::K;
        case 46: return KeyCode::L;
        case 58: return KeyCode::M;
        case 57: return KeyCode::N;
        case 32: return KeyCode::O;
        case 33: return KeyCode::P;
        case 24: return KeyCode::Q;
        case 27: return KeyCode::R;
        case 39: return KeyCode::S;
        case 28: return KeyCode::T;
        case 30: return KeyCode::U;
        case 55: return KeyCode::V;
        case 25: return KeyCode::W;
        case 53: return KeyCode::X;
        case 29: return KeyCode::Y;
        case 52: return KeyCode::Z;
        // Numbers
        case 10: return KeyCode::Num1;
        case 11: return KeyCode::Num2;
        case 12: return KeyCode::Num3;
        case 13: return KeyCode::Num4;
        case 14: return KeyCode::Num5;
        case 15: return KeyCode::Num6;
        case 16: return KeyCode::Num7;
        case 17: return KeyCode::Num8;
        case 18: return KeyCode::Num9;
        case 19: return KeyCode::Num0;
        // Function keys
        case 67: return KeyCode::F1;
        case 68: return KeyCode::F2;
        case 69: return KeyCode::F3;
        case 70: return KeyCode::F4;
        case 71: return KeyCode::F5;
        case 72: return KeyCode::F6;
        case 73: return KeyCode::F7;
        case 74: return KeyCode::F8;
        case 75: return KeyCode::F9;
        case 76: return KeyCode::F10;
        case 95: return KeyCode::F11;
        case 96: return KeyCode::F12;
        default: return KeyCode::Unknown;
    }
}

} // namespace NXRender
