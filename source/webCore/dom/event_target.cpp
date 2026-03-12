// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file event_target.cpp
 * @brief EventTarget implementation stub
 */

#include "webcore/browser/event_target.hpp"
#include "webcore/browser/dom.hpp"

namespace Zepra::WebCore {

void EventTarget::addEventListener(const std::string& type, EventListener listener, 
                                    const EventListenerOptions& options) {
    ListenerEntry entry;
    entry.id = nextListenerId_++;
    entry.listener = listener;
    entry.options = options;
    listeners_[type].push_back(entry);
}

void EventTarget::removeEventListener(const std::string& type, size_t listenerId) {
    auto it = listeners_.find(type);
    if (it != listeners_.end()) {
        auto& vec = it->second;
        vec.erase(std::remove_if(vec.begin(), vec.end(),
            [listenerId](const ListenerEntry& e) { return e.id == listenerId; }),
            vec.end());
    }
}

bool EventTarget::dispatchEvent(Event& event) {
    auto it = listeners_.find(event.type());
    if (it == listeners_.end()) return true;
    
    for (auto& entry : it->second) {
        if (entry.listener) {
            entry.listener(event);
        }
        if (event.immediatePropagationStopped()) break;
    }
    
    return !event.defaultPrevented();
}

// EventDispatcher
void EventDispatcher::dispatch(Event& event, DOMNode* target) {
    if (!target) return;
    
    event.setTarget(target);
    
    // Capture phase
    auto path = buildEventPath(target);
    
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        if (*it != target) {
            event.setCurrentTarget(*it);
            event.setEventPhase(EventPhase::Capturing);
            (*it)->dispatchEvent(event);
            if (event.propagationStopped()) return;
        }
    }
    
    // Target phase
    event.setCurrentTarget(target);
    event.setEventPhase(EventPhase::AtTarget);
    target->dispatchEvent(event);
    if (event.propagationStopped()) return;
    
    // Bubble phase (if bubbles)
    if (event.bubbles()) {
        for (auto* node : path) {
            if (node != target) {
                event.setCurrentTarget(node);
                event.setEventPhase(EventPhase::Bubbling);
                node->dispatchEvent(event);
                if (event.propagationStopped()) return;
            }
        }
    }
}

std::vector<DOMNode*> EventDispatcher::buildEventPath(DOMNode* target) {
    std::vector<DOMNode*> path;
    for (DOMNode* node = target; node; node = node->parentNode()) {
        path.push_back(node);
    }
    return path;
}

void EventDispatcher::invokeListeners(Event& event, DOMNode* node, EventPhase phase) {
    // Stub
}

} // namespace Zepra::WebCore
