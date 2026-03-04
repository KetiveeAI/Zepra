#include "engine/dev_tools.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <nxhttp.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace zepra {

// DeveloperTools Implementation
DeveloperTools::DeveloperTools() 
    : nodeIntegrationEnabled(false)
    , preserveLog(false)
    , showTimestamps(true)
    , performanceMonitoringActive(false)
    , nextNodeId(1)
    , nextRequestId(1) {
    
    std::cout << "🔧 Developer Tools initialized" << std::endl;
}

DeveloperTools::~DeveloperTools() {
    std::cout << "🔧 Developer Tools destroyed" << std::endl;
}

// Console Management
void DeveloperTools::log(const String& message, ConsoleLevel level) {
    ConsoleMessage msg;
    msg.level = level;
    msg.message = message;
    msg.timestamp = std::chrono::system_clock::now();
    addConsoleMessage(msg);
}

void DeveloperTools::log(const String& message, const String& source, int line, int column) {
    ConsoleMessage msg;
    msg.level = ConsoleLevel::LOG;
    msg.message = message;
    msg.source = source;
    msg.line = line;
    msg.column = column;
    msg.timestamp = std::chrono::system_clock::now();
    addConsoleMessage(msg);
}

void DeveloperTools::clearConsole() {
    if (!preserveLog) {
        consoleMessages.clear();
    }
}

std::vector<ConsoleMessage> DeveloperTools::getConsoleMessages() const {
    return consoleMessages;
}

void DeveloperTools::setConsoleCallback(std::function<void(const ConsoleMessage&)> callback) {
    consoleCallback = callback;
}

// Network Monitoring
void DeveloperTools::logNetworkRequest(const NetworkRequest& request) {
    NetworkRequest req = request;
    req.id = nextRequestId++;
    req.start_time = std::chrono::system_clock::now();
    addNetworkRequest(req);
    
    std::cout << "Network Request: " << req.method << " " << req.url << std::endl;
}

void DeveloperTools::logNetworkResponse(const NetworkResponse& response) {
    NetworkResponse resp = response;
    resp.end_time = std::chrono::system_clock::now();
    addNetworkResponse(resp);
    
    std::cout << "Network Response: " << resp.status_code << " (request " << resp.request_id << ")" << std::endl;
}

std::vector<NetworkRequest> DeveloperTools::getNetworkRequests() const {
    return networkRequests;
}

std::vector<NetworkResponse> DeveloperTools::getNetworkResponses() const {
    return networkResponses;
}

void DeveloperTools::clearNetworkLog() {
    networkRequests.clear();
    networkResponses.clear();
}

// DOM Inspector
void DeveloperTools::setDOMTree(std::shared_ptr<DevToolsDOMNode> root) {
    domTree = root;
    std::cout << "🌳 DOM Tree updated" << std::endl;
}

std::shared_ptr<DevToolsDOMNode> DeveloperTools::getDOMTree() const {
    return domTree;
}

std::shared_ptr<DevToolsDOMNode> DeveloperTools::getNodeById(int nodeId) const {
    // Recursive search through DOM tree
    std::function<std::shared_ptr<DevToolsDOMNode>(std::shared_ptr<DevToolsDOMNode>)> search = 
        [&](std::shared_ptr<DevToolsDOMNode> node) -> std::shared_ptr<DevToolsDOMNode> {
            if (!node) return nullptr;
            if (node->nodeId == nodeId) return node;
            
            for (const auto& child : node->children) {
                auto result = search(child);
                if (result) return result;
            }
            return nullptr;
        };
    
    return search(domTree);
}

void DeveloperTools::highlightNode(int nodeId) {
    auto node = getNodeById(nodeId);
    if (node) {
        std::cout << "🎯 Highlighting node: " << node->tagName << " (ID: " << nodeId << ")" << std::endl;
    }
}

void DeveloperTools::inspectElement(int x, int y) {
    std::cout << "🔍 Inspecting element at (" << x << ", " << y << ")" << std::endl;
    // TODO: Implement element inspection at coordinates
}

// JavaScript Console
String DeveloperTools::executeJavaScript(const String& script) {
    std::cout << "📜 Executing JavaScript: " << script.substr(0, 50) << "..." << std::endl;
    
    // TODO: Integrate with actual JavaScript engine
    return "// JavaScript execution result";
}

void DeveloperTools::executeJavaScriptAsync(const String& script) {
    std::cout << "📜 Executing JavaScript async: " << script.substr(0, 50) << "..." << std::endl;
    // TODO: Implement async JavaScript execution
}

