/**
 * @file InspectorProtocol.h
 * @brief Web Inspector Protocol Implementation
 * 
 * Chrome DevTools Protocol (CDP) compatible:
 * - Debugger domain
 * - Runtime domain
 * - Profiler domain
 * - Console domain
 */

#pragma once

#include "../core/EmbedderAPI.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Zepra::Inspector {

// =============================================================================
// Protocol Message
// =============================================================================

/**
 * @brief JSON-RPC style message
 */
struct ProtocolMessage {
    int32_t id = -1;          // Request ID (-1 for events)
    std::string method;        // Domain.method
    std::string params;        // JSON params
    std::string result;        // JSON result (responses only)
    std::string error;         // Error message (if any)
    
    bool IsRequest() const { return id >= 0 && result.empty(); }
    bool IsResponse() const { return id >= 0 && !result.empty(); }
    bool IsEvent() const { return id < 0; }
};

// =============================================================================
// Inspector Agent Base
// =============================================================================

/**
 * @brief Base class for protocol domain agents
 */
class InspectorAgent {
public:
    virtual ~InspectorAgent() = default;
    
    virtual std::string DomainName() const = 0;
    
    // Handle method call
    virtual std::string HandleMethod(const std::string& method,
                                      const std::string& params) = 0;
    
    // Enable/disable domain
    virtual void Enable() { enabled_ = true; }
    virtual void Disable() { enabled_ = false; }
    bool IsEnabled() const { return enabled_; }
    
protected:
    bool enabled_ = false;
};

// =============================================================================
// Debugger Agent
// =============================================================================

/**
 * @brief Debugger domain implementation
 */
class DebuggerAgent : public InspectorAgent {
public:
    std::string DomainName() const override { return "Debugger"; }
    
    std::string HandleMethod(const std::string& method,
                             const std::string& params) override {
        if (method == "enable") {
            Enable();
            return "{}";
        } else if (method == "disable") {
            Disable();
            return "{}";
        } else if (method == "setBreakpointByUrl") {
            return SetBreakpointByUrl(params);
        } else if (method == "removeBreakpoint") {
            return RemoveBreakpoint(params);
        } else if (method == "pause") {
            return Pause();
        } else if (method == "resume") {
            return Resume();
        } else if (method == "stepOver") {
            return StepOver();
        } else if (method == "stepInto") {
            return StepInto();
        } else if (method == "stepOut") {
            return StepOut();
        } else if (method == "getScriptSource") {
            return GetScriptSource(params);
        }
        return R"({"error": "Method not found"})";
    }
    
    // Event emitters
    using EventCallback = std::function<void(const std::string&, const std::string&)>;
    void SetEventCallback(EventCallback cb) { eventCallback_ = cb; }
    
    void EmitPaused(const std::string& reason, const std::string& callFrames) {
        if (eventCallback_) {
            eventCallback_("Debugger.paused", 
                R"({"reason":")" + reason + R"(","callFrames":)" + callFrames + "}");
        }
    }
    
    void EmitResumed() {
        if (eventCallback_) {
            eventCallback_("Debugger.resumed", "{}");
        }
    }
    
private:
    std::string SetBreakpointByUrl(const std::string& params);
    std::string RemoveBreakpoint(const std::string& params);
    std::string Pause();
    std::string Resume();
    std::string StepOver();
    std::string StepInto();
    std::string StepOut();
    std::string GetScriptSource(const std::string& params);
    
    EventCallback eventCallback_;
};

// =============================================================================
// Runtime Agent
// =============================================================================

/**
 * @brief Runtime domain implementation
 */
class RuntimeAgent : public InspectorAgent {
public:
    std::string DomainName() const override { return "Runtime"; }
    
    std::string HandleMethod(const std::string& method,
                             const std::string& params) override {
        if (method == "enable") {
            Enable();
            return "{}";
        } else if (method == "evaluate") {
            return Evaluate(params);
        } else if (method == "callFunctionOn") {
            return CallFunctionOn(params);
        } else if (method == "getProperties") {
            return GetProperties(params);
        } else if (method == "releaseObject") {
            return ReleaseObject(params);
        }
        return R"({"error": "Method not found"})";
    }
    
    // Console API integration
    using ConsoleCallback = std::function<void(const std::string&, const std::string&)>;
    void SetConsoleCallback(ConsoleCallback cb) { consoleCallback_ = cb; }
    
