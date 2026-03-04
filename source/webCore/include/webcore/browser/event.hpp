/**
 * @file event.hpp
 * @brief DOM Event system
 */

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Zepra::WebCore {

// Forward declarations
class DOMNode;
class DOMElement;

/**
 * @brief Event phases
 */
enum class EventPhase {
    None = 0,
    Capturing = 1,
    AtTarget = 2,
    Bubbling = 3
};

/**
 * @brief Base DOM Event
 */
class Event {
public:
    Event(const std::string& type, bool bubbles = true, bool cancelable = true);
    virtual ~Event() = default;
    
    // Event type
    const std::string& type() const { return type_; }
    
    // Target element
    DOMNode* target() const { return target_; }
    DOMNode* currentTarget() const { return currentTarget_; }
    void setTarget(DOMNode* target) { target_ = target; }
    void setCurrentTarget(DOMNode* target) { currentTarget_ = target; }
    
    // Phase
    EventPhase eventPhase() const { return phase_; }
    void setEventPhase(EventPhase phase) { phase_ = phase; }
    
    // Propagation
    bool bubbles() const { return bubbles_; }
    bool cancelable() const { return cancelable_; }
    
    void stopPropagation() { propagationStopped_ = true; }
    void stopImmediatePropagation() { immediatePropagationStopped_ = true; propagationStopped_ = true; }
    bool propagationStopped() const { return propagationStopped_; }
    bool immediatePropagationStopped() const { return immediatePropagationStopped_; }
    
    void preventDefault() { if (cancelable_) defaultPrevented_ = true; }
    bool defaultPrevented() const { return defaultPrevented_; }
    
    // Timestamp
    double timeStamp() const { return timeStamp_; }
    
protected:
    std::string type_;
    DOMNode* target_ = nullptr;
    DOMNode* currentTarget_ = nullptr;
    EventPhase phase_ = EventPhase::None;
    bool bubbles_;
    bool cancelable_;
    bool propagationStopped_ = false;
    bool immediatePropagationStopped_ = false;
    bool defaultPrevented_ = false;
    double timeStamp_ = 0;
};

/**
 * @brief Mouse Event
 */
class MouseEvent : public Event {
public:
    MouseEvent(const std::string& type, float clientX, float clientY, int button = 0);
    
    float clientX() const { return clientX_; }
    float clientY() const { return clientY_; }
    float pageX() const { return pageX_; }
    float pageY() const { return pageY_; }
    float screenX() const { return screenX_; }
    float screenY() const { return screenY_; }
    
    int button() const { return button_; }
    int buttons() const { return buttons_; }
    
    bool ctrlKey() const { return ctrlKey_; }
    bool shiftKey() const { return shiftKey_; }
    bool altKey() const { return altKey_; }
    bool metaKey() const { return metaKey_; }
    
    void setModifiers(bool ctrl, bool shift, bool alt, bool meta) {
        ctrlKey_ = ctrl; shiftKey_ = shift; altKey_ = alt; metaKey_ = meta;
    }
    
private:
    float clientX_, clientY_;
    float pageX_ = 0, pageY_ = 0;
    float screenX_ = 0, screenY_ = 0;
    int button_;
    int buttons_ = 0;
    bool ctrlKey_ = false, shiftKey_ = false, altKey_ = false, metaKey_ = false;
};

/**
 * @brief Keyboard Event
 */
class KeyboardEvent : public Event {
public:
    KeyboardEvent(const std::string& type, const std::string& key, int keyCode);
    
    const std::string& key() const { return key_; }
    const std::string& code() const { return code_; }
    int keyCode() const { return keyCode_; }
    
    bool ctrlKey() const { return ctrlKey_; }
    bool shiftKey() const { return shiftKey_; }
    bool altKey() const { return altKey_; }
    bool metaKey() const { return metaKey_; }
    bool repeat() const { return repeat_; }
    
    void setModifiers(bool ctrl, bool shift, bool alt, bool meta) {
        ctrlKey_ = ctrl; shiftKey_ = shift; altKey_ = alt; metaKey_ = meta;
    }
    
private:
    std::string key_;
    std::string code_;
    int keyCode_;
    bool ctrlKey_ = false, shiftKey_ = false, altKey_ = false, metaKey_ = false;
    bool repeat_ = false;
};

/**
 * @brief Wheel Event
 */
class WheelEvent : public MouseEvent {
public:
    WheelEvent(float clientX, float clientY, float deltaX, float deltaY, float deltaZ = 0);
    
    float deltaX() const { return deltaX_; }
    float deltaY() const { return deltaY_; }
    float deltaZ() const { return deltaZ_; }
    int deltaMode() const { return deltaMode_; } // 0=pixel, 1=line, 2=page
    
private:
    float deltaX_, deltaY_, deltaZ_;
    int deltaMode_ = 0;
};

/**
 * @brief Focus Event
 */
class FocusEvent : public Event {
public:
    FocusEvent(const std::string& type, DOMElement* relatedTarget = nullptr);
    
    DOMElement* relatedTarget() const { return relatedTarget_; }
    
private:
    DOMElement* relatedTarget_;
};

/**
 * @brief Input Event
 */
class InputEvent : public Event {
public:
    InputEvent(const std::string& type, const std::string& data, const std::string& inputType);
    
    const std::string& data() const { return data_; }
    const std::string& inputType() const { return inputType_; }
    
private:
    std::string data_;
    std::string inputType_;
};

/**
 * @brief Event listener callback
 */
using EventListener = std::function<void(Event&)>;

/**
 * @brief Event listener options
 */
struct EventListenerOptions {
    bool capture = false;
    bool once = false;
    bool passive = false;
};



} // namespace Zepra::WebCore