std::vector<JSContext> DeveloperTools::getJSContexts() const {
    return jsContexts;
}

void DeveloperTools::addJSContext(const JSContext& context) {
    jsContexts.push_back(context);
    std::cout << "📜 Added JS Context: " << context.name << std::endl;
}

// Performance Monitoring
void DeveloperTools::startPerformanceMonitoring() {
    performanceMonitoringActive = true;
    std::cout << "📊 Performance monitoring started" << std::endl;
}

void DeveloperTools::stopPerformanceMonitoring() {
    performanceMonitoringActive = false;
    std::cout << "📊 Performance monitoring stopped" << std::endl;
}

PerformanceMetrics DeveloperTools::getPerformanceMetrics() const {
    return performanceMetrics;
}

void DeveloperTools::setPerformanceCallback(std::function<void(const PerformanceMetrics&)> callback) {
    performanceCallback = callback;
}

// Node.js Integration
void DeveloperTools::enableNodeIntegration(bool enabled) {
    nodeIntegrationEnabled = enabled;
    std::cout << "🟢 Node.js integration " << (enabled ? "enabled" : "disabled") << std::endl;
}

bool DeveloperTools::isNodeIntegrationEnabled() const {
    return nodeIntegrationEnabled;
}

String DeveloperTools::executeNodeScript(const String& script) {
    if (!nodeIntegrationEnabled) {
        return "Error: Node.js integration is disabled";
    }
    
    std::cout << "🟢 Executing Node.js script: " << script.substr(0, 50) << "..." << std::endl;
    
    // Execute Node.js script via HTTP API using nxhttp
    try {
        String nodeUrl = "http://localhost:6329/api/node/execute";
        
        json requestData;
        requestData["script"] = script;
        requestData["modulesPath"] = nodeModulesPath;
        
        String jsonRequest = requestData.dump();
        
        nx::HttpClient client;
        auto response = client.post(nodeUrl, jsonRequest, "application/json");
        
        if (response.ok()) {
            try {
                json responseData = json::parse(response.body());
                if (responseData.contains("result")) {
                    return responseData["result"].get<String>();
                }
            } catch (const json::exception& e) {
                std::cerr << "Failed to parse Node.js response: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "Node.js request failed: " << response.status() << std::endl;
        }
    } catch (const nx::HttpException& e) {
        std::cerr << "Node.js HTTP error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Node.js execution error: " << e.what() << std::endl;
    }
    
    return "Node.js execution failed";
}

void DeveloperTools::setNodeModulesPath(const String& path) {
    nodeModulesPath = path;
    std::cout << "📁 Node modules path set to: " << path << std::endl;
}

String DeveloperTools::getNodeModulesPath() const {
    return nodeModulesPath;
}

// Storage Inspector
void DeveloperTools::inspectLocalStorage() {
    std::cout << "💾 Inspecting Local Storage" << std::endl;
    // TODO: Implement local storage inspection
}

void DeveloperTools::inspectSessionStorage() {
    std::cout << "💾 Inspecting Session Storage" << std::endl;
    // TODO: Implement session storage inspection
}

void DeveloperTools::inspectCookies() {
    std::cout << "🍪 Inspecting Cookies" << std::endl;
    // TODO: Implement cookie inspection
}

void DeveloperTools::inspectIndexedDB() {
    std::cout << "🗄️ Inspecting IndexedDB" << std::endl;
    // TODO: Implement IndexedDB inspection
}

// Application Panel
void DeveloperTools::inspectManifest() {
    std::cout << "📱 Inspecting Web App Manifest" << std::endl;
    // TODO: Implement manifest inspection
}

void DeveloperTools::inspectServiceWorkers() {
    std::cout << "👷 Inspecting Service Workers" << std::endl;
    // TODO: Implement service worker inspection
}

void DeveloperTools::inspectCacheStorage() {
    std::cout << "🗂️ Inspecting Cache Storage" << std::endl;
    // TODO: Implement cache storage inspection
}

// Security Panel
void DeveloperTools::inspectSecurityInfo() {
    std::cout << "🔒 Inspecting Security Information" << std::endl;
    // TODO: Implement security inspection
}

void DeveloperTools::inspectCertificates() {
    std::cout << "📜 Inspecting Certificates" << std::endl;
    // TODO: Implement certificate inspection
}

void DeveloperTools::inspectPermissions() {
    std::cout << "🔐 Inspecting Permissions" << std::endl;
    // TODO: Implement permission inspection
}

// Settings
void DeveloperTools::setPreserveLog(bool preserve) {
    preserveLog = preserve;
}

bool DeveloperTools::isPreserveLogEnabled() const {
    return preserveLog;
}

void DeveloperTools::setShowTimestamps(bool show) {
    showTimestamps = show;
}

bool DeveloperTools::isShowTimestampsEnabled() const {
    return showTimestamps;
}

// Private Methods
void DeveloperTools::addConsoleMessage(const ConsoleMessage& message) {
    consoleMessages.push_back(message);
    
    // Limit console messages
    if (consoleMessages.size() > 1000) {
        consoleMessages.erase(consoleMessages.begin());
    }
    
    if (consoleCallback) {
        consoleCallback(message);
    }
    
    // Print to console
    String levelStr;
    switch (message.level) {
        case ConsoleLevel::LOG: levelStr = "LOG"; break;
        case ConsoleLevel::INFO: levelStr = "INFO"; break;
        case ConsoleLevel::WARN: levelStr = "WARN"; break;
        case ConsoleLevel::ERROR: levelStr = "ERROR"; break;
        case ConsoleLevel::DEBUG: levelStr = "DEBUG"; break;
        case ConsoleLevel::ASSERT: levelStr = "ASSERT"; break;
    }
    
    std::cout << "[" << levelStr << "] " << message.message;
    if (!message.source.empty()) {
        std::cout << " (" << message.source << ":" << message.line << ":" << message.column << ")";
    }
    std::cout << std::endl;
}

void DeveloperTools::addNetworkRequest(const NetworkRequest& request) {
    networkRequests.push_back(request);
    
    // Limit network requests
    if (networkRequests.size() > 500) {
        networkRequests.erase(networkRequests.begin());
    }
}

void DeveloperTools::addNetworkResponse(const NetworkResponse& response) {
    networkResponses.push_back(response);
    
    // Limit network responses
    if (networkResponses.size() > 500) {
        networkResponses.erase(networkResponses.begin());
    }
}

// DevToolsManager Implementation
DevToolsManager::DevToolsManager() 
    : globalPreserveLog(false)
    , globalShowTimestamps(true)
    , globalNodeIntegration(false) {
    
    std::cout << "🔧 Developer Tools Manager initialized" << std::endl;
}

DevToolsManager::~DevToolsManager() {
    std::cout << "🔧 Developer Tools Manager destroyed" << std::endl;
}

std::shared_ptr<DeveloperTools> DevToolsManager::createTools() {
    auto devTools = std::make_shared<DeveloperTools>();
    devTools->setPreserveLog(globalPreserveLog);
    devTools->setShowTimestamps(globalShowTimestamps);
    devTools->enableNodeIntegration(globalNodeIntegration);
    
    this->tools.push_back(devTools);
    std::cout << "🔧 Created new Developer Tools instance" << std::endl;
    
    return devTools;
}

void DevToolsManager::destroyTools(std::shared_ptr<DeveloperTools> toolsToDestroy) {
    auto it = std::find(this->tools.begin(), this->tools.end(), toolsToDestroy);
    if (it != this->tools.end()) {
        this->tools.erase(it);
        std::cout << "🔧 Destroyed Developer Tools instance" << std::endl;
    }
}

std::vector<std::shared_ptr<DeveloperTools>> DevToolsManager::getAllTools() const {
    return tools;
}

void DevToolsManager::setGlobalPreserveLog(bool preserve) {
    globalPreserveLog = preserve;
    for (auto& tools : this->tools) {
        tools->setPreserveLog(preserve);
    }
}

void DevToolsManager::setGlobalShowTimestamps(bool show) {
    globalShowTimestamps = show;
    for (auto& tools : this->tools) {
        tools->setShowTimestamps(show);
    }
}

void DevToolsManager::setGlobalNodeIntegration(bool enabled) {
    globalNodeIntegration = enabled;
    for (auto& tools : this->tools) {
        tools->enableNodeIntegration(enabled);
    }
}

void DevToolsManager::startGlobalPerformanceMonitoring() {
    for (auto& tools : this->tools) {
        tools->startPerformanceMonitoring();
    }
}

void DevToolsManager::stopGlobalPerformanceMonitoring() {
    for (auto& tools : this->tools) {
        tools->stopPerformanceMonitoring();
    }
}

std::vector<PerformanceMetrics> DevToolsManager::getAllPerformanceMetrics() const {
    std::vector<PerformanceMetrics> metrics;
    for (const auto& tools : this->tools) {
        metrics.push_back(tools->getPerformanceMetrics());
    }
    return metrics;
}

} // namespace zepra 