/**
 * @file html_event_source.hpp
 * @brief Server-Sent Events (EventSource) API
 */

#pragma once

#include <string>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief EventSource ready state
 */
enum class EventSourceReadyState {
    Connecting = 0,
    Open = 1,
    Closed = 2
};

/**
 * @brief Server-sent event
 */
struct ServerEvent {
    std::string type;
    std::string data;
    std::string lastEventId;
    std::string origin;
};

/**
 * @brief EventSource options
 */
struct EventSourceInit {
    bool withCredentials = false;
};

/**
 * @brief EventSource - Server-Sent Events
 */
class EventSource {
public:
    EventSource(const std::string& url, const EventSourceInit& init = {});
    ~EventSource();
    
    // Properties
    std::string url() const { return url_; }
    bool withCredentials() const { return withCredentials_; }
    EventSourceReadyState readyState() const { return readyState_; }
    
    // Methods
    void close();
    
    // Events
    std::function<void()> onOpen;
    std::function<void(const ServerEvent&)> onMessage;
    std::function<void(const std::string&)> onError;
    
    // Named event listeners
    void addEventListener(const std::string& type,
                          std::function<void(const ServerEvent&)> listener);
    void removeEventListener(const std::string& type);
    
private:
    std::string url_;
    bool withCredentials_ = false;
    EventSourceReadyState readyState_ = EventSourceReadyState::Connecting;
    
    std::unordered_map<std::string, std::function<void(const ServerEvent&)>> listeners_;
    
    void connect();
};

} // namespace Zepra::WebCore
