// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file console_log.cpp
 * @brief Console log implementation
 */

#include "engine/console_log.h"
#include <algorithm>

namespace zepra {

ConsoleLog::ConsoleLog() {}

ConsoleLog& ConsoleLog::instance() {
    static ConsoleLog instance;
    return instance;
}

void ConsoleLog::addEntry(ConsoleLogLevel level, const std::string& message,
                          const std::string& source, int line) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ConsoleLogEntry entry;
    entry.level = level;
    entry.message = message;
    entry.source = source;
    entry.line = line;
    entry.timestamp = std::chrono::system_clock::now();
    
    entries_.push_back(entry);
    
    // Remove oldest if over limit
    while (entries_.size() > max_entries_) {
        entries_.erase(entries_.begin());
    }
    
    if (callback_) {
        callback_(entry);
    }
}

void ConsoleLog::log(const std::string& message, const std::string& source) {
    addEntry(ConsoleLogLevel::LOG, message, source);
}

void ConsoleLog::info(const std::string& message, const std::string& source) {
    addEntry(ConsoleLogLevel::INFO, message, source);
}

void ConsoleLog::warn(const std::string& message, const std::string& source) {
    addEntry(ConsoleLogLevel::WARN, message, source);
}

void ConsoleLog::error(const std::string& message, const std::string& source) {
    addEntry(ConsoleLogLevel::ERROR, message, source);
}

void ConsoleLog::debug(const std::string& message, const std::string& source) {
    addEntry(ConsoleLogLevel::DEBUG, message, source);
}

std::vector<ConsoleLogEntry> ConsoleLog::getEntries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_;
}

std::vector<ConsoleLogEntry> ConsoleLog::getLastN(size_t n) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (entries_.size() <= n) return entries_;
    return std::vector<ConsoleLogEntry>(entries_.end() - n, entries_.end());
}

size_t ConsoleLog::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

std::vector<ConsoleLogEntry> ConsoleLog::filterByLevel(ConsoleLogLevel level) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ConsoleLogEntry> result;
    for (const auto& e : entries_) {
        if (e.level == level) result.push_back(e);
    }
    return result;
}

void ConsoleLog::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

void ConsoleLog::setCallback(LogCallback callback) {
    callback_ = callback;
}

void ConsoleLog::setMaxEntries(size_t max) {
    max_entries_ = max;
}

} // namespace zepra
