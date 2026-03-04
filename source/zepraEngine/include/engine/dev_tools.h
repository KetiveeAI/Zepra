#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include "../network/network_monitor.h"  // NetworkRequest, NetworkResponse defined here

namespace zepra {

using String = std::string;

enum class ConsoleLevel {
    LOG,
    INFO,
    WARN,
    ERROR,
    DEBUG,
    ASSERT
};

struct ConsoleMessage {
    ConsoleLevel level;
    String message;
    String source;
    int line = 0;
    int column = 0;
    std::chrono::system_clock::time_point timestamp;
};

// Note: NetworkRequest and NetworkResponse are defined in network/network_monitor.h
// to avoid duplication and enable per-tab network isolation.

struct DevToolsDOMNode {

    int nodeId = 0;
    String tagName;
    std::vector<std::shared_ptr<DevToolsDOMNode>> children;
};

struct JSContext {
    String name;
};

struct PerformanceMetrics {
    double cpuUsage = 0.0;
    double memoryUsage = 0.0;
    double fps = 0.0;
};

class DeveloperTools {
public:
    DeveloperTools();
    ~DeveloperTools();

    // Console Management
    void log(const String& message, ConsoleLevel level);
    void log(const String& message, const String& source, int line, int column);
    void clearConsole();
    std::vector<ConsoleMessage> getConsoleMessages() const;
    void setConsoleCallback(std::function<void(const ConsoleMessage&)> callback);

    // Network Monitoring
    void logNetworkRequest(const NetworkRequest& request);
    void logNetworkResponse(const NetworkResponse& response);
    std::vector<NetworkRequest> getNetworkRequests() const;
    std::vector<NetworkResponse> getNetworkResponses() const;
    void clearNetworkLog();

    // DOM Inspector
    void setDOMTree(std::shared_ptr<DevToolsDOMNode> root);
    std::shared_ptr<DevToolsDOMNode> getDOMTree() const;
    std::shared_ptr<DevToolsDOMNode> getNodeById(int nodeId) const;
    void highlightNode(int nodeId);
    void inspectElement(int x, int y);

    // JavaScript Console
    String executeJavaScript(const String& script);
    void executeJavaScriptAsync(const String& script);
    std::vector<JSContext> getJSContexts() const;
    void addJSContext(const JSContext& context);

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

    // Storage Inspector
    void inspectLocalStorage();
    void inspectSessionStorage();
    void inspectCookies();
    void inspectIndexedDB();
    void inspectCacheStorage();
    void inspectServiceWorkers();
    void inspectManifest();

    // Security Panel
    void inspectSecurityInfo();
    void inspectCertificates();
    void inspectPermissions();

    // Settings
    void setPreserveLog(bool preserve);
    bool isPreserveLogEnabled() const;
    void setShowTimestamps(bool show);
    bool isShowTimestampsEnabled() const;

private:
    bool nodeIntegrationEnabled;
    bool preserveLog;
    bool showTimestamps;
    bool performanceMonitoringActive;
    int nextNodeId;
    int nextRequestId;
    String nodeModulesPath;
    std::vector<ConsoleMessage> consoleMessages;
    std::function<void(const ConsoleMessage&)> consoleCallback;
    std::vector<NetworkRequest> networkRequests;
    std::vector<NetworkResponse> networkResponses;
    std::shared_ptr<DevToolsDOMNode> domTree;
    std::vector<JSContext> jsContexts;
    PerformanceMetrics performanceMetrics;
    std::function<void(const PerformanceMetrics&)> performanceCallback;

    // Private helper methods
    void addConsoleMessage(const ConsoleMessage& msg);
    void addNetworkRequest(const NetworkRequest& req);
    void addNetworkResponse(const NetworkResponse& resp);
};

// DevToolsManager - Factory and manager for DeveloperTools instances
class DevToolsManager {
public:
    DevToolsManager();
    ~DevToolsManager();
    
    std::shared_ptr<DeveloperTools> createTools();
    void destroyTools(std::shared_ptr<DeveloperTools> tools);
    
    void setGlobalPreserveLog(bool preserve);
    void setGlobalShowTimestamps(bool show);
    void setGlobalNodeIntegration(bool enable);
    
    bool isGlobalPreserveLogEnabled() const { return globalPreserveLog; }
    bool isGlobalShowTimestampsEnabled() const { return globalShowTimestamps; }
    bool isGlobalNodeIntegrationEnabled() const { return globalNodeIntegration; }
    
    std::vector<std::shared_ptr<DeveloperTools>> getAllTools() const;
    
    void startGlobalPerformanceMonitoring();
    void stopGlobalPerformanceMonitoring();
    std::vector<PerformanceMetrics> getAllPerformanceMetrics() const;

private:
    std::vector<std::shared_ptr<DeveloperTools>> tools;
    bool globalPreserveLog;
    bool globalShowTimestamps;
    bool globalNodeIntegration;
};

} // namespace zepra