    void EmitConsoleAPICalled(const std::string& type, 
                               const std::vector<std::string>& args) {
        if (consoleCallback_) {
            std::string argsJson = "[";
            for (size_t i = 0; i < args.size(); i++) {
                if (i > 0) argsJson += ",";
                argsJson += args[i];
            }
            argsJson += "]";
            
            consoleCallback_("Runtime.consoleAPICalled",
                R"({"type":")" + type + R"(","args":)" + argsJson + "}");
        }
    }
    
private:
    std::string Evaluate(const std::string& params);
    std::string CallFunctionOn(const std::string& params);
    std::string GetProperties(const std::string& params);
    std::string ReleaseObject(const std::string& params);
    
    ConsoleCallback consoleCallback_;
};

// =============================================================================
// Profiler Agent
// =============================================================================

/**
 * @brief Profiler domain implementation
 */
class ProfilerAgent : public InspectorAgent {
public:
    std::string DomainName() const override { return "Profiler"; }
    
    std::string HandleMethod(const std::string& method,
                             const std::string& params) override {
        if (method == "enable") {
            Enable();
            return "{}";
        } else if (method == "start") {
            return Start();
        } else if (method == "stop") {
            return Stop();
        } else if (method == "setSamplingInterval") {
            return SetSamplingInterval(params);
        }
        return R"({"error": "Method not found"})";
    }
    
private:
    std::string Start();
    std::string Stop();
    std::string SetSamplingInterval(const std::string& params);
};

// =============================================================================
// HeapProfiler Agent
// =============================================================================

/**
 * @brief HeapProfiler domain implementation
 */
class HeapProfilerAgent : public InspectorAgent {
public:
    std::string DomainName() const override { return "HeapProfiler"; }
    
    std::string HandleMethod(const std::string& method,
                             const std::string& params) override {
        if (method == "enable") {
            Enable();
            return "{}";
        } else if (method == "takeHeapSnapshot") {
            return TakeHeapSnapshot();
        } else if (method == "startTrackingHeapObjects") {
            return StartTrackingHeapObjects();
        } else if (method == "stopTrackingHeapObjects") {
            return StopTrackingHeapObjects();
        }
        return R"({"error": "Method not found"})";
    }
    
private:
    std::string TakeHeapSnapshot();
    std::string StartTrackingHeapObjects();
    std::string StopTrackingHeapObjects();
};

// =============================================================================
// Inspector Session
// =============================================================================

/**
 * @brief Debug session managing all agents
 */
class InspectorSession {
public:
    InspectorSession() {
        RegisterAgent(std::make_unique<DebuggerAgent>());
        RegisterAgent(std::make_unique<RuntimeAgent>());
        RegisterAgent(std::make_unique<ProfilerAgent>());
        RegisterAgent(std::make_unique<HeapProfilerAgent>());
    }
    
    // Register custom agent
    void RegisterAgent(std::unique_ptr<InspectorAgent> agent) {
        std::string domain = agent->DomainName();
        agents_[domain] = std::move(agent);
    }
    
    // Dispatch incoming message
    std::string DispatchMessage(const std::string& messageJson) {
        ProtocolMessage msg = ParseMessage(messageJson);
        
        // Extract domain from method
        auto dotPos = msg.method.find('.');
        if (dotPos == std::string::npos) {
            return R"({"id":)" + std::to_string(msg.id) + R"(,"error":"Invalid method"})";
        }
        
        std::string domain = msg.method.substr(0, dotPos);
        std::string method = msg.method.substr(dotPos + 1);
        
        auto it = agents_.find(domain);
        if (it == agents_.end()) {
            return R"({"id":)" + std::to_string(msg.id) + R"(,"error":"Domain not found"})";
        }
        
        std::string result = it->second->HandleMethod(method, msg.params);
        return R"({"id":)" + std::to_string(msg.id) + R"(,"result":)" + result + "}";
    }
    
    // Event callback (for sending events to frontend)
    using EventCallback = std::function<void(const std::string&)>;
    void SetEventCallback(EventCallback cb) { eventCallback_ = cb; }
    
    void SendEvent(const std::string& method, const std::string& params) {
        if (eventCallback_) {
            std::string event = R"({"method":")" + method + R"(","params":)" + params + "}";
            eventCallback_(event);
        }
    }
    
private:
    ProtocolMessage ParseMessage(const std::string& json) {
        ProtocolMessage msg;
        // Would parse JSON here
        return msg;
    }
    
    std::unordered_map<std::string, std::unique_ptr<InspectorAgent>> agents_;
    EventCallback eventCallback_;
};

} // namespace Zepra::Inspector
