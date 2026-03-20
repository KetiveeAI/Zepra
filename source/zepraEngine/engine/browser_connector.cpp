// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "engine/browser_connector.h"
#include <nxhttp.h>
#include <chrono>
#include <sstream>
#include <iostream>

namespace zepra {

// BrowserMessage implementation
String BrowserMessage::serialize() const {
    json j;
    j["type"] = static_cast<int>(type);
    j["id"] = id;
    j["payload"] = payload;
    j["timestamp"] = timestamp;
    return j.dump();
}

BrowserMessage BrowserMessage::deserialize(const String& data) {
    BrowserMessage msg;
    try {
        json j = json::parse(data);
        msg.type = static_cast<MessageType>(j["type"].get<int>());
        msg.id = j["id"].get<String>();
        msg.payload = j["payload"];
        msg.timestamp = j["timestamp"].get<uint64_t>();
    } catch (const std::exception& e) {
        std::cerr << "Failed to deserialize message: " << e.what() << std::endl;
    }
    return msg;
}

// HTTPTransport implementation using nxhttp
HTTPTransport::HTTPTransport() 
    : timeoutMs_(30000), connected_(false) {
    // nxhttp does not require global initialization
}

HTTPTransport::~HTTPTransport() {
    disconnect();
}

bool HTTPTransport::connect(const String& endpoint) {
    endpoint_ = endpoint;
    connected_ = true;
    return true;
}

void HTTPTransport::disconnect() {
    connected_ = false;
}

bool HTTPTransport::isConnected() const {
    return connected_;
}

bool HTTPTransport::send(const BrowserMessage& message) {
    if (!connected_) return false;
    
    try {
        String data = message.serialize();
        
        nx::HttpClient client;
        auto response = client.post(endpoint_, data, "application/json");
        
        if (response.ok() && messageCallback_) {
            BrowserMessage responseMsg = BrowserMessage::deserialize(response.body());
            messageCallback_(responseMsg);
        }
        
        return response.ok();
    } catch (const nx::HttpException& e) {
        std::cerr << "HTTPTransport send error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "HTTPTransport error: " << e.what() << std::endl;
        return false;
    }
}

BrowserMessage HTTPTransport::receive(int timeoutMs) {
    // HTTP is request-response, so we don't have a separate receive
    // This would be used with polling or long-polling
    return BrowserMessage();
}

void HTTPTransport::setMessageCallback(std::function<void(const BrowserMessage&)> callback) {
    messageCallback_ = callback;
}

void HTTPTransport::setHeaders(const std::unordered_map<String, String>& headers) {
    headers_ = headers;
}

void HTTPTransport::setTimeout(int timeoutMs) {
    timeoutMs_ = timeoutMs;
}

// BrowserConnector implementation
BrowserConnector::BrowserConnector()
    : timeoutMs_(30000), retryCount_(3), connected_(false) {
}

bool BrowserConnector::connect(const String& endpoint, const String& transport) {
    endpoint_ = endpoint;
    transport_ = createTransport(transport);
    
    if (!transport_) {
        handleError("Failed to create transport: " + transport);
        return false;
    }
    
    connected_ = transport_->connect(endpoint);
    
    if (connected_) {
        // Set up message callback
        transport_->setMessageCallback([this](const BrowserMessage& msg) {
            handleMessage(msg);
        });
    }
    
    return connected_;
}

void BrowserConnector::disconnect() {
    if (transport_) {
        transport_->disconnect();
    }
    connected_ = false;
}

bool BrowserConnector::isConnected() const {
    return connected_ && transport_ && transport_->isConnected();
}

json BrowserConnector::search(const String& query, const json& options) {
    json payload = options;
    payload["query"] = query;
    
    BrowserMessage msg = createMessage(MessageType::SEARCH_REQUEST, payload);
    return sendAndReceive(msg);
}

json BrowserConnector::searchImages(const String& query) {
    json options;
    options["type"] = "images";
    return search(query, options);
}

json BrowserConnector::searchVideos(const String& query) {
    json options;
    options["type"] = "videos";
    return search(query, options);
}

json BrowserConnector::searchNews(const String& query) {
    json options;
    options["type"] = "news";
    return search(query, options);
}

json BrowserConnector::loadPage(const String& url) {
    json payload;
    payload["url"] = url;
    
    BrowserMessage msg = createMessage(MessageType::PAGE_LOAD, payload);
    return sendAndReceive(msg);
}

json BrowserConnector::renderPage(const String& html, const String& baseUrl) {
    json payload;
    payload["html"] = html;
    payload["baseUrl"] = baseUrl;
    
    BrowserMessage msg = createMessage(MessageType::PAGE_RENDER, payload);
    return sendAndReceive(msg);
}

json BrowserConnector::getPageContent(const String& url) {
    return loadPage(url);
}

json BrowserConnector::getEngineStatus() {
    BrowserMessage msg = createMessage(MessageType::ENGINE_STATUS, json::object());
    return sendAndReceive(msg);
}

json BrowserConnector::getEngineMetrics() {
    json payload;
    payload["metrics"] = true;
    
    BrowserMessage msg = createMessage(MessageType::ENGINE_STATUS, payload);
    return sendAndReceive(msg);
}

void BrowserConnector::searchAsync(const String& query, std::function<void(const json&)> callback) {
    // Create a thread to perform async search
    std::thread([this, query, callback]() {
        try {
            json result = search(query);
            callback(result);
        } catch (const std::exception& e) {
            json error;
            error["error"] = e.what();
            callback(error);
        }
    }).detach();
}

void BrowserConnector::loadPageAsync(const String& url, std::function<void(const json&)> callback) {
    std::thread([this, url, callback]() {
        try {
            json result = loadPage(url);
            callback(result);
        } catch (const std::exception& e) {
            json error;
            error["error"] = e.what();
            callback(error);
        }
    }).detach();
}

void BrowserConnector::sendMessage(const BrowserMessage& message) {
    if (!transport_ || !connected_) {
        handleError("Not connected to engine");
        return;
    }
    
    transport_->send(message);
}

void BrowserConnector::setMessageHandler(MessageType type, std::function<void(const BrowserMessage&)> handler) {
    messageHandlers_[type] = handler;
}

BrowserMessage BrowserConnector::createMessage(MessageType type, const json& payload) {
    BrowserMessage msg;
    msg.type = type;
    msg.id = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    msg.payload = payload;
    msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    return msg;
}

json BrowserConnector::sendAndReceive(const BrowserMessage& message) {
    if (!transport_ || !connected_) {
        json error;
        error["error"] = "Not connected to engine";
        return error;
    }
    
    // For HTTP transport, the response is handled in the callback
    // For now, we'll do a synchronous request
    
    for (int attempt = 0; attempt < retryCount_; ++attempt) {
        try {
            if (transport_->send(message)) {
                // Wait for response (simplified - in real implementation use proper sync)
                BrowserMessage response = transport_->receive(timeoutMs_);
                return response.payload;
            }
        } catch (const std::exception& e) {
            if (attempt == retryCount_ - 1) {
                handleError("Failed after " + std::to_string(retryCount_) + " attempts: " + e.what());
            }
        }
    }
    
    json error;
    error["error"] = "Failed to send message";
    return error;
}

void BrowserConnector::handleMessage(const BrowserMessage& message) {
    auto it = messageHandlers_.find(message.type);
    if (it != messageHandlers_.end()) {
        it->second(message);
    }
}

void BrowserConnector::handleError(const String& error) {
    std::cerr << "BrowserConnector error: " << error << std::endl;
    if (errorHandler_) {
        errorHandler_(error);
    }
}

std::unique_ptr<Transport> BrowserConnector::createTransport(const String& type) {
    if (type == "http" || type == "https") {
        return std::make_unique<HTTPTransport>();
    } else if (type == "ws" || type == "websocket") {
        // WebSocket transport would be implemented here
        return nullptr;
    } else if (type == "ipc") {
        // IPC transport would be implemented here
        return nullptr;
    }
    
    return std::make_unique<HTTPTransport>(); // Default to HTTP
}

// MultiLanguageConnector implementation
MultiLanguageConnector::MultiLanguageConnector()
    : initialized_(false) {
    browserConnector_ = std::make_unique<BrowserConnector>();
    pythonBridge_ = std::make_unique<PythonBridge>();
    jsBridge_ = std::make_unique<JavaScriptBridge>();
}

bool MultiLanguageConnector::initialize() {
    if (initialized_) return true;
    
    bool pythonInit = pythonBridge_->initialize();
    bool jsInit = jsBridge_->initialize();
    
    initialized_ = pythonInit && jsInit;
    return initialized_;
}

void MultiLanguageConnector::finalize() {
    if (!initialized_) return;
    
    pythonBridge_->finalize();
    jsBridge_->finalize();
    browserConnector_->disconnect();
    
    initialized_ = false;
}

json MultiLanguageConnector::search(const String& query, const json& options) {
    // Try browser connector first
    if (browserConnector_->isConnected()) {
        return browserConnector_->search(query, options);
    }
    
    // Fallback to Python bridge
    if (pythonBridge_->isInitialized()) {
        return pythonBridge_->search(query, options);
    }
    
    json error;
    error["error"] = "No search backend available";
    return error;
}

json MultiLanguageConnector::callPython(const String& function, const json& args) {
    if (!pythonBridge_->isInitialized()) {
        json error;
        error["error"] = "Python bridge not initialized";
        return error;
    }
    
    return pythonBridge_->callPythonFunction("", function, args);
}

json MultiLanguageConnector::callJavaScript(const String& function, const json& args) {
    if (!jsBridge_->isInitialized()) {
        json error;
        error["error"] = "JavaScript bridge not initialized";
        return error;
    }
    
    return jsBridge_->callJavaScriptFunction(function, args);
}

json MultiLanguageConnector::callCpp(const String& function, const json& args) {
    // This would call registered C++ functions
    json error;
    error["error"] = "C++ function calling not implemented";
    return error;
}

void MultiLanguageConnector::setSearchEngineEndpoint(const String& endpoint) {
    browserConnector_->setEndpoint(endpoint);
}

void MultiLanguageConnector::setPythonPath(const String& path) {
    pythonBridge_->setPythonPath(path);
}

// Utility functions
namespace connector_utils {
    String serializeMessage(const BrowserMessage& message) {
        return message.serialize();
    }
    
