/**
 * @file StepDebugger.h
 * @brief Step-by-step execution control
 * 
 * Implements:
 * - Step into/over/out
 * - Async step
 * - Blackbox patterns
 * - Pause on exceptions
 * 
 * Based on V8 Debug and CDP stepping
 */

#pragma once

#include "BreakpointManager.h"
#include "runtime/execution/StackFrame.h"
#include <functional>
#include <regex>

namespace Zepra::Debug {

// =============================================================================
// Step Action
// =============================================================================

enum class StepAction : uint8_t {
    Continue,       // Run until next breakpoint
    StepInto,       // Step into function calls
    StepOver,       // Step over function calls
    StepOut,        // Step out of current function
    Pause           // Pause execution
};

// =============================================================================
// Debugger State
// =============================================================================

enum class DebuggerState : uint8_t {
    Running,
    Paused,
    Stepping,
    Disabled
};

// =============================================================================
// Pause Reason
// =============================================================================

enum class PauseReason : uint8_t {
    Breakpoint,
    Exception,
    DebuggerStatement,
    Step,
    Pause,
    Other
};

struct PauseInfo {
    PauseReason reason;
    SourceLocation location;
    std::optional<uint32_t> breakpointId;
    std::string description;
    
    // Exception info (if reason == Exception)
    bool isExceptionUncaught = false;
    std::string exceptionMessage;
};

// =============================================================================
// Call Frame Info (for debugger)
// =============================================================================

struct DebugCallFrame {
    std::string callFrameId;
    std::string functionName;
    SourceLocation location;
    std::vector<std::pair<std::string, std::string>> scopeChain;
    std::string thisObjectId;
    std::optional<std::string> returnValue;
};

// =============================================================================
// Scope
// =============================================================================

enum class ScopeType : uint8_t {
    Global,
    Local,
    With,
    Closure,
    Catch,
    Block,
    Script,
    Eval,
    Module,
    WasmExpressionStack
};

struct Scope {
    ScopeType type;
    std::string objectId;
    std::optional<std::string> name;
    SourceLocation startLocation;
    SourceLocation endLocation;
};

// =============================================================================
// Step Debugger
// =============================================================================

class StepDebugger {
public:
    using PauseCallback = std::function<void(const PauseInfo&, const std::vector<DebugCallFrame>&)>;
    using ResumeCallback = std::function<void()>;
    
    StepDebugger();
    
    // =========================================================================
    // State
    // =========================================================================
    
    DebuggerState state() const { return state_; }
    bool isPaused() const { return state_ == DebuggerState::Paused; }
    bool isEnabled() const { return state_ != DebuggerState::Disabled; }
    
    void enable();
    void disable();
    
    // =========================================================================
    // Execution Control
    // =========================================================================
    
    void pause();
    void resume();
    void stepInto();
    void stepOver();
    void stepOut();
    
    /**
     * @brief Step into async operations
     */
    void stepIntoAsync();
    
    /**
     * @brief Continue to specific location
     */
    void continueToLocation(const SourceLocation& location);
    
    // =========================================================================
    // Exception Handling
    // =========================================================================
    
    void setPauseOnExceptions(bool caught, bool uncaught);
    bool pauseOnCaughtExceptions() const { return pauseOnCaught_; }
    bool pauseOnUncaughtExceptions() const { return pauseOnUncaught_; }
    
    // =========================================================================
    // Blackboxing
    // =========================================================================
    
    /**
     * @brief Add pattern for scripts to skip during stepping
     */
    void addBlackboxPattern(const std::string& pattern);
    void removeBlackboxPattern(const std::string& pattern);
    void clearBlackboxPatterns();
    
    /**
     * @brief Check if location should be skipped
     */
    bool isBlackboxed(const std::string& scriptUrl) const;
    
    // =========================================================================
    // Callbacks
    // =========================================================================
    
    void setPauseCallback(PauseCallback callback) { pauseCallback_ = std::move(callback); }
    void setResumeCallback(ResumeCallback callback) { resumeCallback_ = std::move(callback); }
    
    // =========================================================================
    // VM Integration
    // =========================================================================
    
    /**
     * @brief Called before each statement (statement hook)
     * @return true if execution should continue
     */
    bool onStatement(const SourceLocation& location);
    
    /**
     * @brief Called on function entry
     */
    void onFunctionEnter(const std::string& functionName, const SourceLocation& location);
    
    /**
     * @brief Called on function exit
     */
    void onFunctionExit();
    
    /**
     * @brief Called on exception
     */
    void onException(bool caught, const std::string& message);
    
    /**
     * @brief Called on debugger statement
     */
    void onDebuggerStatement(const SourceLocation& location);
    
    // =========================================================================
    // Frame Inspection
    // =========================================================================
    
    std::vector<DebugCallFrame> getCallFrames() const;
    std::vector<Scope> getScopes(const std::string& callFrameId) const;
    
    /**
     * @brief Evaluate expression in frame context
     */
    struct EvalResult {
        bool success;
        std::string result;
        std::string type;
        std::optional<std::string> exceptionDetails;
    };
    
    EvalResult evaluateOnCallFrame(const std::string& callFrameId,
                                    const std::string& expression);
    
private:
    DebuggerState state_ = DebuggerState::Disabled;
    StepAction currentAction_ = StepAction::Continue;
    
    // Step tracking
    size_t stepStartDepth_ = 0;
    SourceLocation stepStartLocation_;
    
    // Exception settings
    bool pauseOnCaught_ = false;
    bool pauseOnUncaught_ = true;
    
    // Blackbox patterns
    std::vector<std::regex> blackboxPatterns_;
    
    // Callbacks
    PauseCallback pauseCallback_;
    ResumeCallback resumeCallback_;
    
    // Frame tracking
    std::vector<DebugCallFrame> callFrames_;
    
    void doPause(const PauseInfo& info);
    bool shouldPauseAt(const SourceLocation& location);
};

} // namespace Zepra::Debug
