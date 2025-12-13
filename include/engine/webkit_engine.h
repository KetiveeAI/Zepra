#pragma once

#include "../common/types.h"
#include "../common/constants.h"
#include "dev_tools.h"
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <chrono>

// Prevent Windows.h macros from interfering
#ifdef STRICT
#undef STRICT
#endif

// WebKit forward declarations
namespace WebKit {
    class WebPage;
    class WebFrame;
    class WebView;
    class WebInspector;
    class WebSecurityOrigin;
    class WebURLRequest;
    class WebURLResponse;
    class WebResourceLoadDelegate;
}

namespace zepra {

// WebKit Engine Configuration
struct WebKitConfig {
    bool enableJavaScript = true;
    bool enablePlugins = false;
    bool enableWebGL = true;
    bool enableWebAudio = true;
    bool enableWebRTC = true;
    bool enableServiceWorkers = true;
    bool enableNotifications = true;
    bool enableGeolocation = true;
    bool enableCamera = false;
    bool enableMicrophone = false;
    bool enableDevTools = true;
    bool enableNodeIntegration = false;
    String userAgent = "Zepra Browser/1.0";
    String defaultEncoding = "UTF-8";
    int maxMemoryUsage = 512; // MB
    int maxConcurrentRequests = 10;
    bool enableCaching = true;
    bool enableCompression = true;
    String cacheDirectory = "./cache";
    String tempDirectory = "./temp";
    
    WebKitConfig() = default;
};

// WebKit Page Load States
enum class WebKitLoadState {
    UNINITIALIZED,
    LOADING,
    LOADED,
    FAILED,
    CANCELLED,
    TIMEOUT,
    NETWORK_ERROR,
    SSL_ERROR
};

// WebKit Navigation Types
enum class WebKitNavigationType {
    LINK_CLICKED,
    FORM_SUBMITTED,
    BACK_FORWARD,
    RELOAD,
    OTHER,
    REDIRECT,
    HISTORY_API
};

// WebKit Security Policy
enum class WebKitSecurityPolicy {
    ALLOW_ALL,
    BLOCK_MIXED_CONTENT,
    BLOCK_INSECURE_CONTENT,
    WEBKIT_STRICT,
    CUSTOM
};

// WebKit Content Security Policy
enum class WebKitCSPLevel {
    NONE,
    BASIC,
    WEBKIT_STRICT,
    CUSTOM
};

// WebKit Performance Metrics
struct WebKitPerformanceMetrics {
    double domContentLoadedTime = 0.0;
    double loadCompleteTime = 0.0;
    double firstPaintTime = 0.0;
    double firstContentfulPaintTime = 0.0;
    double largestContentfulPaintTime = 0.0;
    double cumulativeLayoutShift = 0.0;
    double firstInputDelay = 0.0;
    int totalBytesTransferred = 0;
    int totalRequests = 0;
    double averageResponseTime = 0.0;
    
    WebKitPerformanceMetrics() = default;
};

// WebKit Network Request
struct WebKitNetworkRequest {
    String url;
    String method;
    String referrer;
    std::unordered_map<String, String> headers;
    String postData;
    bool isMainFrame;
    bool isXHR;
    bool isFetch;
    
    WebKitNetworkRequest() : isMainFrame(false), isXHR(false), isFetch(false) {}
};

// WebKit Network Response
struct WebKitNetworkResponse {
    String url;
    int statusCode;
    String statusText;
    std::unordered_map<String, String> headers;
    String mimeType;
    int contentLength;
    bool fromCache;
    double responseTime;
    
    WebKitNetworkResponse() : statusCode(0), contentLength(0), fromCache(false), responseTime(0.0) {}
};

// WebKit Security Information
struct WebKitSecurityInfo {
    String protocol;
    String certificate;
    String issuer;
    bool isValid;
    bool isSecure;
    String warnings;
    
