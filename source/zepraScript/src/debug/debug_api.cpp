/**
 * @file debug_api.cpp
 * @brief Unified debug API facade
 * 
 * Provides a single entry point for all debugging functionality:
 * - Zero overhead when debugging is disabled
 * - Callback registration for debug events
 * - Expression evaluation in execution context
 * - Object inspection and property enumeration
 */

#include "zeprascript/debug/debugger.hpp"
#include "zeprascript/runtime/value.hpp"
#include "zeprascript/runtime/object.hpp"
#include "zeprascript/config.hpp"
#include <functional>
#include <unordered_map>
#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace Zepra::Debug {

using Runtime::Value;
using Runtime::Object;

// =============================================================================
// PropertyInfo - For object inspection
// =============================================================================

struct PropertyInfo {
    std::string name;
    Value value;
    bool writable;
    bool enumerable;
    bool configurable;
    bool isGetter;
    bool isSetter;
    bool isSymbol;
    
    PropertyInfo()
        : writable(true), enumerable(true), configurable(true)
        , isGetter(false), isSetter(false), isSymbol(false) {}
};

// =============================================================================
// ObjectPreview - Compact representation of an object
// =============================================================================

struct ObjectPreview {
    std::string type;           // "object", "array", "function", etc.
    std::string subtype;        // "null", "regexp", "date", etc.
    std::string className;      // Object constructor name
    std::string description;    // Short description
    bool overflow;              // True if preview is truncated
    std::vector<PropertyInfo> properties;
    
    static constexpr size_t MAX_PREVIEW_PROPERTIES = 5;
};

// =============================================================================
// DebugConfig - Configuration for debug API
// =============================================================================

struct DebugConfig {
    bool enabled = true;
    bool pauseOnEntry = false;
    bool pauseOnExit = false;
    size_t maxObjectPreviewDepth = 3;
    size_t maxArrayPreviewLength = 100;
    size_t maxStringPreviewLength = 100;
};

// =============================================================================
// EventType - Debug event types
// =============================================================================

enum class EventType {
    Paused,         // Execution paused
    Resumed,        // Execution resumed
    ScriptParsed,   // New script parsed
    ScriptFailed,   // Script parse failed
    BreakpointResolved,  // Breakpoint resolved to bytecode
    ConsoleMessage, // Console API called
    Exception       // Exception thrown
};

// =============================================================================
// EventData - Data associated with debug events
// =============================================================================

struct EventData {
    EventType type;
    std::string reason;         // Pause reason, error message, etc.
    std::string scriptUrl;      // For script events
    uint32_t scriptId;          // For script events
    uint32_t breakpointId;      // For breakpoint events
    size_t line;
    size_t column;
    
    EventData() : type(EventType::Paused), scriptId(0), breakpointId(0)
        , line(0), column(0) {}
};

// =============================================================================
// DebugAPI - Unified debug interface
// =============================================================================

class DebugAPI {
public:
    using EventCallback = std::function<void(const EventData&)>;
    using EvalCallback = std::function<Value(const std::string& expression, size_t frameIndex)>;
    
    DebugAPI() = default;
    
    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------
    
    /**
     * @brief Enable/disable debug API
     * When disabled, all debug operations become no-ops
     */
    void setEnabled(bool enabled) { config_.enabled = enabled; }
    bool isEnabled() const { return config_.enabled; }
    
    /**
     * @brief Set configuration
     */
    void configure(const DebugConfig& config) { config_ = config; }
    const DebugConfig& config() const { return config_; }
    
    // -------------------------------------------------------------------------
    // Event Registration
    // -------------------------------------------------------------------------
    
    /**
     * @brief Register callback for debug events
     * @return Registration ID for later removal
     */
    uint32_t addEventListener(EventCallback callback) {
        uint32_t id = nextCallbackId_++;
        callbacks_[id] = std::move(callback);
        return id;
    }
    
    /**
     * @brief Remove event listener
     */
    bool removeEventListener(uint32_t id) {
        return callbacks_.erase(id) > 0;
    }
    
    /**
     * @brief Fire an event to all listeners
     */
    void fireEvent(const EventData& event) {
        if (!config_.enabled) return;
        
        for (const auto& [id, callback] : callbacks_) {
            callback(event);
        }
    }
    
    // -------------------------------------------------------------------------
    // Expression Evaluation
    // -------------------------------------------------------------------------
    
    /**
     * @brief Set evaluation callback (connects to VM)
     */
    void setEvalCallback(EvalCallback callback) {
        evalCallback_ = std::move(callback);
    }
    