    BrowserMessage deserializeMessage(const String& data) {
        return BrowserMessage::deserialize(data);
    }
    
    json createSearchRequest(const String& query, const json& options) {
        json request = options;
        request["query"] = query;
        request["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        return request;
    }
    
    json createPageLoadRequest(const String& url) {
        json request;
        request["url"] = url;
        request["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        return request;
    }
    
    json createErrorResponse(const String& error) {
        json response;
        response["error"] = error;
        response["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        return response;
    }
    
    String buildSearchUrl(const String& endpoint, const String& query) {
        return endpoint + "/api/search?q=" + query;
    }
    
    String buildApiUrl(const String& endpoint, const String& path) {
        return endpoint + path;
    }
    
    bool isValidMessage(const BrowserMessage& message) {
        return !message.id.empty() && message.timestamp > 0;
    }
    
    bool isValidEndpoint(const String& endpoint) {
        return endpoint.find("http://") == 0 || endpoint.find("https://") == 0;
    }
    
    String formatError(const String& error, const String& context) {
        if (context.empty()) {
            return error;
        }
        return context + ": " + error;
    }
}

// Stub implementations for Python and JavaScript bridges

PythonBridge::PythonBridge() : initialized_(false) {}
PythonBridge::~PythonBridge() { finalize(); }

bool PythonBridge::initialize() {
    // TODO: Initialize Python interpreter
    initialized_ = false; // Set to false until proper implementation
    return initialized_;
}

void PythonBridge::finalize() {
    // TODO: Finalize Python interpreter
    initialized_ = false;
}

bool PythonBridge::isInitialized() const {
    return initialized_;
}

json PythonBridge::executePython(const String& code) {
    json error;
    error["error"] = "Python bridge not implemented";
    return error;
}

json PythonBridge::callPythonFunction(const String& module, const String& function, const json& args) {
    json error;
    error["error"] = "Python bridge not implemented";
    return error;
}

json PythonBridge::search(const String& query, const json& options) {
    json error;
    error["error"] = "Python bridge not implemented";
    return error;
}

json PythonBridge::getSearchSuggestions(const String& query) {
    json error;
    error["error"] = "Python bridge not implemented";
    return error;
}

json PythonBridge::getTrendingSearches() {
    json error;
    error["error"] = "Python bridge not implemented";
    return error;
}

json PythonBridge::analyzeContent(const String& content) {
    json error;
    error["error"] = "Python bridge not implemented";
    return error;
}

json PythonBridge::generateSummary(const String& content) {
    json error;
    error["error"] = "Python bridge not implemented";
    return error;
}

json PythonBridge::extractKeywords(const String& content) {
    json error;
    error["error"] = "Python bridge not implemented";
    return error;
}

void PythonBridge::setPythonPath(const String& path) {
    // TODO: Set Python path
}

void PythonBridge::addModulePath(const String& path) {
    // TODO: Add module path
}

JavaScriptBridge::JavaScriptBridge() : initialized_(false) {}
JavaScriptBridge::~JavaScriptBridge() { finalize(); }

bool JavaScriptBridge::initialize() {
    // TODO: Initialize JavaScript engine
    initialized_ = false; // Set to false until proper implementation
    return initialized_;
}

void JavaScriptBridge::finalize() {
    // TODO: Finalize JavaScript engine
    initialized_ = false;
}

bool JavaScriptBridge::isInitialized() const {
    return initialized_;
}

json JavaScriptBridge::executeJavaScript(const String& code) {
    json error;
    error["error"] = "JavaScript bridge not implemented";
    return error;
}

json JavaScriptBridge::callJavaScriptFunction(const String& function, const json& args) {
    json error;
    error["error"] = "JavaScript bridge not implemented";
    return error;
}

json JavaScriptBridge::evaluateSelector(const String& selector, const String& html) {
    json error;
    error["error"] = "JavaScript bridge not implemented";
    return error;
}

json JavaScriptBridge::executeScript(const String& script, const String& html) {
    json error;
    error["error"] = "JavaScript bridge not implemented";
    return error;
}

void JavaScriptBridge::addEventListener(const String& event, std::function<void(const json&)> handler) {
    eventHandlers_[event] = handler;
}

void JavaScriptBridge::removeEventListener(const String& event) {
    eventHandlers_.erase(event);
}

void JavaScriptBridge::dispatchEvent(const String& event, const json& data) {
    auto it = eventHandlers_.find(event);
    if (it != eventHandlers_.end()) {
        it->second(data);
    }
}

} // namespace zepra
