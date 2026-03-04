/**
 * @file event_system.cpp
 * @brief JavaScript Event system implementation
 */

#include "browser/event_system.hpp"
#include "runtime/objects/function.hpp"
#include <chrono>
#include <algorithm>

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
    return dispatchEventWithContext(event, nullptr);
}

bool EventTarget::dispatchEventWithContext(Event* event, Runtime::Context* ctx) {
    if (!event) return false;
    
    event->setTarget(this);
    event->setCurrentTarget(this);
    event->setEventPhase(Event::AT_TARGET);
    
    auto it = listeners_.find(event->type());
    if (it == listeners_.end()) return !event->defaultPrevented();
    
    std::vector<EventListener> toRemove;
    
    for (auto& listener : it->second) {
        if (event->isImmediatePropagationStopped()) break;
        
        // Call callback through VM using Function::call()
        if (listener.callback.isObject()) {
            Runtime::Object* obj = listener.callback.asObject();
            if (obj && obj->isFunction()) {
                Runtime::Function* fn = static_cast<Runtime::Function*>(obj);
                // Create event argument
                std::vector<Value> args = { Value::object(event) };
                // Invoke the callback with 'this' as the event target
                fn->call(ctx, Value::object(this), args);
            }
        }
        
        if (listener.options.once) {
            toRemove.push_back(listener);
        }
    }
    
    // Remove once listeners
    if (!toRemove.empty()) {
        auto& listenerVec = it->second;
        for (const auto& rem : toRemove) {
            listenerVec.erase(
                std::remove_if(listenerVec.begin(), listenerVec.end(),
                    [&rem](const EventListener& l) {
                        // Compare by callback value (pointer equality for functions)
                        return l.callback.isObject() && rem.callback.isObject() &&
                               l.callback.asObject() == rem.callback.asObject();
                    }),
                listenerVec.end());
        }
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