    /**
     * @brief Evaluate expression in a specific frame context
     */
    Value evaluate(const std::string& expression, size_t frameIndex = 0) {
        if (!config_.enabled || !evalCallback_) {
            return Value::undefined();
        }
        return evalCallback_(expression, frameIndex);
    }
    
    /**
     * @brief Evaluate expression and return as string
     */
    std::string evaluateToString(const std::string& expression, 
                                  size_t frameIndex = 0) {
        Value result = evaluate(expression, frameIndex);
        return formatValue(result);
    }
    
    // -------------------------------------------------------------------------
    // Object Inspection
    // -------------------------------------------------------------------------
    
    /**
     * @brief Get properties of an object
     */
    std::vector<PropertyInfo> getProperties(Object* obj, 
                                             bool ownOnly = true,
                                             bool includeAccessors = true) {
        std::vector<PropertyInfo> result;
        if (!obj || !config_.enabled) return result;
        
        // Get own properties
        auto props = obj->getOwnPropertyNames();
        for (const auto& name : props) {
            PropertyInfo info;
            info.name = name;
            info.value = obj->get(name);
            info.enumerable = true;
            info.writable = true;
            info.configurable = true;
            // TODO: Get actual property attributes
            result.push_back(info);
        }
        
        // Get prototype properties if requested
        if (!ownOnly && obj->prototype()) {
            auto protoProps = getProperties(obj->prototype(), false, includeAccessors);
            for (auto& prop : protoProps) {
                // Skip if already in own properties
                bool skip = false;
                for (const auto& own : result) {
                    if (own.name == prop.name) {
                        skip = true;
                        break;
                    }
                }
                if (!skip) {
                    result.push_back(std::move(prop));
                }
            }
        }
        
        return result;
    }
    
    /**
     * @brief Get a short preview of an object
     */
    ObjectPreview getPreview(Value value, size_t depth = 0) {
        ObjectPreview preview;
        
        if (value.isUndefined()) {
            preview.type = "undefined";
            preview.description = "undefined";
            return preview;
        }
        
        if (value.isNull()) {
            preview.type = "object";
            preview.subtype = "null";
            preview.description = "null";
            return preview;
        }
        
        if (value.isBoolean()) {
            preview.type = "boolean";
            preview.description = value.asBoolean() ? "true" : "false";
            return preview;
        }
        
        if (value.isNumber()) {
            preview.type = "number";
            preview.description = std::to_string(value.asNumber());
            return preview;
        }
        
        if (value.isString()) {
            preview.type = "string";
            std::string str = value.toString();
            if (str.length() > config_.maxStringPreviewLength) {
                str = str.substr(0, config_.maxStringPreviewLength) + "...";
            }
            preview.description = "\"" + str + "\"";
            return preview;
        }
        
        if (value.isObject()) {
            Object* obj = value.asObject();
            
            // Check for array
            if (obj->isArray()) {
                preview.type = "object";
                preview.subtype = "array";
                size_t length = obj->length();
                preview.description = "Array(" + std::to_string(length) + ")";
                preview.overflow = length > ObjectPreview::MAX_PREVIEW_PROPERTIES;
                
                // Add array elements
                if (depth < config_.maxObjectPreviewDepth) {
                    size_t count = std::min(length, ObjectPreview::MAX_PREVIEW_PROPERTIES);
                    for (size_t i = 0; i < count; ++i) {
                        PropertyInfo prop;
                        prop.name = std::to_string(i);
                        prop.value = obj->get(i);
                        preview.properties.push_back(prop);
                    }
                }
                return preview;
            }
            
            // Check for function
            if (obj->isFunction()) {
                preview.type = "function";
                preview.description = "function";
                // TODO: Get function name
                return preview;
            }
            
            // Regular object
            preview.type = "object";
            preview.className = "Object"; // TODO: Get actual class name
            preview.description = "Object";
            
            if (depth < config_.maxObjectPreviewDepth) {
                auto props = getProperties(obj, true, false);
                preview.overflow = props.size() > ObjectPreview::MAX_PREVIEW_PROPERTIES;
                
                size_t count = std::min(props.size(), ObjectPreview::MAX_PREVIEW_PROPERTIES);
                for (size_t i = 0; i < count; ++i) {
                    preview.properties.push_back(props[i]);
                }
            }
            
            return preview;
        }
        
        preview.type = "unknown";
        preview.description = "unknown";
        return preview;
    }
    
