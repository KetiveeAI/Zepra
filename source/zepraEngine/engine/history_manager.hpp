/**
 * @file history_manager.hpp
 * @brief Global browser history management
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <optional>

namespace Zepra::Engine {

struct HistoryItem {
    int64_t id;
    std::string url;
    std::string title;
    std::chrono::system_clock::time_point lastVisit;
    int visitCount;
    std::string faviconUrl;
};

/**
 * @class HistoryManager
 * @brief Manages persistent browsing history across sessions
 */
class HistoryManager {
public:
    HistoryManager();
    ~HistoryManager();
    
    // Add visit
    void addVisit(const std::string& url, const std::string& title);
    
    // Query history
    std::vector<HistoryItem> getRecent(int limit = 100);
    std::vector<HistoryItem> search(const std::string& query, int limit = 50);
    std::vector<HistoryItem> getByDate(
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to);
    
    std::optional<HistoryItem> getByUrl(const std::string& url);
    
    // Delete
    void deleteItem(int64_t id);
    void deleteByUrl(const std::string& url);
    void deleteRange(
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to);
    void clearAll();
    
    // Most visited
    std::vector<HistoryItem> getMostVisited(int limit = 10);
    
    // Persistence
    bool load(const std::string& path);
    bool save(const std::string& path);
    
    // Callback
    using ChangeCallback = std::function<void()>;
    void setOnChange(ChangeCallback callback) { onChange_ = std::move(callback); }
    
private:
    std::vector<HistoryItem> items_;
    int64_t nextId_ = 1;
    ChangeCallback onChange_;
};

HistoryManager& getHistoryManager();

} // namespace Zepra::Engine
