// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file monitored_http.h
 * @brief HTTP client wrapper that records to NetworkMonitor
 * 
 * Bridges NxHTTP to ZepraWebView's NetworkMonitor for real-time
 * request/response tracking in the Network tab.
 */

#ifndef MONITORED_HTTP_H
#define MONITORED_HTTP_H

#include "network_monitor.h"

// Include NxHTTP if USE_ZEPRA_STACK is enabled
#ifdef USE_ZEPRA_STACK
#include "nxhttp.h"
#endif

#include <string>
#include <chrono>
#include <functional>

namespace zepra {

/**
 * HTTP client that automatically records all requests to NetworkMonitor.
 * Use this instead of raw NxHTTP for monitored requests.
 */
class MonitoredHttpClient {
public:
    MonitoredHttpClient();
    ~MonitoredHttpClient();
    
    // Perform GET request (recorded to NetworkMonitor)
    struct Response {
        int status = 0;
        std::string statusText;
        std::string body;
        size_t bodyLength = 0;
        std::string contentType;
        bool ok() const { return status >= 200 && status < 300; }
        std::string error;
    };
    
    Response get(const std::string& url);
    Response post(const std::string& url, const std::string& body, 
                  const std::string& contentType = "application/json");
    
    // Set origin for CORS tracking
    void setOrigin(const std::string& origin) { origin_ = origin; }
    
private:
    std::string origin_;
    
#ifdef USE_ZEPRA_STACK
    NxHttpClient* client_ = nullptr;
#endif
    
    // Helper: Record request to NetworkMonitor
    uint64_t recordRequest(const std::string& method, const std::string& url,
                           const std::string& body = "");
    
    // Helper: Record response to NetworkMonitor
    void recordResponse(uint64_t id, int status, const std::string& statusText,
                        size_t bodyLen, const std::string& contentType);
    
    // Helper: Record error
    void recordError(uint64_t id, const std::string& error);
};

// ============================================================================
// Global convenience functions (with monitoring)
// ============================================================================

/**
 * Perform a GET request with automatic NetworkMonitor recording.
 */
MonitoredHttpClient::Response monitoredGet(const std::string& url);

/**
 * Perform a POST request with automatic NetworkMonitor recording.
 */
MonitoredHttpClient::Response monitoredPost(const std::string& url, 
                                            const std::string& body,
                                            const std::string& contentType = "application/json");

} // namespace zepra

#endif // MONITORED_HTTP_H
