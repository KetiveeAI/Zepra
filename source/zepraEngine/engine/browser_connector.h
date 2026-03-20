// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once

#include "../common/types.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace zepra {

using json = nlohmann::json;

// Message types for browser-engine communication
enum class MessageType {
    SEARCH_REQUEST,
    SEARCH_RESPONSE,
    PAGE_LOAD,
    PAGE_RENDER,
    USER_INPUT,
    ENGINE_STATUS,
    ERROR,
    HEARTBEAT
};

// Communication protocol
struct BrowserMessage {
    MessageType type;
    String id;
    json payload;
    uint64_t timestamp;
    
    BrowserMessage() : type(MessageType::HEARTBEAT), timestamp(0) {}
    
    String serialize() const;
    static BrowserMessage deserialize(const String& data);
};

// Transport layer interface
class Transport {
public:
    virtual ~Transport() = default;
    
    virtual bool connect(const String& endpoint) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    
    virtual bool send(const BrowserMessage& message) = 0;
    virtual BrowserMessage receive(int timeoutMs = -1) = 0;
    
    virtual void setMessageCallback(std::function<void(const BrowserMessage&)> callback) = 0;
};

// WebSocket transport for real-time communication
class WebSocketTransport : public Transport {
public:
    WebSocketTransport();
    ~WebSocketTransport() override;
    
    bool connect(const String& endpoint) override;
    void disconnect() override;
    bool isConnected() const override;
    
    bool send(const BrowserMessage& message) override;
    BrowserMessage receive(int timeoutMs = -1) override;
    
    void setMessageCallback(std::function<void(const BrowserMessage&)> callback) override;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// HTTP transport for request-response pattern
class HTTPTransport : public Transport {
public:
    HTTPTransport();
    ~HTTPTransport() override;
    
    bool connect(const String& endpoint) override;
    void disconnect() override;
    bool isConnected() const override;
    
    bool send(const BrowserMessage& message) override;
    BrowserMessage receive(int timeoutMs = -1) override;
    
    void setMessageCallback(std::function<void(const BrowserMessage&)> callback) override;
    
    // HTTP specific
    void setHeaders(const std::unordered_map<String, String>& headers);
    void setTimeout(int timeoutMs);
    
private:
    String endpoint_;
    std::unordered_map<String, String> headers_;
    int timeoutMs_;
    bool connected_;
    std::function<void(const BrowserMessage&)> messageCallback_;
};

// IPC transport for local communication (Unix sockets/Named pipes)
class IPCTransport : public Transport {
public:
    IPCTransport();
    ~IPCTransport() override;
    
    bool connect(const String& endpoint) override;
    void disconnect() override;
    bool isConnected() const override;
    
    bool send(const BrowserMessage& message) override;
    BrowserMessage receive(int timeoutMs = -1) override;
    
    void setMessageCallback(std::function<void(const BrowserMessage&)> callback) override;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Browser-Engine Connector
class BrowserConnector {
public:
    BrowserConnector();
    ~BrowserConnector() = default;
    
    // Connection management
    bool connect(const String& endpoint, const String& transport = "http");
    void disconnect();
    bool isConnected() const;
    
    // Search operations
    json search(const String& query, const json& options = json::object());
    json searchImages(const String& query);
    json searchVideos(const String& query);
    json searchNews(const String& query);
    
    // Page operations
    json loadPage(const String& url);
    json renderPage(const String& html, const String& baseUrl = "");
    json getPageContent(const String& url);
    
    // Engine status
    json getEngineStatus();
    json getEngineMetrics();
    
    // Async operations with callbacks
    void searchAsync(const String& query, std::function<void(const json&)> callback);
    void loadPageAsync(const String& url, std::function<void(const json&)> callback);
    
    // Message handling
    void sendMessage(const BrowserMessage& message);
    void setMessageHandler(MessageType type, std::function<void(const BrowserMessage&)> handler);
    
    // Configuration
    void setEndpoint(const String& endpoint) { endpoint_ = endpoint; }
    String getEndpoint() const { return endpoint_; }
    
    void setTimeout(int timeoutMs) { timeoutMs_ = timeoutMs; }
    int getTimeout() const { return timeoutMs_; }
    
