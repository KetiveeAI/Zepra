#pragma once

/**
 * @file devtools_protocol.hpp
 * @brief Chrome DevTools Protocol implementation
 */

#include "../config.hpp"
#include <string>
#include <functional>
#include <unordered_map>

namespace Zepra::Debug {

/**
 * @brief CDP (Chrome DevTools Protocol) message
 */
struct CDPMessage {
    int id = 0;
    std::string method;
    std::string params;   // JSON string
    std::string result;   // JSON string
    std::string error;    // JSON string
    
    bool isRequest() const { return !method.empty() && id > 0; }
    bool isResponse() const { return id > 0 && method.empty(); }
    bool isEvent() const { return !method.empty() && id == 0; }
};

/**
 * @brief Handler for CDP methods
 */
using CDPHandler = std::function<std::string(const std::string& params)>;

/**
 * @brief Chrome DevTools Protocol server
 * 
 * Implements CDP for debugging ZepraScript from Chrome DevTools.
 */
class DevToolsProtocol {
public:
    DevToolsProtocol();
    
    /**
     * @brief Handle incoming CDP message
     */
    std::string handleMessage(const std::string& message);
    
    /**
     * @brief Send event to DevTools
     */
    void sendEvent(const std::string& method, const std::string& params);
    
    /**
     * @brief Set message send callback
     */
    using SendCallback = std::function<void(const std::string& message)>;
    void setSendCallback(SendCallback callback) { sendCallback_ = std::move(callback); }
    
    // --- Domain Handlers ---
    
    // Runtime domain
    std::string runtimeEnable(const std::string& params);
    std::string runtimeEvaluate(const std::string& params);
    std::string runtimeGetProperties(const std::string& params);
    std::string runtimeCallFunctionOn(const std::string& params);
    
    // Debugger domain
    std::string debuggerEnable(const std::string& params);
    std::string debuggerDisable(const std::string& params);
    std::string debuggerSetBreakpointByUrl(const std::string& params);
    std::string debuggerRemoveBreakpoint(const std::string& params);
    std::string debuggerPause(const std::string& params);
    std::string debuggerResume(const std::string& params);
    std::string debuggerStepInto(const std::string& params);
    std::string debuggerStepOver(const std::string& params);
    std::string debuggerStepOut(const std::string& params);
    std::string debuggerGetScriptSource(const std::string& params);
    
    // Profiler domain
    std::string profilerEnable(const std::string& params);
    std::string profilerStart(const std::string& params);
    std::string profilerStop(const std::string& params);
    
    // HeapProfiler domain
    std::string heapProfilerTakeSnapshot(const std::string& params);
    std::string heapProfilerStartTrackingAllocations(const std::string& params);
    
    // Console domain
    std::string consoleEnable(const std::string& params);
    std::string consoleClear(const std::string& params);
    
    // DOM domain
    std::string domGetDocument(const std::string& params);
    std::string domQuerySelector(const std::string& params);
    std::string domGetAttributes(const std::string& params);
    std::string domSetAttributeValue(const std::string& params);
    
    // CSS domain
    std::string cssGetComputedStyle(const std::string& params);
    std::string cssGetMatchedStyles(const std::string& params);
    
private:
    void registerHandlers();
    CDPMessage parseMessage(const std::string& json);
    std::string serializeMessage(const CDPMessage& msg);
    
    std::unordered_map<std::string, CDPHandler> handlers_;
    SendCallback sendCallback_;
    
    bool runtimeEnabled_ = false;
    bool debuggerEnabled_ = false;
    bool profilerEnabled_ = false;
    bool consoleEnabled_ = false;
};

/**
 * @brief WebSocket server for DevTools connection
 */
class DevToolsServer {
public:
    DevToolsServer(int port = 9222);
    ~DevToolsServer();
    
    /**
     * @brief Start the DevTools server
     */
    bool start();
    
    /**
     * @brief Stop the server
     */
    void stop();
    
    /**
     * @brief Check if server is running
     */
    bool isRunning() const { return running_; }
    
    /**
     * @brief Process incoming connections
     */
    void processConnections();
    
    /**
     * @brief Get debug URL for Chrome
     */
    std::string getDebugUrl() const;
    
private:
    int port_;
    bool running_ = false;
    DevToolsProtocol protocol_;
};

} // namespace Zepra::Debug
