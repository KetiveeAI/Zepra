/**
 * @file network_panel.hpp
 * @brief Network Panel - HTTP requests, WebSocket, timing
 */

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <memory>
#include <unordered_map>

namespace Zepra::DevTools {

/**
 * @brief HTTP request method
 */
enum class HttpMethod {
    GET, POST, PUT, DELETE_,
    PATCH, HEAD, OPTIONS, CONNECT
};

/**
 * @brief Resource type
 */
enum class ResourceType {
    Document,
    Stylesheet,
    Script,
    Image,
    Font,
    XHR,
    Fetch,
    WebSocket,
    Media,
    Other
};

/**
 * @brief Request timing information
 */
struct RequestTiming {
    double dnsStart;
    double dnsEnd;
    double connectStart;
    double connectEnd;
    double sslStart;
    double sslEnd;
    double sendStart;
    double sendEnd;
    double waitingStart;
    double waitingEnd;
    double receiveStart;
    double receiveEnd;
    double totalTime;
};

/**
 * @brief HTTP header
 */
struct HttpHeader {
    std::string name;
    std::string value;
};

/**
 * @brief Network request
 */
struct NetworkRequest {
    int id;
    std::string url;
    HttpMethod method;
    ResourceType type;
    
    // Request
    std::vector<HttpHeader> requestHeaders;
    std::string requestBody;
    size_t requestSize;
    
    // Response
    int statusCode;
    std::string statusText;
    std::vector<HttpHeader> responseHeaders;
    std::string responseBody;
    size_t responseSize;
    std::string mimeType;
    
    // Timing
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    RequestTiming timing;
    
    // State
    bool completed;
    bool failed;
    std::string errorMessage;
    bool cached;
    std::string initiator;      // URL that initiated request
    int initiatorLine;
    
    // Preview
    bool previewAvailable;
    std::string previewText;
};

/**
 * @brief Network filter options
 */
struct NetworkFilter {
    std::string urlFilter;
    std::vector<ResourceType> types;
    bool hideDataUrls = true;
    bool showOnlyFailed = false;
    int minSize = 0;
    int maxSize = 0;
};

/**
 * @brief Network request callback
 */
using NetworkCallback = std::function<void(const NetworkRequest&)>;

/**
 * @brief Network Panel - HTTP Inspector
 */
class NetworkPanel {
public:
    NetworkPanel();
    ~NetworkPanel();
    
    // --- Requests ---
    
    /**
     * @brief Add/update a request
     */
    void addRequest(const NetworkRequest& request);
    void updateRequest(int id, const NetworkRequest& request);
    
    /**
     * @brief Get all requests
     */
    const std::vector<NetworkRequest>& requests() const { return requests_; }
    
    /**
     * @brief Get filtered requests
     */
    std::vector<NetworkRequest> filteredRequests() const;
    
    /**
     * @brief Clear all requests
     */
    void clear();
    
    /**
     * @brief Get request by ID
     */
    const NetworkRequest* getRequest(int id) const;
    
    // --- Selection ---
    
    /**
     * @brief Select a request
     */
    void selectRequest(int id);
    int selectedRequest() const { return selectedRequestId_; }
    
    // --- Filtering ---
    
    void setFilter(const NetworkFilter& filter);
    NetworkFilter filter() const { return filter_; }
    
    // --- Statistics ---
    
    struct Stats {
        int totalRequests;
        int failedRequests;
        size_t totalTransferred;
        size_t totalSize;
        double totalTime;
        std::unordered_map<ResourceType, int> byType;
    };
    
    Stats getStats() const;
    
    // --- Recording ---
    
    void startRecording();
    void stopRecording();
    bool isRecording() const { return recording_; }
    
    /**
     * @brief Preserve log on navigation
     */
    void setPreserveLog(bool preserve);
    bool preserveLog() const { return preserveLog_; }
    
    /**
     * @brief Disable cache while DevTools open
     */
    void setDisableCache(bool disable);
    bool disableCache() const { return disableCache_; }
    
    // --- Throttling ---
    
    enum class ThrottlePreset {
        None,
        Slow3G,
        Fast3G,
        Offline
    };
    
    void setThrottle(ThrottlePreset preset);
    ThrottlePreset throttle() const { return throttle_; }
    
    // --- Export ---
    
    std::string exportAsHAR() const;
    void importFromHAR(const std::string& har);
    
    // --- Callbacks ---
    void onRequest(NetworkCallback callback);
    
    // --- UI ---
    void update();
    void render();
    
private:
    void renderRequestList();
    void renderRequestDetails();
    void renderTimingWaterfall();
    std::string formatSize(size_t bytes);
    std::string formatTime(double ms);
    
    std::vector<NetworkRequest> requests_;
    int selectedRequestId_ = -1;
    int nextRequestId_ = 1;
    
    NetworkFilter filter_;
    bool recording_ = true;
    bool preserveLog_ = false;
    bool disableCache_ = false;
    ThrottlePreset throttle_ = ThrottlePreset::None;
    
    std::vector<NetworkCallback> callbacks_;
};

} // namespace Zepra::DevTools
