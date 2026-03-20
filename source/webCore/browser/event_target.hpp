#pragma once

#include "event.hpp"
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>

namespace Zepra::WebCore {

class DOMNode;

/**
 * @brief Event target - can receive events
 */
class EventTarget {
public:
    virtual ~EventTarget() = default;
    
    // Add/remove listeners
    void addEventListener(const std::string& type, EventListener listener, 
                         const EventListenerOptions& options = {});
    void removeEventListener(const std::string& type, size_t listenerId); // TODO: Remove by listener ptr/obj?
    
    // Dispatch event
    virtual bool dispatchEvent(Event& event);
    
protected:
    struct ListenerEntry {
        size_t id;
        EventListener listener;
        EventListenerOptions options;
    };
    
    std::unordered_map<std::string, std::vector<ListenerEntry>> listeners_;
    size_t nextListenerId_ = 1;

    friend class EventDispatcher;
};

/**
 * @brief Event dispatcher - handles event propagation
 */
class EventDispatcher {
public:
    static void dispatch(Event& event, DOMNode* target);
    
private:
    static std::vector<DOMNode*> buildEventPath(DOMNode* target);
    static void invokeListeners(Event& event, DOMNode* node, EventPhase phase);
};

} // namespace Zepra::WebCore
