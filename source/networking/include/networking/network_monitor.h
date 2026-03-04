/**
 * @file network_monitor.h
 * @brief Network request/response monitor for ZepraWebView
 */

#ifndef NETWORK_MONITOR_H
#define NETWORK_MONITOR_H

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <functional>
#include <mutex>

namespace zepra {

// ============================================================================
// Network Request/Response Data
// ============================================================================

struct NetworkHeader {
    std::string name;
    std::string value;
};

struct NetworkRequest {
    uint64_t id;
    std::string method;
    std::string url;
    std::string origin;
    std::vector<NetworkHeader> headers;
    std::string body;
    size_t body_size = 0;
    std::chrono::system_clock::time_point start_time;
    
    // Flags
    bool is_xhr = false;
    bool is_fetch = false;
    bool is_cors = false;
    bool blocked = false;
    std::string blocked_reason;
};

struct NetworkResponse {
    uint64_t request_id;
    int status_code;
    std::string status_text;
    std::vector<NetworkHeader> headers;
    std::string content_type;
    size_t content_length = 0;
    std::chrono::system_clock::time_point end_time;
    double duration_ms = 0;
    
    // Body (truncated for display)
    std::string body_preview;
    bool body_truncated = false;
    
    // Security
    bool is_secure = false;           // HTTPS
    std::string security_protocol;    // TLS version
    std::string certificate_issuer;
};

enum class NetworkResourceType {
    DOCUMENT,
    STYLESHEET,
    SCRIPT,
    IMAGE,
    FONT,
    XHR,
    FETCH,
    WEBSOCKET,
    MEDIA,
    OTHER
};

struct NetworkEntry {
    NetworkRequest request;
    NetworkResponse response;
    NetworkResourceType type = NetworkResourceType::OTHER;
    bool completed = false;
    bool failed = false;
    std::string error;
};

// ============================================================================
// Network Monitor
// ============================================================================

class NetworkMonitor {
public:
    using RequestCallback = std::function<void(const NetworkRequest&)>;
    using ResponseCallback = std::function<void(const NetworkEntry&)>;
    
    NetworkMonitor();
    ~NetworkMonitor();
    
    // Enable/disable monitoring
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    // Callbacks
    void setRequestCallback(RequestCallback cb);
    void setResponseCallback(ResponseCallback cb);
    
    // Record requests/responses
    uint64_t recordRequest(const NetworkRequest& req);
    void recordResponse(uint64_t request_id, const NetworkResponse& res);
    void recordError(uint64_t request_id, const std::string& error);
    
    // Access entries
    const std::vector<NetworkEntry>& getEntries() const;
    std::vector<NetworkEntry> getEntriesForOrigin(const std::string& origin) const;
    
    // Clear
    void clear();
    
    // Stats
    size_t getTotalRequests() const;
    size_t getBlockedRequests() const;
    double getTotalTransferSize() const;
    
    // Filter
    std::vector<NetworkEntry> filterByType(NetworkResourceType type) const;
    std::vector<NetworkEntry> filterByStatus(int min_status, int max_status) const;
    
    // Export
    std::string exportAsHAR() const;  // HTTP Archive format
    
private:
    std::vector<NetworkEntry> entries_;
    std::map<uint64_t, size_t> id_to_index_;
    uint64_t next_id_ = 1;
    bool enabled_ = true;
    mutable std::mutex mutex_;
    
    RequestCallback request_callback_;
    ResponseCallback response_callback_;
};

// Global network monitor instance
NetworkMonitor& getNetworkMonitor();

// ============================================================================
// Helper: Format for display
// ============================================================================

std::string formatHeaders(const std::vector<NetworkHeader>& headers);
std::string formatDuration(double ms);
std::string formatSize(size_t bytes);
NetworkResourceType guessResourceType(const std::string& url, const std::string& content_type);

} // namespace zepra

#endif // NETWORK_MONITOR_H
