/**
 * @file event.cpp
 * @brief DOM Event system implementation
 */

#include "webcore/event.hpp"
#include "webcore/dom.hpp"
#include <chrono>

namespace Zepra::WebCore {

// =============================================================================
// Event
// =============================================================================

Event::Event(const std::string& type, bool bubbles, bool cancelable)
    : type_(type), bubbles_(bubbles), cancelable_(cancelable) {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    timeStamp_ = std::chrono::duration<double, std::milli>(duration).count();
}

// =============================================================================
// MouseEvent
// =============================================================================

MouseEvent::MouseEvent(const std::string& type, float clientX, float clientY, int button)
    : Event(type, true, true), clientX_(clientX), clientY_(clientY), button_(button) {
    pageX_ = clientX;
    pageY_ = clientY;
    screenX_ = clientX;
    screenY_ = clientY;
}

// =============================================================================
// KeyboardEvent
// =============================================================================

KeyboardEvent::KeyboardEvent(const std::string& type, const std::string& key, int keyCode)
    : Event(type, true, true), key_(key), keyCode_(keyCode) {
    // Map common keys to code
    if (key == " ") code_ = "Space";
    else if (key == "Enter") code_ = "Enter";
    else if (key == "Escape") code_ = "Escape";
    else if (key == "Tab") code_ = "Tab";
    else if (key == "Backspace") code_ = "Backspace";
    else if (key == "ArrowUp") code_ = "ArrowUp";
    else if (key == "ArrowDown") code_ = "ArrowDown";
    else if (key == "ArrowLeft") code_ = "ArrowLeft";
    else if (key == "ArrowRight") code_ = "ArrowRight";
    else if (key.length() == 1 && std::isalpha(key[0])) {
        code_ = std::string("Key") + static_cast<char>(std::toupper(key[0]));
    } else if (key.length() == 1 && std::isdigit(key[0])) {
        code_ = std::string("Digit") + key;
    } else {
        code_ = key;
    }
}

// =============================================================================
// WheelEvent
// =============================================================================

WheelEvent::WheelEvent(float clientX, float clientY, float deltaX, float deltaY, float deltaZ)
    : MouseEvent("wheel", clientX, clientY, 0), deltaX_(deltaX), deltaY_(deltaY), deltaZ_(deltaZ) {}

// =============================================================================
// FocusEvent
// =============================================================================

FocusEvent::FocusEvent(const std::string& type, DOMElement* relatedTarget)
    : Event(type, false, false), relatedTarget_(relatedTarget) {}

// =============================================================================
// InputEvent
// =============================================================================

InputEvent::InputEvent(const std::string& type, const std::string& data, const std::string& inputType)
    : Event(type, true, false), data_(data), inputType_(inputType) {}



} // namespace Zepra::WebCore