    /**
     * @brief Format a value as a string for display
     */
    std::string formatValue(Value value, size_t depth = 0) {
        if (depth > config_.maxObjectPreviewDepth) {
            return "...";
        }
        
        auto preview = getPreview(value, depth);
        
        if (preview.type == "object" && !preview.properties.empty()) {
            std::string result = "{";
            for (size_t i = 0; i < preview.properties.size(); ++i) {
                if (i > 0) result += ", ";
                result += preview.properties[i].name + ": ";
                result += formatValue(preview.properties[i].value, depth + 1);
            }
            if (preview.overflow) {
                result += ", ...";
            }
            result += "}";
            return result;
        }
        
        return preview.description;
    }
    
    // -------------------------------------------------------------------------
    // Script Management
    // -------------------------------------------------------------------------
    
    /**
     * @brief Notify that a script was parsed
     */
    void notifyScriptParsed(uint32_t scriptId, const std::string& url,
                            const std::string& source) {
        if (!config_.enabled) return;
        
        scripts_[scriptId] = {url, source};
        
        EventData event;
        event.type = EventType::ScriptParsed;
        event.scriptId = scriptId;
        event.scriptUrl = url;
        fireEvent(event);
    }
    
    /**
     * @brief Get script source by ID
     */
    std::optional<std::string> getScriptSource(uint32_t scriptId) const {
        auto it = scripts_.find(scriptId);
        if (it != scripts_.end()) {
            return it->second.source;
        }
        return std::nullopt;
    }
    
    /**
     * @brief Get script URL by ID
     */
    std::optional<std::string> getScriptUrl(uint32_t scriptId) const {
        auto it = scripts_.find(scriptId);
        if (it != scripts_.end()) {
            return it->second.url;
        }
        return std::nullopt;
    }
    
    // -------------------------------------------------------------------------
    // Console Integration
    // -------------------------------------------------------------------------
    
    /**
     * @brief Notify console message
     */
    void notifyConsoleMessage(const std::string& level, 
                               const std::string& message,
                               const std::string& url = "",
                               size_t line = 0) {
        if (!config_.enabled) return;
        
        EventData event;
        event.type = EventType::ConsoleMessage;
        event.reason = level + ": " + message;
        event.scriptUrl = url;
        event.line = line;
        fireEvent(event);
    }
    
    // -------------------------------------------------------------------------
    // Statistics
    // -------------------------------------------------------------------------
    
    /**
     * @brief Get debug API statistics
     */
    struct Stats {
        size_t eventsFired = 0;
        size_t expressionsEvaluated = 0;
        size_t objectsInspected = 0;
        size_t scriptsTracked = 0;
        size_t callbacksRegistered = 0;
    };
    
    Stats getStats() const {
        Stats s;
        s.scriptsTracked = scripts_.size();
        s.callbacksRegistered = callbacks_.size();
        return s;
    }
    
private:
    struct ScriptInfo {
        std::string url;
        std::string source;
    };
    
    DebugConfig config_;
    std::unordered_map<uint32_t, EventCallback> callbacks_;
    std::unordered_map<uint32_t, ScriptInfo> scripts_;
    EvalCallback evalCallback_;
    uint32_t nextCallbackId_ = 1;
};

// =============================================================================
// Global DebugAPI Instance
// =============================================================================

static DebugAPI* globalDebugAPI = nullptr;

DebugAPI* getDebugAPI() {
    if (!globalDebugAPI) {
        globalDebugAPI = new DebugAPI();
    }
    return globalDebugAPI;
}

void shutdownDebugAPI() {
    delete globalDebugAPI;
    globalDebugAPI = nullptr;
}

// =============================================================================
// Zero-Overhead Macros
// =============================================================================

// These macros provide zero overhead when ZEPRA_DEBUG_ENABLED is false
#ifdef ZEPRA_DEBUG_ENABLED
    #define DEBUG_FIRE_EVENT(event) if (globalDebugAPI) globalDebugAPI->fireEvent(event)
    #define DEBUG_NOTIFY_SCRIPT(id, url, src) if (globalDebugAPI) globalDebugAPI->notifyScriptParsed(id, url, src)
    #define DEBUG_CONSOLE_MSG(lvl, msg) if (globalDebugAPI) globalDebugAPI->notifyConsoleMessage(lvl, msg)
#else
    #define DEBUG_FIRE_EVENT(event) ((void)0)
    #define DEBUG_NOTIFY_SCRIPT(id, url, src) ((void)0)
    #define DEBUG_CONSOLE_MSG(lvl, msg) ((void)0)
#endif

} // namespace Zepra::Debug
