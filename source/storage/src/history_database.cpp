/**
 * @file history_database.cpp
 * @brief History tracking implementation
 */

#include "storage/history_database.h"
#include <iostream>
#include <algorithm>
#include <map>

namespace ZepraBrowser {

struct HistoryDatabase::Impl {
    std::map<std::string, HistoryEntry> entries;  // URL -> Entry
    int64_t nextId = 1;
    bool privacyMode = false;
    std::string filepath;
};

HistoryDatabase& HistoryDatabase::instance() {
    static HistoryDatabase instance;
    return instance;
}

HistoryDatabase::HistoryDatabase() : impl_(std::make_unique<Impl>()) {
    std::cout << "[HistoryDatabase] Initialized" << std::endl;
}

HistoryDatabase::~HistoryDatabase() = default;

void HistoryDatabase::recordVisit(const std::string& url, const std::string& title) {
    if (impl_->privacyMode) {
        return;  // Don't record in privacy mode
    }
    
    auto now = std::chrono::system_clock::now();
    
    auto it = impl_->entries.find(url);
    if (it != impl_->entries.end()) {
        // Update existing entry
        it->second.visitCount++;
        it->second.lastVisit = now;
        it->second.title = title;  // Update title
    } else {
        // Create new entry
        HistoryEntry entry;
        entry.id = impl_->nextId++;
        entry.url = url;
        entry.title = title;
        entry.visitCount = 1;
        entry.firstVisit = now;
        entry.lastVisit = now;
        
        impl_->entries[url] = entry;
    }
}

void HistoryDatabase::setPrivacyMode(bool enabled) {
    impl_->privacyMode = enabled;
    std::cout << "[HistoryDatabase] Privacy mode: " << (enabled ? "ON" : "OFF") << std::endl;
}

bool HistoryDatabase::isPrivacyMode() const {
    return impl_->privacyMode;
}

std::vector<HistoryEntry> HistoryDatabase::search(const std::string& query, int limit) const {
    std::vector<HistoryEntry> results;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    for (const auto& pair : impl_->entries) {
        const HistoryEntry& entry = pair.second;
        std::string lowerUrl = entry.url;
        std::string lowerTitle = entry.title;
        std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), ::tolower);
        std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::tolower);
        
        if (lowerUrl.find(lowerQuery) != std::string::npos ||
            lowerTitle.find(lowerQuery) != std::string::npos) {
            results.push_back(entry);
        }
    }
    
    // Sort by relevance (visit count and recency)
    std::sort(results.begin(), results.end(),
             [](const HistoryEntry& a, const HistoryEntry& b) {
                 return a.lastVisit > b.lastVisit;
             });
    
    if (results.size() > limit) {
        results.resize(limit);
    }
    
    return results;
}

std::vector<HistoryEntry> HistoryDatabase::getRecent(int count) const {
    std::vector<HistoryEntry> results;
    results.reserve(impl_->entries.size());
    
    for (const auto& pair : impl_->entries) {
        results.push_back(pair.second);
    }
    
    std::sort(results.begin(), results.end(),
             [](const HistoryEntry& a, const HistoryEntry& b) {
                 return a.lastVisit > b.lastVisit;
             });
    
    if (results.size() > count) {
        results.resize(count);
    }
    
    return results;
}

std::vector<HistoryEntry> HistoryDatabase::getMostVisited(int count) const {
    std::vector<HistoryEntry> results;
    results.reserve(impl_->entries.size());
    
    for (const auto& pair : impl_->entries) {
        results.push_back(pair.second);
    }
    
    std::sort(results.begin(), results.end(),
             [](const HistoryEntry& a, const HistoryEntry& b) {
                 return a.visitCount > b.visitCount;
             });
    
    if (results.size() > count) {
        results.resize(count);
    }
    
    return results;
}

std::vector<HistoryEntry> HistoryDatabase::getVisitedToday() const {
    std::vector<HistoryEntry> results;
    auto now = std::chrono::system_clock::now();
    // C++17 compatible: use hours instead of days
    using hours = std::chrono::hours;
    auto hours_since_epoch = std::chrono::duration_cast<hours>(now.time_since_epoch()).count();
    auto today_start_hours = (hours_since_epoch / 24) * 24;  // Start of current day
    
    for (const auto& pair : impl_->entries) {
        auto visit_hours = std::chrono::duration_cast<hours>(
            pair.second.lastVisit.time_since_epoch()).count();
        auto visit_day = visit_hours / 24;
        if (visit_day == today_start_hours / 24) {
            results.push_back(pair.second);
        }
    }
    
    return results;
}

HistoryEntry* HistoryDatabase::getEntry(const std::string& url) {
    auto it = impl_->entries.find(url);
    if (it != impl_->entries.end()) {
        return &it->second;
    }
    return nullptr;
}

void HistoryDatabase::clearAll() {
    impl_->entries.clear();
    std::cout << "[HistoryDatabase] Cleared all history" << std::endl;
}

void HistoryDatabase::clearRange(std::chrono::system_clock::time_point start,
                                std::chrono::system_clock::time_point end) {
    auto it = impl_->entries.begin();
    while (it != impl_->entries.end()) {
        if (it->second.lastVisit >= start && it->second.lastVisit <= end) {
            it = impl_->entries.erase(it);
        } else {
            ++it;
        }
    }
    
    std::cout << "[HistoryDatabase] Cleared history range" << std::endl;
}

void HistoryDatabase::clearUrl(const std::string& url) {
    impl_->entries.erase(url);
    std::cout << "[HistoryDatabase] Cleared URL: " << url << std::endl;
}

void HistoryDatabase::clearBefore(std::chrono::system_clock::time_point cutoff) {
    auto it = impl_->entries.begin();
    while (it != impl_->entries.end()) {
        if (it->second.lastVisit < cutoff) {
            it = impl_->entries.erase(it);
        } else {
            ++it;
        }
    }
    
    std::cout << "[HistoryDatabase] Cleared old history" << std::endl;
}

bool HistoryDatabase::load(const std::string& filepath) {
    impl_->filepath = filepath;
    // TODO: Implement JSON loading
    std::cout << "[HistoryDatabase] Load from: " << filepath << std::endl;
    return true;
}

bool HistoryDatabase::save(const std::string& filepath) {
    impl_->filepath = filepath;
    // TODO: Implement JSON saving
    std::cout << "[HistoryDatabase] Save to: " << filepath << std::endl;
    return true;
}

} // namespace ZepraBrowser
