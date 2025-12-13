#pragma once

/**
 * @file debugger.hpp
 * @brief ZepraScript Debugger - breakpoints, stepping, inspection
 */

#include "../config.hpp"
#include "../runtime/value.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace Zepra::Runtime { class VM; }

namespace Zepra::Debug {

using Runtime::Value;

/**
 * @brief Breakpoint location
 */
struct BreakpointLocation {
    std::string sourceFile;
    uint32_t line;
    uint32_t column = 0;
    
    bool operator==(const BreakpointLocation& other) const {
        return sourceFile == other.sourceFile && line == other.line;
    }
};

/**
 * @brief Breakpoint information
 */
struct Breakpoint {
    uint32_t id;
    BreakpointLocation location;
    bool enabled = true;
    std::string condition;  // Optional conditional expression
    uint32_t hitCount = 0;
};

/**
 * @brief Call frame for stack inspection
 */
struct DebugCallFrame {
    std::string functionName;
    std::string sourceFile;
    uint32_t line;
    uint32_t column;
    std::unordered_map<std::string, Value> locals;
    Value thisValue;
};

/**
 * @brief Debug step mode
 */
enum class StepMode {
    None,       // Normal execution
    StepInto,   // Step into function calls
    StepOver,   // Step over function calls
    StepOut,    // Step out of current function
    Continue    // Continue to next breakpoint
};

/**
 * @brief Debug event types
 */
enum class DebugEvent {
    BreakpointHit,
    StepComplete,
    ExceptionThrown,
    ExecutionPaused,
    ExecutionResumed
};

/**
 * @brief Debug event callback
 */
using DebugCallback = std::function<void(DebugEvent, const Breakpoint*)>;

/**
 * @brief ZepraScript Debugger
 * 
 * Manages breakpoints, stepping, and variable inspection.
 */
class Debugger {
public:
    explicit Debugger(Runtime::VM* vm);
    
    // --- Breakpoint Management ---
    
    /**
     * @brief Set a breakpoint at a location
     * @return Breakpoint ID
     */
    uint32_t setBreakpoint(const std::string& file, uint32_t line);
    
    /**
     * @brief Set a conditional breakpoint
     */
    uint32_t setBreakpoint(const std::string& file, uint32_t line, 
                           const std::string& condition);
    
    /**
     * @brief Remove a breakpoint by ID
     */
    bool removeBreakpoint(uint32_t id);
    
    /**
     * @brief Enable/disable a breakpoint
     */
    void setBreakpointEnabled(uint32_t id, bool enabled);
    
    /**
     * @brief Get all breakpoints
     */
    std::vector<Breakpoint> getBreakpoints() const;
    
    /**
     * @brief Clear all breakpoints
     */
    void clearAllBreakpoints();
    
    // --- Execution Control ---
    
    /**
     * @brief Pause execution
     */
    void pause();
    
    /**
     * @brief Resume execution
     */
    void resume();
    
    /**
     * @brief Step into next statement
     */
    void stepInto();
    
    /**
     * @brief Step over current statement
     */
    void stepOver();
    
    /**
     * @brief Step out of current function
     */
    void stepOut();
    
    /**
     * @brief Check if debugger is paused
     */
    bool isPaused() const { return paused_; }
    
    // --- Call Stack ---
    
    /**
     * @brief Get current call stack
     */
    std::vector<DebugCallFrame> getCallStack() const;
    
    /**
     * @brief Get variables in scope at frame index
     */
    std::unordered_map<std::string, Value> getScopeVariables(size_t frameIndex) const;
    
    // --- Variable Inspection ---
    
    /**
     * @brief Evaluate an expression in current context
     */
    Value evaluate(const std::string& expression);
    
    /**
     * @brief Watch an expression
     */
    void addWatch(const std::string& expression);
    
    /**
     * @brief Get watched expressions and values
     */
    std::vector<std::pair<std::string, Value>> getWatches() const;
    
    // --- Callbacks ---
    
    /**
     * @brief Set debug event callback
     */
    void setCallback(DebugCallback callback);
    
    // --- VM Integration ---
    
    /**
     * @brief Called by VM before executing each instruction
     * @return true to continue, false to pause
     */
    bool onInstruction(const std::string& file, uint32_t line);
    
private:
    bool checkBreakpoint(const std::string& file, uint32_t line);
    void notifyEvent(DebugEvent event, const Breakpoint* bp = nullptr);
    
    Runtime::VM* vm_;
    std::unordered_map<uint32_t, Breakpoint> breakpoints_;
    std::unordered_set<std::string> breakpointLocations_; // "file:line" for fast lookup
    std::vector<std::string> watches_;
    DebugCallback callback_;
    
    uint32_t nextBreakpointId_ = 1;
    StepMode stepMode_ = StepMode::None;
    bool paused_ = false;
    size_t stepStartDepth_ = 0;
};

} // namespace Zepra::Debug