    WebKitSecurityInfo() : isValid(false), isSecure(false) {}
};

// WebKit Engine Events
struct WebKitLoadEvent {
    WebKitLoadState state;
    String url;
    String errorMessage;
    int httpStatusCode;
    bool isMainFrame;
    WebKitPerformanceMetrics metrics;
    WebKitSecurityInfo securityInfo;
    
    WebKitLoadEvent() : state(WebKitLoadState::UNINITIALIZED), httpStatusCode(0), isMainFrame(false) {}
};

struct WebKitNavigationEvent {
    WebKitNavigationType type;
    String fromUrl;
    String toUrl;
    bool isRedirect;
    bool isUserGesture;
    String referrer;
    std::chrono::system_clock::time_point timestamp;
    
    WebKitNavigationEvent() : type(WebKitNavigationType::OTHER), isRedirect(false), isUserGesture(false) {}
};

struct WebKitSecurityEvent {
    WebKitSecurityPolicy policy;
    String url;
    String reason;
    bool isBlocked;
    WebKitSecurityInfo securityInfo;
    
    WebKitSecurityEvent() : policy(WebKitSecurityPolicy::ALLOW_ALL), isBlocked(false) {}
};

struct WebKitNetworkEvent {
    WebKitNetworkRequest request;
    WebKitNetworkResponse response;
    bool isBlocked;
    String blockReason;
    
    WebKitNetworkEvent() : isBlocked(false) {}
};

struct WebKitConsoleEvent {
    String message;
    String source;
    int line;
    int column;
    String level; // "log", "info", "warn", "error", "debug"
    std::chrono::system_clock::time_point timestamp;
    
    WebKitConsoleEvent() : line(0), column(0) {}
};

// WebKit Engine Callbacks
using WebKitLoadCallback = std::function<void(const WebKitLoadEvent&)>;
using WebKitNavigationCallback = std::function<void(const WebKitNavigationEvent&)>;
using WebKitSecurityCallback = std::function<void(const WebKitSecurityEvent&)>;
using WebKitNetworkCallback = std::function<void(const WebKitNetworkEvent&)>;
using WebKitConsoleCallback = std::function<void(const WebKitConsoleEvent&)>;
using WebKitAlertCallback = std::function<void(const String& message)>;
using WebKitConfirmCallback = std::function<bool(const String& message)>;
using WebKitPromptCallback = std::function<String(const String& message, const String& defaultValue)>;
using WebKitBeforeLoadCallback = std::function<bool(const WebKitNetworkRequest&)>;
using WebKitResourceLoadCallback = std::function<void(const WebKitNetworkRequest&, const WebKitNetworkResponse&)>;

// WebKit Page Events
enum class WebKitEventType {
    LOAD_STARTED,
    LOAD_COMMITTED,
    LOAD_FINISHED,
    LOAD_ERROR,
    TITLE_CHANGED,
    URL_CHANGED,
    PROGRESS_CHANGED,
    SECURITY_CHANGED,
    CONSOLE_MESSAGE,
    ALERT,
    CONFIRM,
    PROMPT,
    NEW_WINDOW,
    CLOSE_WINDOW,
    CONTEXT_MENU,
    DOWNLOAD_STARTED,
    DOWNLOAD_FINISHED,
    DOWNLOAD_ERROR,
    PERMISSION_REQUEST,
    GEOLOCATION_REQUEST,
    NOTIFICATION_REQUEST,
    MEDIA_ACCESS_REQUEST
};

// WebKit Event Data
struct WebKitEvent {
    WebKitEventType type;
    String data;
    String url;
    int progress;
    String title;
    String message;
    bool allowed;
    std::unordered_map<String, String> metadata;
    
    WebKitEvent() : type(WebKitEventType::LOAD_STARTED), progress(0), allowed(false) {}
};

// WebKit Download Info
struct WebKitDownloadInfo {
    String url;
    String filename;
    String mimeType;
    size_t totalSize;
    size_t downloadedSize;
    int statusCode;
    String statusText;
    bool isComplete;
    bool isCancelled;
    
