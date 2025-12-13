/**
 * @file event_system.cpp
 * @brief JavaScript Event system implementation
 */

#include "zeprascript/browser/event_system.hpp"
#include <chrono>

namespace Zepra::Browser {

// =============================================================================
// Event Implementation
// =============================================================================

Event::Event(const std::string& type, bool bubbles, bool cancelable)
    : Object(Runtime::ObjectType::Ordinary)
    , type_(type)
    , bubbles_(bubbles)
    , cancelable_(cancelable) {
    
    auto now = std::chrono::system_clock::now();
    timeStamp_ = std::chrono::duration<double, std::milli>(
        now.time_since_epoch()).count();
}

void Event::preventDefault() {
    if (cancelable_) {
        defaultPrevented_ = true;
    }
}

void Event::stopPropagation() {
    propagationStopped_ = true;
}

void Event::stopImmediatePropagation() {
    propagationStopped_ = true;
    immediatePropagationStopped_ = true;
}

// =============================================================================
// EventTarget Implementation
// =============================================================================

EventTarget::EventTarget() : Object(Runtime::ObjectType::Ordinary) {}

void EventTarget::addEventListener(const std::string& type, Value callback,
                                    const EventListenerOptions& options) {
    EventListener listener;
    listener.callback = callback;
    listener.options = options;
    listeners_[type].push_back(listener);
}

void EventTarget::removeEventListener(const std::string& type, Value,
                                       bool) {
    // TODO: Match and remove specific callback
    auto it = listeners_.find(type);
    if (it != listeners_.end()) {
        it->second.clear();
    }
}

bool EventTarget::dispatchEvent(Event* event) {
    if (!event) return false;
    
    event->setTarget(this);
    event->setCurrentTarget(this);
    event->setEventPhase(Event::AT_TARGET);
    
    auto it = listeners_.find(event->type());
    if (it == listeners_.end()) return !event->defaultPrevented();
    
    std::vector<EventListener> toRemove;
    
    for (auto& listener : it->second) {
        if (event->isImmediatePropagationStopped()) break;
        
        // TODO: Call callback through VM
        // callCallback(listener.callback, event);
        
        if (listener.options.once) {
            toRemove.push_back(listener);
        }
    }
    
    // Remove once listeners
    for (const auto& rem : toRemove) {
        (void)rem; // TODO: Remove from list
    }
    
    return !event->defaultPrevented();
}

// =============================================================================
// MouseEvent Implementation
// =============================================================================

MouseEvent::MouseEvent(const std::string& type, int clientX, int clientY)
    : Event(type, true, true)
    , clientX_(clientX)
    , clientY_(clientY) {}

// =============================================================================
// KeyboardEvent Implementation
// =============================================================================

KeyboardEvent::KeyboardEvent(const std::string& type, const std::string& key)
    : Event(type, true, true)
    , key_(key)
    , code_(key) {}

// =============================================================================
// CustomEvent Implementation
// =============================================================================

CustomEvent::CustomEvent(const std::string& type, Value detail)
    : Event(type, false, false)
    , detail_(detail) {}

} // namespace Zepra::Browser