    void setRetryCount(int count) { retryCount_ = count; }
    int getRetryCount() const { return retryCount_; }
    
    // Error handling
    void setErrorHandler(std::function<void(const String&)> handler) { errorHandler_ = handler; }
    
private:
    std::unique_ptr<Transport> transport_;
    String endpoint_;
    int timeoutMs_;
    int retryCount_;
    bool connected_;
    
    std::unordered_map<MessageType, std::function<void(const BrowserMessage&)>> messageHandlers_;
    std::function<void(const String&)> errorHandler_;
    
    // Helper methods
    BrowserMessage createMessage(MessageType type, const json& payload);
    json sendAndReceive(const BrowserMessage& message);
    void handleMessage(const BrowserMessage& message);
    void handleError(const String& error);
    
    // Transport factory
    std::unique_ptr<Transport> createTransport(const String& type);
};

// Python bridge for Python-based search engine
class PythonBridge {
public:
    PythonBridge();
    ~PythonBridge();
    
    // Initialize Python interpreter
    bool initialize();
    void finalize();
    bool isInitialized() const;
    
    // Execute Python code
    json executePython(const String& code);
    json callPythonFunction(const String& module, const String& function, const json& args);
    
    // Search engine integration
    json search(const String& query, const json& options = json::object());
    json getSearchSuggestions(const String& query);
    json getTrendingSearches();
    
    // ML operations
    json analyzeContent(const String& content);
    json generateSummary(const String& content);
    json extractKeywords(const String& content);
    
    // Configuration
    void setPythonPath(const String& path);
    void addModulePath(const String& path);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    bool initialized_;
};

class JavaScriptBridge {
public:
    JavaScriptBridge();
    ~JavaScriptBridge();
    
    // Initialize JavaScript engine
    bool initialize();
    void finalize();
    bool isInitialized() const;
    
    // Execute JavaScript code
    json executeJavaScript(const String& code);
    json callJavaScriptFunction(const String& function, const json& args);
    
    // DOM manipulation
    json evaluateSelector(const String& selector, const String& html);
    json executeScript(const String& script, const String& html);
    
    // Event handling
    void addEventListener(const String& event, std::function<void(const json&)> handler);
    void removeEventListener(const String& event);
    void dispatchEvent(const String& event, const json& data);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    bool initialized_;
    std::unordered_map<String, std::function<void(const json&)>> eventHandlers_;
};

// Multi-language connector that supports C++, Python, and JavaScript
class MultiLanguageConnector {
public:
    MultiLanguageConnector();
    ~MultiLanguageConnector() = default;
    
    // Initialize all bridges
    bool initialize();
    void finalize();
    
    // Get individual bridges
    BrowserConnector& getBrowserConnector() { return *browserConnector_; }
    PythonBridge& getPythonBridge() { return *pythonBridge_; }
    JavaScriptBridge& getJavaScriptBridge() { return *jsBridge_; }
    
    // Unified search interface
    json search(const String& query, const json& options = json::object());
    
    // Cross-language communication
    json callPython(const String& function, const json& args);
    json callJavaScript(const String& function, const json& args);
    json callCpp(const String& function, const json& args);
    
    // Configuration
    void setSearchEngineEndpoint(const String& endpoint);
    void setPythonPath(const String& path);
    
private:
    std::unique_ptr<BrowserConnector> browserConnector_;
    std::unique_ptr<PythonBridge> pythonBridge_;
    std::unique_ptr<JavaScriptBridge> jsBridge_;
    
    bool initialized_;
};

// Utility functions
namespace connector_utils {
    // Message serialization
    String serializeMessage(const BrowserMessage& message);
    BrowserMessage deserializeMessage(const String& data);
    
    // JSON helpers
    json createSearchRequest(const String& query, const json& options = json::object());
    json createPageLoadRequest(const String& url);
    json createErrorResponse(const String& error);
    
    // URL helpers
    String buildSearchUrl(const String& endpoint, const String& query);
    String buildApiUrl(const String& endpoint, const String& path);
    
    // Validation
    bool isValidMessage(const BrowserMessage& message);
    bool isValidEndpoint(const String& endpoint);
    
    // Error handling
    String formatError(const String& error, const String& context = "");
}

} // namespace zepra
