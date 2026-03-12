// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file events.h
 * @brief Event types for NXRender input handling
 */

#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace NXRender {

/**
 * @brief Mouse buttons
 */
enum class MouseButton {
    None = 0,
    Left = 1,
    Middle = 2,
    Right = 3,
    X1 = 4,
    X2 = 5
};

/**
 * @brief Keyboard modifiers
 */
struct Modifiers {
    bool shift = false;
    bool ctrl = false;
    bool alt = false;
    bool meta = false;  // Cmd on macOS, Win on Windows
    
    bool any() const { return shift || ctrl || alt || meta; }
    bool none() const { return !any(); }
};

/**
 * @brief Key codes (subset of common keys)
 */
enum class KeyCode {
    Unknown = 0,
    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    // Numbers
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    // Navigation
    Left, Right, Up, Down,
    Home, End, PageUp, PageDown,
    // Editing
    Backspace, Delete, Insert,
    Enter, Tab, Escape, Space,
    // Modifiers
    Shift, Ctrl, Alt, Meta,
    CapsLock, NumLock, ScrollLock,
    // Punctuation
    Comma, Period, Slash, Backslash,
    Semicolon, Quote, LeftBracket, RightBracket,
    Minus, Equals, Backtick, Plus
};

/**
 * @brief Event type
 */
enum class EventType {
    None,
    // Mouse
    MouseDown,
    MouseUp,
    MouseMove,
    MouseEnter,
    MouseLeave,
    MouseWheel,
    // Keyboard
    KeyDown,
    KeyUp,
    TextInput,
    // Focus
    Focus,
    Blur,
    // Window
    Resize,
    Close,
    // File
    FileDrop
};

/**
 * @brief Mouse event data
 */
struct MouseEventData {
    float x = 0;
    float y = 0;
    MouseButton button = MouseButton::None;
    float wheelDelta = 0;  // For scroll
    Modifiers modifiers;
};

/**
 * @brief Keyboard event data
 */
struct KeyEventData {
    KeyCode key = KeyCode::Unknown;
    Modifiers modifiers;
    bool repeat = false;
};

/**
 * @brief Text input event data
 */
struct TextEventData {
    std::string text;
};

/**
 * @brief Window event data
 */
struct WindowEventData {
    int width = 0;
    int height = 0;
};

/**
 * @brief File drop event data
 */
struct FileDropData {
    std::vector<std::string> files;  // List of dropped file paths
    float x = 0;  // Drop position
    float y = 0;
};

/**
 * @brief Unified event structure
 */
struct Event {
    EventType type = EventType::None;
    uint64_t timestamp = 0;  // In milliseconds
    
    union {
        MouseEventData mouse;
        KeyEventData key;
        WindowEventData window;
    };
    
    std::string textInput;  // For TextInput event
    std::vector<std::string> droppedFiles;  // For FileDrop event
    float dropX = 0, dropY = 0;  // Drop position
    
    Event() : mouse{} {}
    ~Event() = default;
    
    // Helpers
    bool isMouse() const {
        return type == EventType::MouseDown || type == EventType::MouseUp ||
               type == EventType::MouseMove || type == EventType::MouseWheel ||
               type == EventType::MouseEnter || type == EventType::MouseLeave;
    }
    
    bool isKeyboard() const {
        return type == EventType::KeyDown || type == EventType::KeyUp ||
               type == EventType::TextInput;
    }
};

} // namespace NXRender