    WebKitDownloadInfo() : totalSize(0), downloadedSize(0), statusCode(0), isComplete(false), isCancelled(false) {}
};

// WebKit Permission Request
struct WebKitPermissionRequest {
    String permission;
    String origin;
    String description;
    bool granted;
    
    WebKitPermissionRequest() : granted(false) {}
};

// WebKit Engine Interface
class WebKitEngine {
public:
    WebKitEngine();
    ~WebKitEngine();
    
    // Initialization and Configuration
    bool initialize(const WebKitConfig& config = WebKitConfig());
    void shutdown();
    bool isInitialized() const;
    void setConfig(const WebKitConfig& config);
    WebKitConfig getConfig() const;
    
    // Page Management
    bool createPage();
    bool destroyPage();
    bool loadURL(const String& url);
    bool loadHTML(const String& html, const String& baseURL = "");
    bool loadFile(const String& filePath);
    String getCurrentURL() const;
    String getCurrentTitle() const;
    bool canGoBack() const;
    bool canGoForward() const;
    bool goBack();
    bool goForward();
    bool reload();
    bool stopLoading();
    
    // Navigation and History
    void navigateTo(const String& url);
    void navigateBack();
    void navigateForward();
    void navigateReload();
    void navigateStop();
    std::vector<String> getHistory() const;
    void clearHistory();
    void setHistoryLimit(int limit);
    
    // JavaScript Execution
    String executeJavaScript(const String& script);
    void executeJavaScriptAsync(const String& script);
    bool injectJavaScript(const String& script);
    bool removeInjectedJavaScript(const String& identifier);
    void setJavaScriptEnabled(bool enabled);
    bool isJavaScriptEnabled() const;
    
    // DOM Manipulation
    String getDOMContent() const;
    bool setDOMContent(const String& html);
    String getElementById(const String& id) const;
    String getElementsByTagName(const String& tagName) const;
    String getElementsByClassName(const String& className) const;
    bool setElementAttribute(const String& selector, const String& attribute, const String& value);
    bool removeElement(const String& selector);
    bool addElement(const String& parentSelector, const String& html);
    
    // Event Handling
    void setEventHandler(std::function<void(const WebKitEvent&)> handler);
    void setLoadHandler(std::function<void(const String&)> handler);
    void setErrorHandler(std::function<void(const String&)> handler);
    void setProgressHandler(std::function<void(int)> handler);
    void setTitleHandler(std::function<void(const String&)> handler);
    void setConsoleHandler(std::function<void(const String&, const String&)> handler);
    
    // Security and Permissions
    WebKitSecurityInfo getSecurityInfo() const;
    void setSecurityPolicy(const String& policy);
    bool requestPermission(const String& permission, const String& origin);
    void grantPermission(const String& permission, const String& origin);
    void denyPermission(const String& permission, const String& origin);
    std::vector<WebKitPermissionRequest> getPendingPermissions() const;
    
    // Downloads
    void setDownloadHandler(std::function<void(const WebKitDownloadInfo&)> handler);
    bool startDownload(const String& url, const String& filename = "");
    void cancelDownload(const String& url);
    std::vector<WebKitDownloadInfo> getDownloads() const;
    void clearDownloads();
    
    // Developer Tools Integration
    void enableDeveloperTools(bool enabled);
    bool isDeveloperToolsEnabled() const;
    // std::shared_ptr<DeveloperTools> getDeveloperTools() const; // Commented out: DeveloperTools not implemented
    // void setDeveloperTools(std::shared_ptr<DeveloperTools> tools); // Commented out: DeveloperTools not implemented
    void openDeveloperTools();
    void closeDeveloperTools();
    bool isDeveloperToolsOpen() const;
    
