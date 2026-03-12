/**
 * @file navigation_history.hpp
 * @brief Browser navigation history (back/forward)
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace Zepra::WebCore {

/**
 * @brief History entry
 */
struct HistoryEntry {
    std::string url;
    std::string title;
    std::string scrollPosition;  // Serialized scroll state
    std::chrono::system_clock::time_point visitTime;
};

/**
 * @brief Navigation History Manager
 * 
 * Manages back/forward navigation per tab.
 */
class NavigationHistory {
public:
    NavigationHistory();
    ~NavigationHistory();
    
    /**
     * @brief Push new navigation
     */
    void push(const std::string& url, const std::string& title = "");
    
    /**
     * @brief Go back
     * @return URL to navigate to, empty if can't go back
     */
    std::string goBack();
    
    /**
     * @brief Go forward
     * @return URL to navigate to, empty if can't go forward
     */
    std::string goForward();
    
    /**
     * @brief Check if can go back
     */
    bool canGoBack() const { return currentIndex_ > 0; }
    
    /**
     * @brief Check if can go forward
     */
    bool canGoForward() const { return currentIndex_ < static_cast<int>(entries_.size()) - 1; }
    
    /**
     * @brief Get current entry
     */
    const HistoryEntry* current() const;
    
    /**
     * @brief Get entry at offset (-1 = back, +1 = forward)
     */
    const HistoryEntry* entryAt(int offset) const;
    
    /**
     * @brief Get all entries
     */
    const std::vector<HistoryEntry>& entries() const { return entries_; }
    
    /**
     * @brief Get current index
     */
    int currentIndex() const { return currentIndex_; }
    
    /**
     * @brief Clear history
     */
    void clear();
    
    /**
     * @brief Update current entry's title
     */
    void updateTitle(const std::string& title);
    
    /**
     * @brief Update current entry's scroll position
     */
    void updateScrollPosition(const std::string& scroll);
    
    /**
     * @brief Set history change callback
     */
    using HistoryChangeCallback = std::function<void()>;
    void setOnChange(HistoryChangeCallback cb) { onChange_ = std::move(cb); }
    
private:
    std::vector<HistoryEntry> entries_;
    int currentIndex_ = -1;
    HistoryChangeCallback onChange_;
};

} // namespace Zepra::WebCore
