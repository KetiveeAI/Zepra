/**
 * @file html_notifications.hpp
 * @brief Notifications API
 */

#pragma once

#include <string>
#include <functional>
#include <vector>

namespace Zepra::WebCore {

/**
 * @brief Notification permission
 */
enum class NotificationPermission {
    Default,
    Granted,
    Denied
};

/**
 * @brief Notification direction
 */
enum class NotificationDirection {
    Auto,
    LTR,
    RTL
};

/**
 * @brief Notification options
 */
struct NotificationOptions {
    NotificationDirection dir = NotificationDirection::Auto;
    std::string lang;
    std::string body;
    std::string tag;
    std::string image;
    std::string icon;
    std::string badge;
    std::vector<int> vibrate;
    double timestamp = 0;
    bool renotify = false;
    bool silent = false;
    bool requireInteraction = false;
    std::any data;
    std::vector<struct NotificationAction> actions;
};

/**
 * @brief Notification action
 */
struct NotificationAction {
    std::string action;
    std::string title;
    std::string icon;
};

/**
 * @brief Notification
 */
class Notification {
public:
    Notification(const std::string& title, const NotificationOptions& options = {});
    ~Notification();
    
    // Static methods
    static NotificationPermission permission();
    static void requestPermission(std::function<void(NotificationPermission)> callback);
    
    // Properties
    std::string title() const { return title_; }
    NotificationDirection dir() const { return options_.dir; }
    std::string lang() const { return options_.lang; }
    std::string body() const { return options_.body; }
    std::string tag() const { return options_.tag; }
    std::string image() const { return options_.image; }
    std::string icon() const { return options_.icon; }
    std::string badge() const { return options_.badge; }
    double timestamp() const { return options_.timestamp; }
    bool renotify() const { return options_.renotify; }
    bool silent() const { return options_.silent; }
    bool requireInteraction() const { return options_.requireInteraction; }
    std::any data() const { return options_.data; }
    std::vector<NotificationAction> actions() const { return options_.actions; }
    
    // Methods
    void close();
    
    // Events
    std::function<void()> onClick;
    std::function<void()> onClose;
    std::function<void()> onError;
    std::function<void()> onShow;
    
private:
    std::string title_;
    NotificationOptions options_;
};

} // namespace Zepra::WebCore
