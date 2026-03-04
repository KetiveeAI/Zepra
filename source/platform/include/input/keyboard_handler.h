/**
 * @file keyboard_handler.h
 * @brief Keyboard event processing and focus management
 */

#ifndef ZEPRA_INPUT_KEYBOARD_HANDLER_H
#define ZEPRA_INPUT_KEYBOARD_HANDLER_H

#include <string>
#include <functional>
#include <memory>
#include <cstdint>

namespace zepra {
namespace input {

/**
 * Key codes (X11-compatible with extensions)
 */
namespace KeyCode {
    // Letters
    constexpr int A = 'a';
    constexpr int B = 'b';
    constexpr int C = 'c';
    constexpr int D = 'd';
    constexpr int E = 'e';
    constexpr int F = 'f';
    constexpr int G = 'g';
    constexpr int H = 'h';
    constexpr int I = 'i';
    constexpr int J = 'j';
    constexpr int K = 'k';
    constexpr int L = 'l';
    constexpr int M = 'm';
    constexpr int N = 'n';
    constexpr int O = 'o';
    constexpr int P = 'p';
    constexpr int Q = 'q';
    constexpr int R = 'r';
    constexpr int S = 's';
    constexpr int T = 't';
    constexpr int U = 'u';
    constexpr int V = 'v';
    constexpr int W = 'w';
    constexpr int X = 'x';
    constexpr int Y = 'y';
    constexpr int Z = 'z';
    
    // Numbers
    constexpr int Num0 = '0';
    constexpr int Num1 = '1';
    constexpr int Num2 = '2';
    constexpr int Num3 = '3';
    constexpr int Num4 = '4';
    constexpr int Num5 = '5';
    constexpr int Num6 = '6';
    constexpr int Num7 = '7';
    constexpr int Num8 = '8';
    constexpr int Num9 = '9';
    
    // Function keys
    constexpr int F1 = 0xFF01;
    constexpr int F2 = 0xFF02;
    constexpr int F3 = 0xFF03;
    constexpr int F4 = 0xFF04;
    constexpr int F5 = 0xFF05;
    constexpr int F6 = 0xFF06;
    constexpr int F7 = 0xFF07;
    constexpr int F8 = 0xFF08;
    constexpr int F9 = 0xFF09;
    constexpr int F10 = 0xFF0A;
    constexpr int F11 = 0xFF0B;
    constexpr int F12 = 0xFF0C;
    
    // Navigation
    constexpr int Up = 0xFF52;
    constexpr int Down = 0xFF54;
    constexpr int Left = 0xFF51;
    constexpr int Right = 0xFF53;
    constexpr int Home = 0xFF50;
    constexpr int End = 0xFF57;
    constexpr int PageUp = 0xFF55;
    constexpr int PageDown = 0xFF56;
    
    // Editing
    constexpr int Backspace = 0xFF08;
    constexpr int Delete = 0xFFFF;
    constexpr int Enter = 0xFF0D;
    constexpr int Tab = 0xFF09;
    constexpr int Escape = 0xFF1B;
    constexpr int Space = ' ';
    
    // Modifiers
    constexpr int LeftCtrl = 0xFFE3;
    constexpr int RightCtrl = 0xFFE4;
    constexpr int LeftShift = 0xFFE1;
    constexpr int RightShift = 0xFFE2;
    constexpr int LeftAlt = 0xFFE9;
    constexpr int RightAlt = 0xFFEA;
    constexpr int LeftSuper = 0xFFEB;
    constexpr int RightSuper = 0xFFEC;
    
    // Symbols
    constexpr int Plus = '+';
    constexpr int Minus = '-';
    constexpr int Equal = '=';
}

/**
 * Modifier key flags
 */
struct Modifiers {
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    bool super = false;     // Windows/Command key
    
    bool any() const { return ctrl || shift || alt || super; }
    bool none() const { return !any(); }
    bool onlyCtrl() const { return ctrl && !shift && !alt && !super; }
};

/**
 * Key event data
 */
struct KeyEvent {
    int keyCode;
    Modifiers modifiers;
    bool pressed;           // true = press, false = release
    bool repeat;            // true if auto-repeat
    uint32_t timestamp;
};

/**
 * Text input event
 */
struct TextInputEvent {
    std::string text;       // UTF-8 encoded
    uint32_t timestamp;
};

/**
 * KeyboardHandler - Central keyboard event processor
 * 
 * Features:
 * - Key press/release tracking
 * - Modifier state management
 * - Focus management between UI elements
 * - Event routing to focused widget
 */
class KeyboardHandler {
public:
    using KeyCallback = std::function<bool(const KeyEvent& event)>;
    using TextCallback = std::function<bool(const TextInputEvent& event)>;

    KeyboardHandler();
    ~KeyboardHandler();

    // Event processing (call from main event loop)
    void processKeyDown(int keyCode, uint32_t timestamp = 0);
    void processKeyUp(int keyCode, uint32_t timestamp = 0);
    void processTextInput(const std::string& text, uint32_t timestamp = 0);

    // Modifier state
    Modifiers getModifiers() const;
    bool isKeyPressed(int keyCode) const;
    
    // Callbacks (return true if handled)
    void setKeyCallback(KeyCallback callback);
    void setTextCallback(TextCallback callback);

    // Focus management
    void setFocusedWidget(void* widget);
    void* getFocusedWidget() const;
    void clearFocus();

    // Reset state (on window focus loss)
    void reset();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Utility functions
std::string keyCodeToString(int keyCode);
int stringToKeyCode(const std::string& str);
bool isModifierKey(int keyCode);
bool isPrintableKey(int keyCode);

} // namespace input
} // namespace zepra

#endif // ZEPRA_INPUT_KEYBOARD_HANDLER_H
