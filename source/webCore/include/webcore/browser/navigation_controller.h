/**
 * @file navigation_controller.h
 * @brief URL navigation and history management
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace ZepraBrowser {

struct NavigationEntry {
    std::string url;
    std::string title;
    int64_t timestamp;
};

/**
 * NavigationController - Manages URL navigation and history
 *
 * Features:
 * - URL parsing and normalization
 * - Back/forward navigation
 * - History per tab
 * - Reload functionality
 */
class NavigationController {
public:
    NavigationController(int tabId);
    ~NavigationController();
    
    // Navigation
    void navigateTo(const std::string& url);
    bool canGoBack() const;
    bool canGoForward() const;
    void goBack();
    void goForward();
    void reload();
    void stop();
    
    // Current state
    std::string getCurrentUrl() const;
    std::string getCurrentTitle() const;
    bool isLoading() const;
    
    // History
    std::vector<NavigationEntry> getHistory() const;
    void clearHistory();
    
    // Callbacks
    using NavigationCallback = std::function<void(const std::string& url)>;
    void onNavigationStart(NavigationCallback callback);
    void onNavigationComplete(NavigationCallback callback);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ZepraBrowser
