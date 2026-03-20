/**
 * @file navigation_controller.hpp
 * @brief URL navigation and history controller
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace Zepra::Engine {

struct HistoryEntry {
    std::string url;
    std::string title;
    std::chrono::system_clock::time_point timestamp;
    float scrollPosition = 0.0f;
};

/**
 * @class NavigationController
 * @brief Manages navigation history and URL transitions
 */
class NavigationController {
public:
    NavigationController();
    ~NavigationController();
    
    // Navigation
    void navigate(const std::string& url);
    bool goBack();
    bool goForward();
    void reload();
    void stop();
    
    // History state
    bool canGoBack() const;
    bool canGoForward() const;
    std::string currentUrl() const;
    std::string pendingUrl() const;
    
    // Entry access
    int entryCount() const { return static_cast<int>(entries_.size()); }
    int currentEntryIndex() const { return currentIndex_; }
    const HistoryEntry* getEntry(int index) const;
    const HistoryEntry* currentEntry() const;
    
    // Modify current entry
    void updateTitle(const std::string& title);
    void updateScrollPosition(float pos);
    
    // Callbacks
    using NavigateCallback = std::function<void(const std::string& url, bool isReload)>;
    using StateCallback = std::function<void()>;
    
    void setOnNavigate(NavigateCallback callback) { onNavigate_ = std::move(callback); }
    void setOnStateChange(StateCallback callback) { onStateChange_ = std::move(callback); }
    
    // Clear history
    void clearHistory();
    
private:
    void pushEntry(const std::string& url);
    void notifyStateChange();
    
    std::vector<HistoryEntry> entries_;
    int currentIndex_ = -1;
    std::string pendingUrl_;
    
    NavigateCallback onNavigate_;
    StateCallback onStateChange_;
};

} // namespace Zepra::Engine