    // Console and Debugging
    // void logToConsole(const String& message, ConsoleLevel level = ConsoleLevel::LOG); // Commented out: ConsoleLevel not implemented
    void clearConsole();
    // std::vector<ConsoleMessage> getConsoleMessages() const; // Commented out: ConsoleMessage not implemented
    // void setConsoleCallback(std::function<void(const ConsoleMessage&)> callback); // Commented out: ConsoleMessage not implemented
    
    // Network Monitoring
    void enableNetworkMonitoring(bool enabled);
    bool isNetworkMonitoringEnabled() const;
    std::vector<NetworkRequest> getNetworkRequests() const;
    std::vector<NetworkResponse> getNetworkResponses() const;
    void clearNetworkLog();
    
    // Performance Monitoring
    void startPerformanceMonitoring();
    void stopPerformanceMonitoring();
    PerformanceMetrics getPerformanceMetrics() const;
    void setPerformanceCallback(std::function<void(const PerformanceMetrics&)> callback);
    
    // Node.js Integration
    void enableNodeIntegration(bool enabled);
    bool isNodeIntegrationEnabled() const;
    String executeNodeScript(const String& script);
    void setNodeModulesPath(const String& path);
    String getNodeModulesPath() const;
    
    // Storage and Cache
    void clearCache();
    void clearCookies();
    void clearLocalStorage();
    void clearSessionStorage();
    void setCacheEnabled(bool enabled);
    bool isCacheEnabled() const;
    void setCacheSize(size_t size);
    size_t getCacheSize() const;
    
    // User Agent and Headers
    void setUserAgent(const String& userAgent);
    String getUserAgent() const;
    void setCustomHeaders(const std::unordered_map<String, String>& headers);
    std::unordered_map<String, String> getCustomHeaders() const;
    
    // Viewport and Rendering
    void setViewportSize(int width, int height);
    void getViewportSize(int& width, int& height) const;
    void setZoomLevel(double zoom);
    double getZoomLevel() const;
    void setScrollPosition(int x, int y);
    void getScrollPosition(int& x, int& y) const;
    
    // Screenshot and Capture
    bool captureScreenshot(const String& filePath);
    String captureScreenshotAsBase64();
    bool captureDOMSnapshot(const String& filePath);
    String captureDOMSnapshotAsJSON();
    
    // Accessibility
    void enableAccessibility(bool enabled);
    bool isAccessibilityEnabled() const;
    String getAccessibilityTree() const;
    void setAccessibilityCallback(std::function<void(const String&)> callback);
    
    // Extensions and Plugins
    bool loadExtension(const String& extensionPath);
    bool unloadExtension(const String& extensionId);
    std::vector<String> getLoadedExtensions() const;
    void setExtensionEnabled(const String& extensionId, bool enabled);
    bool isExtensionEnabled(const String& extensionId) const;
    
    // Service Workers
    void enableServiceWorkers(bool enabled);
    bool isServiceWorkersEnabled() const;
    std::vector<String> getRegisteredServiceWorkers() const;
    void unregisterServiceWorker(const String& scope);
    
    // WebRTC and Media
    void enableWebRTC(bool enabled);
    bool isWebRTCEnabled() const;
    void setMediaPermissions(bool camera, bool microphone);
    bool getCameraPermission() const;
    bool getMicrophonePermission() const;
    
    // Geolocation
    void enableGeolocation(bool enabled);
    bool isGeolocationEnabled() const;
    void setGeolocation(double latitude, double longitude);
    void getGeolocation(double& latitude, double& longitude) const;
    
    // Notifications
    void enableNotifications(bool enabled);
    bool isNotificationsEnabled() const;
    void showNotification(const String& title, const String& body);
    void setNotificationCallback(std::function<void(const String&, const String&)> callback);
    
    // Error Handling
    void setErrorCallback(std::function<void(const String&, const String&)> callback);
    std::vector<String> getErrors() const;
    void clearErrors();
    
    // Memory Management
    size_t getMemoryUsage() const;
    void setMemoryLimit(size_t limit);
    size_t getMemoryLimit() const;
    void garbageCollect();
    
