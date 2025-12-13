/**
 * @file navigation_history.cpp
 * @brief Navigation history implementation
 */

#include "webcore/navigation_history.hpp"
#include <chrono>

namespace Zepra::WebCore {

NavigationHistory::NavigationHistory() = default;
NavigationHistory::~NavigationHistory() = default;

void NavigationHistory::push(const std::string& url, const std::string& title) {
    // If we're not at the end, truncate forward history
    if (currentIndex_ < static_cast<int>(entries_.size()) - 1) {
        entries_.erase(entries_.begin() + currentIndex_ + 1, entries_.end());
    }
    
    // Don't add duplicate consecutive entries
    if (!entries_.empty() && entries_.back().url == url) {
        return;
    }
    
    HistoryEntry entry;
    entry.url = url;
    entry.title = title.empty() ? url : title;
    entry.visitTime = std::chrono::system_clock::now();
    
    entries_.push_back(entry);
    currentIndex_ = entries_.size() - 1;
    
    if (onChange_) onChange_();
}

std::string NavigationHistory::goBack() {
    if (!canGoBack()) return "";
    
    currentIndex_--;
    if (onChange_) onChange_();
    
    return entries_[currentIndex_].url;
}

std::string NavigationHistory::goForward() {
    if (!canGoForward()) return "";
    
    currentIndex_++;
    if (onChange_) onChange_();
    
    return entries_[currentIndex_].url;
}

const HistoryEntry* NavigationHistory::current() const {
    if (currentIndex_ >= 0 && currentIndex_ < static_cast<int>(entries_.size())) {
        return &entries_[currentIndex_];
    }
    return nullptr;
}

const HistoryEntry* NavigationHistory::entryAt(int offset) const {
    int index = currentIndex_ + offset;
    if (index >= 0 && index < static_cast<int>(entries_.size())) {
        return &entries_[index];
    }
    return nullptr;
}

void NavigationHistory::clear() {
    entries_.clear();
    currentIndex_ = -1;
    if (onChange_) onChange_();
}

void NavigationHistory::updateTitle(const std::string& title) {
    if (currentIndex_ >= 0 && currentIndex_ < static_cast<int>(entries_.size())) {
        entries_[currentIndex_].title = title;
    }
}

void NavigationHistory::updateScrollPosition(const std::string& scroll) {
    if (currentIndex_ >= 0 && currentIndex_ < static_cast<int>(entries_.size())) {
        entries_[currentIndex_].scrollPosition = scroll;
    }
}

} // namespace Zepra::WebCore
