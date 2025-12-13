/**
 * @file network_panel.cpp
 * @brief Network requests panel implementation
 */

#include "devtools/network_panel.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Zepra::DevTools {

NetworkPanel::NetworkPanel() : DevToolsPanel("Network") {}

NetworkPanel::~NetworkPanel() = default;

void NetworkPanel::render() {
    // Network panel renders request list
    // Each entry shown with: status, method, URL, type, size, time
}

void NetworkPanel::refresh() {
    // Just triggers re-render
}

void NetworkPanel::addRequest(const NetworkRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    requests_.push_back(request);
    if (onRequestAdded_) {
        onRequestAdded_(request);
    }
}

void NetworkPanel::updateRequest(int requestId, const NetworkRequest& update) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& req : requests_) {
        if (req.id == requestId) {
            req.status = update.status;
            req.statusText = update.statusText;
            req.responseHeaders = update.responseHeaders;
            req.responseBody = update.responseBody;
            req.responseSize = update.responseSize;
            req.endTime = update.endTime;
            req.duration = update.duration;
            break;
        }
    }
    
    if (onRequestUpdated_) {
        onRequestUpdated_(requestId);
    }
}

void NetworkPanel::clearRequests() {
    std::lock_guard<std::mutex> lock(mutex_);
    requests_.clear();
    selectedRequestId_ = -1;
}

std::vector<NetworkRequest> NetworkPanel::getAllRequests() {
    std::lock_guard<std::mutex> lock(mutex_);
    return requests_;
}

std::vector<NetworkRequest> NetworkPanel::getFilteredRequests() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (filter_.empty() && resourceTypeFilter_.empty()) {
        return requests_;
    }
    
    std::vector<NetworkRequest> filtered;
    for (const auto& req : requests_) {
        // URL filter
        if (!filter_.empty() && 
            req.url.find(filter_) == std::string::npos) {
            continue;
        }
        
        // Resource type filter
        if (!resourceTypeFilter_.empty() && 
            req.resourceType != resourceTypeFilter_) {
            continue;
        }
        
        filtered.push_back(req);
    }
    
    return filtered;
}

const NetworkRequest* NetworkPanel::getRequest(int requestId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& req : requests_) {
        if (req.id == requestId) {
            return &req;
        }
    }
    return nullptr;
}

void NetworkPanel::selectRequest(int requestId) {
    selectedRequestId_ = requestId;
    if (onRequestSelected_) {
        onRequestSelected_(requestId);
    }
}

void NetworkPanel::setFilter(const std::string& filter) {
    filter_ = filter;
}

void NetworkPanel::setResourceTypeFilter(const std::string& type) {
    resourceTypeFilter_ = type;
}

NetworkStats NetworkPanel::getStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    NetworkStats stats;
    stats.totalRequests = requests_.size();
    
    for (const auto& req : requests_) {
        stats.totalSize += req.responseSize;
        stats.totalDuration += req.duration;
        
        if (req.status >= 200 && req.status < 300) {
            stats.successCount++;
        } else if (req.status >= 400) {
            stats.errorCount++;
        }
        
        if (req.fromCache) {
            stats.cacheHits++;
        }
    }
    
    if (!requests_.empty()) {
        stats.avgDuration = stats.totalDuration / requests_.size();
    }
    
    return stats;
}

std::string NetworkPanel::formatSize(size_t bytes) {
    std::ostringstream oss;
    
    if (bytes < 1024) {
        oss << bytes << " B";
    } else if (bytes < 1024 * 1024) {
        oss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
    } else {
        oss << std::fixed << std::setprecision(1) << (bytes / (1024.0 * 1024.0)) << " MB";
    }
    
    return oss.str();
}

std::string NetworkPanel::formatDuration(double ms) {
    std::ostringstream oss;
    
    if (ms < 1000) {
        oss << std::fixed << std::setprecision(0) << ms << " ms";
    } else {
        oss << std::fixed << std::setprecision(2) << (ms / 1000.0) << " s";
    }
    
    return oss.str();
}

} // namespace Zepra::DevTools