    // Threading and Async
    void setAsyncCallback(std::function<void()> callback);
    void runAsync(std::function<void()> task);
    bool isAsyncTaskRunning() const;
    
    // Main loop integration
    void update() {}  // Stub for main loop
    
private:
    WebKitConfig config;
    bool initialized;
    // std::shared_ptr<DeveloperTools> devTools; // Commented out: DeveloperTools not implemented
    
    // Event handlers
    std::function<void(const WebKitEvent&)> eventHandler;
    std::function<void(const String&)> loadHandler;
    std::function<void(const String&)> errorHandler;
    std::function<void(int)> progressHandler;
    std::function<void(const String&)> titleHandler;
    std::function<void(const String&, const String&)> consoleHandler;
    std::function<void(const WebKitDownloadInfo&)> downloadHandler;
    std::function<void(const PerformanceMetrics&)> performanceCallback;
    std::function<void(const ConsoleMessage&)> consoleCallback; // Commented out: ConsoleMessage not implemented
    std::function<void(const String&)> accessibilityCallback;
    std::function<void(const String&, const String&)> notificationCallback;
    std::function<void(const String&, const String&)> errorCallback;
    std::function<void()> asyncCallback;
    
    // Internal state
    String currentURL;
    String currentTitle;
    std::vector<String> history;
    int historyLimit;
    bool developerToolsOpen;
    bool networkMonitoringEnabled;
    bool performanceMonitoringActive;
    bool nodeIntegrationEnabled;
    String nodeModulesPath;
    bool cacheEnabled;
    size_t cacheSize;
    String userAgent;
    std::unordered_map<String, String> customHeaders;
    int viewportWidth;
    int viewportHeight;
    double zoomLevel;
    int scrollX;
    int scrollY;
    bool accessibilityEnabled;
    bool serviceWorkersEnabled;
    bool webRTCEnabled;
    bool cameraPermission;
    bool microphonePermission;
    bool geolocationEnabled;
    double latitude;
    double longitude;
    bool notificationsEnabled;
    size_t memoryLimit;
    bool asyncTaskRunning;
    
    // Internal methods
    void initializeDeveloperTools();
    void handleWebKitEvent(const WebKitEvent& event);
    void updateSecurityInfo();
    void updatePerformanceMetrics();
    void cleanupResources();
};

// WebKit Engine Manager
class WebKitEngineManager {
public:
    WebKitEngineManager();
    ~WebKitEngineManager();
    
    // Engine management
    std::shared_ptr<WebKitEngine> createEngine(const WebKitConfig& config = WebKitConfig{});
    void destroyEngine(std::shared_ptr<WebKitEngine> engine);
    std::vector<std::shared_ptr<WebKitEngine>> getEngines() const;
    
    // Global settings
    void setGlobalConfig(const WebKitConfig& config);
    const WebKitConfig& getGlobalConfig() const;
    
    // Memory management
    void clearAllCaches();
    void garbageCollectAll();
    int getTotalMemoryUsage() const;
    
    // Statistics
    int getActivePageCount() const;
    int getTotalPageCount() const;
    String getWebKitVersion() const;
    
    // Performance monitoring
    void startGlobalPerformanceMonitoring();
    void stopGlobalPerformanceMonitoring();
    std::vector<WebKitPerformanceMetrics> getAllPerformanceMetrics() const;
    
    // Network management
    void setGlobalProxy(const String& host, int port);
    void setGlobalCustomHeaders(const std::unordered_map<String, String>& headers);
    void clearAllCookies();
    
    // Security management
    void setGlobalSecurityPolicy(WebKitSecurityPolicy policy);
    void addGlobalAllowedDomain(const String& domain);
    void removeGlobalAllowedDomain(const String& domain);
    
private:
    WebKitConfig globalConfig;
    std::vector<std::shared_ptr<WebKitEngine>> engines;
    bool webKitInitialized;
    
    void initializeWebKit();
    void shutdownWebKit();
};

} // namespace zepra