/**
 * @file DevToolsAPI.h
 * @brief Native DevTools Integration for ZepraScript
 * 
 * Provides debugging, profiling, and memory inspection APIs.
 */

#pragma once

#include "../config.hpp"
#include "runtime/objects/value.hpp"
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace Zepra::DevTools {

using Runtime::Value;

// =============================================================================
// Stack Frame
// =============================================================================

/**
 * @brief Represents a call stack frame
 */
struct StackFrame {
    uint32_t frameId;
    std::string functionName;
    std::string scriptUrl;
    uint32_t lineNumber;
    uint32_t columnNumber;
    bool isAsync = false;
    bool isNative = false;
    
    // Scope chain
    std::vector<uint32_t> scopeIds;
};

// =============================================================================
// Variable/Property
// =============================================================================

/**
 * @brief Represents an inspectable variable
 */
struct Variable {
    std::string name;
    std::string type;       // "undefined", "number", "string", "object", etc.
    std::string value;      // String representation
    uint32_t objectId = 0;  // For expandable objects
    bool writable = true;
    bool configurable = true;
    bool enumerable = true;
    
    // For objects/arrays
    size_t propertyCount = 0;
    bool hasChildren = false;
};

// =============================================================================
// Scope
// =============================================================================

/**
 * @brief Represents a scope (local, closure, global)
 */
struct Scope {
    enum class Type { Local, Closure, Script, Global, Block };
    
    uint32_t scopeId;
    Type type;
    std::string name;  // For closure: name of function
    
    std::vector<Variable> variables;
};

// =============================================================================
// Breakpoint
// =============================================================================

/**
 * @brief Represents a breakpoint
 */
struct Breakpoint {
    uint32_t breakpointId;
    std::string url;
    uint32_t lineNumber;
    uint32_t columnNumber = 0;
    std::string condition;  // Conditional breakpoint expression
    bool enabled = true;
    uint32_t hitCount = 0;
};

// =============================================================================
// Pause Reason
// =============================================================================

enum class PauseReason {
    Breakpoint,
    DebuggerStatement,
    Exception,
    StepComplete,
    AsyncStackComplete
};

// =============================================================================
// Inspector
// =============================================================================

/**
 * @brief Main debugging inspector interface
 */
class Inspector {
public:
    static Inspector& instance();
    
    // =========================================================================
    // Execution Control
    // =========================================================================
    
    /**
     * @brief Pause script execution
     */
    void pause();
    
    /**
     * @brief Resume execution
     */
    void resume();
    
    /**
     * @brief Step over current statement
     */
    void stepOver();
    
    /**
     * @brief Step into function call
     */
    void stepInto();
    
    /**
     * @brief Step out of current function
     */
    void stepOut();
    
    /**
     * @brief Is execution currently paused?
     */
    bool isPaused() const { return paused_; }
    
    // =========================================================================
    // Breakpoints
    // =========================================================================
    
    /**
     * @brief Set a breakpoint
     * @return Breakpoint ID
     */
    uint32_t setBreakpoint(const std::string& url, uint32_t line, 
                           uint32_t column = 0, const std::string& condition = "");
    
    /**
     * @brief Remove a breakpoint
     */
    void removeBreakpoint(uint32_t breakpointId);
    
    /**
     * @brief Enable/disable a breakpoint
     */
    void setBreakpointEnabled(uint32_t breakpointId, bool enabled);
    
    /**
     * @brief Get all breakpoints
     */
    std::vector<Breakpoint> getBreakpoints() const { return breakpoints_; }
    
    /**
     * @brief Set exception breakpoint
     */
    void setBreakOnExceptions(bool caught, bool uncaught);
    
    // =========================================================================
    // Stack & Scope Inspection
    // =========================================================================
    
    /**
     * @brief Get current call stack
     */
    std::vector<StackFrame> getCallStack() const;
    
    /**
     * @brief Get scope for a frame
     */
    std::vector<Scope> getScopes(uint32_t frameId) const;
    
    /**
     * @brief Get variables in a scope
     */
    std::vector<Variable> getVariables(uint32_t scopeId) const;
    
    /**
     * @brief Get object properties
     */
    std::vector<Variable> getObjectProperties(uint32_t objectId) const;
    
    // =========================================================================
    // Evaluation
    // =========================================================================
    
    /**
     * @brief Evaluate expression in frame context
     */
    Value evaluate(const std::string& expression, uint32_t frameId = 0);
    
    /**
     * @brief Set variable value
     */
    bool setVariableValue(uint32_t scopeId, const std::string& name, const Value& value);
    
    // =========================================================================
    // Callbacks
    // =========================================================================
    
    using PausedCallback = std::function<void(PauseReason, const std::vector<StackFrame>&)>;
    using ResumedCallback = std::function<void()>;
    
    void setOnPaused(PausedCallback callback) { onPaused_ = std::move(callback); }
    void setOnResumed(ResumedCallback callback) { onResumed_ = std::move(callback); }
    
private:
    Inspector();
    
    bool paused_ = false;
    bool breakOnCaughtExceptions_ = false;
    bool breakOnUncaughtExceptions_ = true;
    
    std::vector<Breakpoint> breakpoints_;
    uint32_t nextBreakpointId_ = 1;
    
    PausedCallback onPaused_;
    ResumedCallback onResumed_;
};

// =============================================================================
// Profiler
// =============================================================================

/**
 * @brief CPU profile node
 */
struct ProfileNode {
    uint32_t nodeId;
    std::string functionName;
    std::string url;
    uint32_t lineNumber;
    uint32_t columnNumber;
    uint64_t hitCount = 0;
    std::vector<uint32_t> children;
    
    // Timing
    double selfTime = 0;    // Time in this function only
    double totalTime = 0;   // Time including children
};

/**
 * @brief CPU profile result
 */
struct ProfileResult {
    std::vector<ProfileNode> nodes;
    uint64_t startTime;
    uint64_t endTime;
    std::vector<uint32_t> samples;      // Node IDs
    std::vector<uint64_t> timeDeltas;   // Microseconds
};

/**
 * @brief CPU profiler
 */
class Profiler {
public:
    static Profiler& instance();
    
    void start();
    ProfileResult stop();
    bool isRunning() const { return running_; }
    
    // Sampling rate (default: 1000 samples/sec)
    void setSamplingInterval(uint32_t microseconds);
    
private:
    Profiler();
    
    bool running_ = false;
    uint32_t samplingInterval_ = 1000;
    uint64_t startTime_ = 0;
    std::vector<ProfileNode> nodes_;
    std::vector<uint32_t> samples_;
    std::vector<uint64_t> timeDeltas_;
};

// =============================================================================
// Memory Inspector
// =============================================================================

/**
 * @brief Heap snapshot node
 */
struct HeapNode {
    uint32_t nodeId;
    std::string name;
    std::string type;
    size_t selfSize;
    size_t retainedSize;
    std::vector<uint32_t> children;
    uint32_t edgeCount = 0;
};

/**
 * @brief Heap snapshot
 */
struct HeapSnapshot {
    std::vector<HeapNode> nodes;
    size_t totalSize;
    size_t totalObjects;
    uint64_t timestamp;
};

/**
 * @brief Allocation record
 */
struct AllocationRecord {
    std::string type;
    size_t size;
    std::vector<StackFrame> stack;
    uint64_t timestamp;
};

/**
 * @brief Memory inspector
 */
class MemoryInspector {
public:
    static MemoryInspector& instance();
    
    /**
     * @brief Take a heap snapshot
     */
    HeapSnapshot takeHeapSnapshot();
    
    /**
     * @brief Start tracking allocations
     */
    void startAllocationTracking();
    
    /**
     * @brief Stop tracking and get allocations
     */
    std::vector<AllocationRecord> stopAllocationTracking();
    
    /**
     * @brief Get current memory usage
     */
    struct MemoryUsage {
        size_t usedHeapSize;
        size_t totalHeapSize;
        size_t heapSizeLimit;
        size_t externalMemory;
    };
    
    MemoryUsage getMemoryUsage() const;
    
    /**
     * @brief Force garbage collection (debug only)
     */
    void forceGC();
    
private:
    MemoryInspector();
    
    bool trackingAllocations_ = false;
    std::vector<AllocationRecord> allocations_;
};

// =============================================================================
// Console API (for DevTools)
// =============================================================================

/**
 * @brief Console message for DevTools
 */
struct ConsoleMessage {
    enum class Level { Log, Info, Warn, Error, Debug };
    
    Level level;
    std::string message;
    std::string url;
    uint32_t lineNumber;
    uint32_t columnNumber;
    uint64_t timestamp;
    std::vector<Value> args;
};

/**
 * @brief Console message handler
 */
class ConsoleHandler {
public:
    static ConsoleHandler& instance();
    
    using MessageCallback = std::function<void(const ConsoleMessage&)>;
    void setOnMessage(MessageCallback callback) { onMessage_ = std::move(callback); }
    
    // Log methods
    void log(const std::vector<Value>& args);
    void info(const std::vector<Value>& args);
    void warn(const std::vector<Value>& args);
    void error(const std::vector<Value>& args);
    void debug(const std::vector<Value>& args);
    
    void clear();
    std::vector<ConsoleMessage> getMessages() const { return messages_; }
    
private:
    ConsoleHandler();
    
    void addMessage(ConsoleMessage::Level level, const std::vector<Value>& args);
    
    std::vector<ConsoleMessage> messages_;
    MessageCallback onMessage_;
};

// =============================================================================
// Builtin Functions
// =============================================================================

void initDevTools();

} // namespace Zepra::DevTools
