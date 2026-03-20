// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file monitored_http.cpp
 * @brief Implementation of MonitoredHttpClient
 */

#include "monitored_http.h"
#include <chrono>

namespace zepra {

// ============================================================================
// MonitoredHttpClient Implementation
// ============================================================================

MonitoredHttpClient::MonitoredHttpClient() {
#ifdef USE_ZEPRA_STACK
    client_ = nx_http_client_create(nullptr);
#endif
}

MonitoredHttpClient::~MonitoredHttpClient() {
#ifdef USE_ZEPRA_STACK
    if (client_) {
        nx_http_client_free(client_);
    }
#endif
}

uint64_t MonitoredHttpClient::recordRequest(const std::string& method, 
                                             const std::string& url,
                                             const std::string& body) {
    NetworkRequest req;
    req.method = method;
    req.url = url;
    req.origin = origin_;
    req.body_size = body.size();
    req.start_time = std::chrono::system_clock::now();
    req.is_fetch = true;
    
    return getNetworkMonitor().recordRequest(req);
}

void MonitoredHttpClient::recordResponse(uint64_t id, int status, 
                                          const std::string& statusText,
                                          size_t bodyLen, 
                                          const std::string& contentType) {
    NetworkResponse res;
    res.request_id = id;
    res.status_code = status;
    res.status_text = statusText;
    res.content_length = bodyLen;
    res.content_type = contentType;
    res.end_time = std::chrono::system_clock::now();
    
    getNetworkMonitor().recordResponse(id, res);
}

void MonitoredHttpClient::recordError(uint64_t id, const std::string& error) {
    getNetworkMonitor().recordError(id, error);
}

MonitoredHttpClient::Response MonitoredHttpClient::get(const std::string& url) {
    Response result;
    
    // Record request to NetworkMonitor
    uint64_t reqId = recordRequest("GET", url);
    auto startTime = std::chrono::steady_clock::now();
    
#ifdef USE_ZEPRA_STACK
    if (!client_) {
        result.error = "HTTP client not initialized";
        recordError(reqId, result.error);
        return result;
    }
    
    NxHttpError err;
    NxHttpRequest* req = nx_http_request_create(NX_HTTP_GET, url.c_str());
    if (!req) {
        result.error = "Failed to create request";
        recordError(reqId, result.error);
        return result;
    }
    
    NxHttpResponse* res = nx_http_client_send(client_, req, &err);
    nx_http_request_free(req);
    
    if (!res) {
        result.error = nx_http_error_string(err);
        recordError(reqId, result.error);
        return result;
    }
    
    // Extract response data
    result.status = nx_http_response_status(res);
    result.statusText = nx_http_response_status_text(res) ? 
                        nx_http_response_status_text(res) : "";
    result.body = nx_http_response_body_string(res) ? 
                  nx_http_response_body_string(res) : "";
    result.bodyLength = nx_http_response_body_len(res);
    
    const char* ct = nx_http_response_header(res, "Content-Type");
    result.contentType = ct ? ct : "";
    
    nx_http_response_free(res);
    
    // Record response to NetworkMonitor
    recordResponse(reqId, result.status, result.statusText, 
                   result.bodyLength, result.contentType);
    
#else
    // Fallback: no NxHTTP available
    result.error = "NxHTTP not available (USE_ZEPRA_STACK not defined)";
    recordError(reqId, result.error);
#endif
    
    return result;
}

MonitoredHttpClient::Response MonitoredHttpClient::post(const std::string& url, 
                                                         const std::string& body,
                                                         const std::string& contentType) {
    Response result;
    
    // Record request to NetworkMonitor
    uint64_t reqId = recordRequest("POST", url, body);
    
#ifdef USE_ZEPRA_STACK
    if (!client_) {
        result.error = "HTTP client not initialized";
        recordError(reqId, result.error);
        return result;
    }
    
    NxHttpError err;
    NxHttpRequest* req = nx_http_request_create(NX_HTTP_POST, url.c_str());
    if (!req) {
        result.error = "Failed to create request";
        recordError(reqId, result.error);
        return result;
    }
    
    nx_http_request_set_header(req, "Content-Type", contentType.c_str());
    nx_http_request_set_body_string(req, body.c_str());
    
    NxHttpResponse* res = nx_http_client_send(client_, req, &err);
    nx_http_request_free(req);
    
    if (!res) {
        result.error = nx_http_error_string(err);
        recordError(reqId, result.error);
        return result;
    }
    
    // Extract response data
    result.status = nx_http_response_status(res);
    result.statusText = nx_http_response_status_text(res) ? 
                        nx_http_response_status_text(res) : "";
    result.body = nx_http_response_body_string(res) ? 
                  nx_http_response_body_string(res) : "";
    result.bodyLength = nx_http_response_body_len(res);
    
    const char* ct = nx_http_response_header(res, "Content-Type");
    result.contentType = ct ? ct : "";
    
    nx_http_response_free(res);
    
    // Record response to NetworkMonitor
    recordResponse(reqId, result.status, result.statusText, 
                   result.bodyLength, result.contentType);
    
#else
    result.error = "NxHTTP not available";
    recordError(reqId, result.error);
#endif
    
    return result;
}

// ============================================================================
// Global convenience functions
// ============================================================================

MonitoredHttpClient::Response monitoredGet(const std::string& url) {
    MonitoredHttpClient client;
    return client.get(url);
}

MonitoredHttpClient::Response monitoredPost(const std::string& url, 
                                            const std::string& body,
                                            const std::string& contentType) {
    MonitoredHttpClient client;
    return client.post(url, body, contentType);
}

} // namespace zepra
