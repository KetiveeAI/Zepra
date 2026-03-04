/**
 * @file history_database.h
 * @brief Browser history tracking and search
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <memory>

namespace ZepraBrowser {

struct HistoryEntry {
    int64_t id;
    std::string url;
    std::string title;
    int32_t visitCount;
    std::chrono::system_clock::time_point lastVisit;
    std::chrono::system_clock::time_point firstVisit;
};

/**
 * HistoryDatabase - Records and manages browsing history
 * 
 * Features:
 * - Record page visits
 * - Search history
 * - Clear history (all/range/specific URL)
 * - Privacy mode (no recording)
 */
class HistoryDatabase {
public:
    static HistoryDatabase& instance();
    
    // Recording
    void recordVisit(const std::string& url, const std::string& title);
    void setPrivacyMode(bool enabled);
    bool isPrivacyMode() const;
    
    // Querying
    std::vector<HistoryEntry> search(const std::string& query, int limit = 50) const;
    std::vector<HistoryEntry> getRecent(int count = 20) const;
    std::vector<HistoryEntry> getMostVisited(int count = 10) const;
    std::vector<HistoryEntry> getVisitedToday() const;
    HistoryEntry* getEntry(const std::string& url);
    
    // Clearing
    void clearAll();
    void clearRange(std::chrono::system_clock::time_point start,
                   std::chrono::system_clock::time_point end);
    void clearUrl(const std::string& url);
    void clearBefore(std::chrono::system_clock::time_point cutoff);
    
    // Persistence
    bool load(const std::string& filepath);
    bool save(const std::string& filepath);
    
private:
    HistoryDatabase();
    ~HistoryDatabase();
    HistoryDatabase(const HistoryDatabase&) = delete;
    HistoryDatabase& operator=(const HistoryDatabase&) = delete;
    
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ZepraBrowser
