#pragma once

/**
 * @file event_system.hpp
 * @brief JavaScript Event system for browser environment
 */

#include "../config.hpp"
#include "../runtime/object.hpp"
#include "../runtime/value.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Zepra::Runtime { class Context; }

namespace Zepra::Browser {

using Runtime::Value;
using Runtime::Object;

/**
 * @brief Event interface
 */
class Event : public Object {
public:
    Event(const std::string& type, bool bubbles = false, bool cancelable = false);
    
    const std::string& type() const { return type_; }
    Object* target() const { return target_; }
    Object* currentTarget() const { return currentTarget_; }
    
    bool bubbles() const { return bubbles_; }
    bool cancelable() const { return cancelable_; }
    bool defaultPrevented() const { return defaultPrevented_; }
    
    uint16_t eventPhase() const { return eventPhase_; }
    double timeStamp() const { return timeStamp_; }
    
    void preventDefault();
    void stopPropagation();
    void stopImmediatePropagation();
    
    // For internal use
    void setTarget(Object* target) { target_ = target; }
    void setCurrentTarget(Object* currentTarget) { currentTarget_ = currentTarget; }
    void setEventPhase(uint16_t phase) { eventPhase_ = phase; }
    bool isPropagationStopped() const { return propagationStopped_; }
    bool isImmediatePropagationStopped() const { return immediatePropagationStopped_; }
    
    // Event phases
    static constexpr uint16_t NONE = 0;
    static constexpr uint16_t CAPTURING_PHASE = 1;
    static constexpr uint16_t AT_TARGET = 2;
    static constexpr uint16_t BUBBLING_PHASE = 3;
    
private:
    std::string type_;
    Object* target_ = nullptr;
    Object* currentTarget_ = nullptr;
    bool bubbles_;
    bool cancelable_;
    bool defaultPrevented_ = false;
    bool propagationStopped_ = false;
    bool immediatePropagationStopped_ = false;
    uint16_t eventPhase_ = NONE;
    double timeStamp_;
};

/**
 * @brief Event listener options
 */
struct EventListenerOptions {
    bool capture = false;
    bool once = false;
    bool passive = false;
};

/**
 * @brief Event listener entry
 */
struct EventListener {
    Value callback;
    EventListenerOptions options;
};

/**
 * @brief EventTarget interface
 */
class EventTarget : public Object {
public:
    EventTarget();
    
    void addEventListener(const std::string& type, Value callback, 
                          const EventListenerOptions& options = {});
    void removeEventListener(const std::string& type, Value callback,
                             bool capture = false);
    bool dispatchEvent(Event* event);
    
protected:
    std::unordered_map<std::string, std::vector<EventListener>> listeners_;
};

/**
 * @brief Mouse Event
 */
class MouseEvent : public Event {
public:
    MouseEvent(const std::string& type, int clientX = 0, int clientY = 0);
    
    int clientX() const { return clientX_; }
    int clientY() const { return clientY_; }
    int screenX() const { return screenX_; }
    int screenY() const { return screenY_; }
    int button() const { return button_; }
    int buttons() const { return buttons_; }
    bool ctrlKey() const { return ctrlKey_; }
    bool shiftKey() const { return shiftKey_; }
    bool altKey() const { return altKey_; }
    bool metaKey() const { return metaKey_; }
    
private:
    int clientX_, clientY_;
    int screenX_ = 0, screenY_ = 0;
    int button_ = 0, buttons_ = 0;
    bool ctrlKey_ = false, shiftKey_ = false;
    bool altKey_ = false, metaKey_ = false;
};

/**
 * @brief Keyboard Event
 */
class KeyboardEvent : public Event {
public:
    KeyboardEvent(const std::string& type, const std::string& key);
    
    const std::string& key() const { return key_; }
    const std::string& code() const { return code_; }
    bool ctrlKey() const { return ctrlKey_; }
    bool shiftKey() const { return shiftKey_; }
    bool altKey() const { return altKey_; }
    bool metaKey() const { return metaKey_; }
    bool repeat() const { return repeat_; }
    
private:
    std::string key_;
    std::string code_;
    bool ctrlKey_ = false, shiftKey_ = false;
    bool altKey_ = false, metaKey_ = false;
    bool repeat_ = false;
};

/**
 * @brief Custom Event
 */
class CustomEvent : public Event {
public:
    CustomEvent(const std::string& type, Value detail = Value::undefined());
    
    Value detail() const { return detail_; }
    
private:
    Value detail_;
};

} // namespace Zepra::Browser
