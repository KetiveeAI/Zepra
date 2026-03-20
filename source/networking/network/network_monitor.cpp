// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file network_monitor.cpp
 * @brief Network monitor implementation for ZepraWebView
 */

#include "network_monitor.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace zepra {

NetworkMonitor::NetworkMonitor() {}

NetworkMonitor::~NetworkMonitor() {}

void NetworkMonitor::setEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = enabled;
}

bool NetworkMonitor::isEnabled() const {
    return enabled_;
}

void NetworkMonitor::setRequestCallback(RequestCallback cb) {
    request_callback_ = cb;
}

void NetworkMonitor::setResponseCallback(ResponseCallback cb) {
    response_callback_ = cb;
}

uint64_t NetworkMonitor::recordRequest(const NetworkRequest& req) {
    if (!enabled_) return 0;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    NetworkEntry entry;
    entry.request = req;
    entry.request.id = next_id_++;
    entry.request.start_time = std::chrono::system_clock::now();
    entry.type = guessResourceType(req.url, "");
    
    size_t index = entries_.size();
    entries_.push_back(entry);
    id_to_index_[entry.request.id] = index;
    
    if (request_callback_) {
        request_callback_(entry.request);
    }
    
    return entry.request.id;
}

void NetworkMonitor::recordResponse(uint64_t request_id, const NetworkResponse& res) {
    if (!enabled_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = id_to_index_.find(request_id);
    if (it == id_to_index_.end()) return;
    
    NetworkEntry& entry = entries_[it->second];
    entry.response = res;
    entry.response.request_id = request_id;
    entry.response.end_time = std::chrono::system_clock::now();
    
    // Calculate duration
    auto duration = entry.response.end_time - entry.request.start_time;
    entry.response.duration_ms = std::chrono::duration<double, std::milli>(duration).count();
    
    // Update type based on content-type
    entry.type = guessResourceType(entry.request.url, entry.response.content_type);
    entry.completed = true;
    
    if (response_callback_) {
        response_callback_(entry);
    }
}

void NetworkMonitor::recordError(uint64_t request_id, const std::string& error) {
    if (!enabled_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = id_to_index_.find(request_id);
    if (it == id_to_index_.end()) return;
    
    NetworkEntry& entry = entries_[it->second];
    entry.failed = true;
    entry.error = error;
    entry.completed = true;
    
    if (response_callback_) {
        response_callback_(entry);
    }
}

const std::vector<NetworkEntry>& NetworkMonitor::getEntries() const {
    return entries_;
}

std::vector<NetworkEntry> NetworkMonitor::getEntriesForOrigin(const std::string& origin) const {
    std::vector<NetworkEntry> result;
    for (const auto& entry : entries_) {
        if (entry.request.origin == origin) {
            result.push_back(entry);
        }
    }
    return result;
}

void NetworkMonitor::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
    id_to_index_.clear();
}

size_t NetworkMonitor::getTotalRequests() const {
    return entries_.size();
}

size_t NetworkMonitor::getBlockedRequests() const {
    size_t count = 0;
    for (const auto& entry : entries_) {
        if (entry.request.blocked) count++;
    }
    return count;
}

double NetworkMonitor::getTotalTransferSize() const {
    double total = 0;
    for (const auto& entry : entries_) {
        total += entry.response.content_length;
    }
    return total;
}

std::vector<NetworkEntry> NetworkMonitor::filterByType(NetworkResourceType type) const {
    std::vector<NetworkEntry> result;
    for (const auto& entry : entries_) {
        if (entry.type == type) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<NetworkEntry> NetworkMonitor::filterByStatus(int min_status, int max_status) const {
    std::vector<NetworkEntry> result;
    for (const auto& entry : entries_) {
        int status = entry.response.status_code;
        if (status >= min_status && status <= max_status) {
            result.push_back(entry);
        }
    }
    return result;
}

std::string NetworkMonitor::exportAsHAR() const {
    // Simplified HAR export
    std::stringstream ss;
    ss << "{\n  \"log\": {\n    \"entries\": [\n";
    
    bool first = true;
    for (const auto& entry : entries_) {
        if (!first) ss << ",\n";
        first = false;
        
        ss << "      {\n";
        ss << "        \"request\": {\n";
        ss << "          \"method\": \"" << entry.request.method << "\",\n";
        ss << "          \"url\": \"" << entry.request.url << "\"\n";
        ss << "        },\n";
        ss << "        \"response\": {\n";
        ss << "          \"status\": " << entry.response.status_code << ",\n";
        ss << "          \"statusText\": \"" << entry.response.status_text << "\"\n";
        ss << "        },\n";
        ss << "        \"time\": " << entry.response.duration_ms << "\n";
        ss << "      }";
    }
    
    ss << "\n    ]\n  }\n}";
    return ss.str();
}

// Global instance
static NetworkMonitor g_network_monitor;

NetworkMonitor& getNetworkMonitor() {
    return g_network_monitor;
}

// ============================================================================
// Helpers
// ============================================================================

std::string formatHeaders(const std::vector<NetworkHeader>& headers) {
    std::stringstream ss;
    for (const auto& h : headers) {
        ss << h.name << ": " << h.value << "\n";
    }
    return ss.str();
}

std::string formatDuration(double ms) {
    if (ms < 1) {
        return std::to_string(static_cast<int>(ms * 1000)) + " μs";
    } else if (ms < 1000) {
        return std::to_string(static_cast<int>(ms)) + " ms";
    } else {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << (ms / 1000) << " s";
        return ss.str();
    }
}

std::string formatSize(size_t bytes) {
    if (bytes < 1024) {
        return std::to_string(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
        return ss.str();
    } else {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << (bytes / (1024.0 * 1024.0)) << " MB";
        return ss.str();
    }
}

NetworkResourceType guessResourceType(const std::string& url, const std::string& content_type) {
    // Check content-type first
    if (!content_type.empty()) {
        if (content_type.find("text/html") != std::string::npos) return NetworkResourceType::DOCUMENT;
        if (content_type.find("text/css") != std::string::npos) return NetworkResourceType::STYLESHEET;
        if (content_type.find("javascript") != std::string::npos) return NetworkResourceType::SCRIPT;
        if (content_type.find("image/") != std::string::npos) return NetworkResourceType::IMAGE;
        if (content_type.find("font/") != std::string::npos) return NetworkResourceType::FONT;
        if (content_type.find("video/") != std::string::npos) return NetworkResourceType::MEDIA;
        if (content_type.find("audio/") != std::string::npos) return NetworkResourceType::MEDIA;
        if (content_type.find("json") != std::string::npos) return NetworkResourceType::XHR;
    }
    
    // Guess from URL extension
    std::string lower = url;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower.find(".html") != std::string::npos || lower.find(".htm") != std::string::npos) {
        return NetworkResourceType::DOCUMENT;
    }
    if (lower.find(".css") != std::string::npos) return NetworkResourceType::STYLESHEET;
    if (lower.find(".js") != std::string::npos) return NetworkResourceType::SCRIPT;
    if (lower.find(".png") != std::string::npos || 
        lower.find(".jpg") != std::string::npos ||
        lower.find(".gif") != std::string::npos ||
        lower.find(".svg") != std::string::npos ||
        lower.find(".webp") != std::string::npos ||
        lower.find(".ico") != std::string::npos) {
        return NetworkResourceType::IMAGE;
    }
    if (lower.find(".woff") != std::string::npos || 
        lower.find(".ttf") != std::string::npos ||
        lower.find(".otf") != std::string::npos) {
        return NetworkResourceType::FONT;
    }
    if (lower.find(".mp4") != std::string::npos || 
        lower.find(".webm") != std::string::npos ||
        lower.find(".mp3") != std::string::npos) {
        return NetworkResourceType::MEDIA;
    }
    
    return NetworkResourceType::OTHER;
}

} // namespace zepra
