/**
 * @file execution_control.cpp
 * @brief Execution control for debugging (stepping, pausing)
 * 
 * Implements step execution state machine with:
 * - Step into/over/out operations
 * - Async-aware stepping (step over await)
 * - Exception catching control
 * - Execution timeout/watchdog
 */

#include "zeprascript/debug/debugger.hpp"
#include "zeprascript/config.hpp"
#include <chrono>
#include <atomic>
#include <functional>
#include <optional>

namespace Zepra::Debug {

// =============================================================================
// ExecutionState - Current state of execution control
// =============================================================================

enum class ExecutionState {
    Running,        // Normal execution
    Paused,         // Execution paused (breakpoint, step, pause request)
    Stepping,       // Executing a step operation
    Terminated      // Execution has ended
};

// =============================================================================
// StepOperation - Details of current step operation
// =============================================================================

struct StepOperation {
    StepMode mode;              // Step into/over/out
    size_t startFrameDepth;     // Frame depth when step started
    std::string startFile;      // File when step started
    uint32_t startLine;         // Line when step started
    bool asyncAware;            // Handle async functions specially
    
    StepOperation()
        : mode(StepMode::None)
        , startFrameDepth(0)
        , startLine(0)
        , asyncAware(true) {}
        
    void reset() {
        mode = StepMode::None;
        startFrameDepth = 0;
        startFile.clear();
        startLine = 0;
    }
    
    bool isActive() const {
        return mode != StepMode::None && mode != StepMode::Continue;
    }
};

// =============================================================================
// ExceptionConfig - Exception catching configuration
// =============================================================================

struct ExceptionConfig {
    bool pauseOnCaughtExceptions = false;
    bool pauseOnUncaughtExceptions = true;
    std::vector<std::string> skipPatterns;  // Exception message patterns to skip
    
    bool shouldPauseOn(bool isCaught, const std::string& message) const {
        // Check if should pause based on caught/uncaught
        if (isCaught && !pauseOnCaughtExceptions) return false;
        if (!isCaught && !pauseOnUncaughtExceptions) return false;
        
        // Check skip patterns
        for (const auto& pattern : skipPatterns) {
            if (message.find(pattern) != std::string::npos) {
                return false;
            }
        }
        
        return true;
    }
};

// =============================================================================
// WatchdogConfig - Execution timeout configuration
// =============================================================================

struct WatchdogConfig {
    bool enabled = false;
    std::chrono::milliseconds timeout{30000};  // 30 second default
    std::chrono::milliseconds checkInterval{100};  // Check every 100ms
};

// =============================================================================
// ExecutionController - Main execution control class
// =============================================================================

class ExecutionController {
public:
    using PauseCallback = std::function<void(const std::string& reason)>;
    using ResumeCallback = std::function<void()>;
    using ExceptionCallback = std::function<void(const std::string& message, bool isCaught)>;
    
    ExecutionController()
        : state_(ExecutionState::Running)
        , pauseRequested_(false) {}
    
    // -------------------------------------------------------------------------
    // Execution State
    // -------------------------------------------------------------------------
    
    /**
     * @brief Get current execution state
     */
    ExecutionState state() const { return state_; }
    
    /**
     * @brief Check if execution is paused
     */
    bool isPaused() const { return state_ == ExecutionState::Paused; }
    
    /**
     * @brief Check if currently stepping
     */
    bool isStepping() const { return state_ == ExecutionState::Stepping; }
    
    // -------------------------------------------------------------------------
    // Pause/Resume Control
    // -------------------------------------------------------------------------
    
    /**
     * @brief Request pause at next opportunity
     */
    void requestPause() {
        pauseRequested_ = true;
    }
    
    /**
     * @brief Immediately pause execution
     */
    void pause(const std::string& reason = "Paused") {
        state_ = ExecutionState::Paused;
        pauseReason_ = reason;
        currentStep_.reset();
        
        if (pauseCallback_) {
            pauseCallback_(reason);
        }
    }
    
    /**
     * @brief Resume execution
     */
    void resume() {
        if (state_ != ExecutionState::Terminated) {
            state_ = ExecutionState::Running;
            pauseRequested_ = false;
            currentStep_.reset();
            
            if (resumeCallback_) {
                resumeCallback_();
            }
        }
    }
    
    /**
     * @brief Terminate execution
     */
    void terminate() {
        state_ = ExecutionState::Terminated;
        currentStep_.reset();
    }
    
    // -------------------------------------------------------------------------
    // Stepping Operations
    // -------------------------------------------------------------------------
    
    /**
     * @brief Start step into operation
     */
    void stepInto(size_t currentDepth, const std::string& file, uint32_t line) {
        currentStep_.mode = StepMode::StepInto;
        currentStep_.startFrameDepth = currentDepth;
        currentStep_.startFile = file;
        currentStep_.startLine = line;
        state_ = ExecutionState::Stepping;
        pauseRequested_ = false;
    }
    
    /**
     * @brief Start step over operation
     */
    void stepOver(size_t currentDepth, const std::string& file, uint32_t line) {
        currentStep_.mode = StepMode::StepOver;
        currentStep_.startFrameDepth = currentDepth;
        currentStep_.startFile = file;
        currentStep_.startLine = line;
        state_ = ExecutionState::Stepping;
        pauseRequested_ = false;
    }
    
    /**
     * @brief Start step out operation
     */
    void stepOut(size_t currentDepth, const std::string& file, uint32_t line) {
        currentStep_.mode = StepMode::StepOut;
        currentStep_.startFrameDepth = currentDepth;
        currentStep_.startFile = file;
        currentStep_.startLine = line;
        state_ = ExecutionState::Stepping;
        pauseRequested_ = false;
    }
    
    /**
     * @brief Continue to next breakpoint
     */
    void continueExecution() {
        currentStep_.mode = StepMode::Continue;
        state_ = ExecutionState::Running;
        pauseRequested_ = false;
    }
    
    /**
     * @brief Get current step operation
     */
    const StepOperation& currentStep() const { return currentStep_; }
    
    // -------------------------------------------------------------------------
    // Step Completion Check
    // -------------------------------------------------------------------------
    
    /**
     * @brief Result of checking step completion
     */
    struct StepCheckResult {
        bool shouldPause;
        std::string reason;
    };
    
    /**
     * @brief Check if step operation is complete
     */
    StepCheckResult checkStepComplete(size_t currentDepth, 
                                       const std::string& file, 
                                       uint32_t line) {
        StepCheckResult result{false, ""};
        
        // Check for pause request first
        if (pauseRequested_) {
            pauseRequested_ = false;
            result.shouldPause = true;
            result.reason = "Pause requested";
            return result;
        }
        
        if (!currentStep_.isActive()) {
            return result;
        }
        
        switch (currentStep_.mode) {
            case StepMode::StepInto:
                // Pause on any line change (even in different file)
                if (file != currentStep_.startFile || 
                    line != currentStep_.startLine) {
                    result.shouldPause = true;
                    result.reason = "Step into complete";
                }
                break;
                
            case StepMode::StepOver:
                // Pause when at same or lower depth and different line
                if (currentDepth <= currentStep_.startFrameDepth &&
                    (file != currentStep_.startFile || 
                     line != currentStep_.startLine)) {
                    result.shouldPause = true;
                    result.reason = "Step over complete";
                }
                break;
                
            case StepMode::StepOut:
                // Pause when we've exited the starting frame
                if (currentDepth < currentStep_.startFrameDepth) {
                    result.shouldPause = true;
                    result.reason = "Step out complete";
                }
                break;
                
            default:
                break;
        }
        
        if (result.shouldPause) {
            pause(result.reason);
        }
        
        return result;
    }
    
    // -------------------------------------------------------------------------
    // Async-Aware Stepping
    // -------------------------------------------------------------------------
    
    /**
     * @brief Notify entering async function
     */
    void onAsyncFunctionEnter(size_t depth) {
        if (!currentStep_.asyncAware) return;
        
        // If stepping over and we enter async, we need to track it
        if (currentStep_.mode == StepMode::StepOver) {
            asyncInfo_.isInAsync = true;
            asyncInfo_.asyncStartDepth = depth;
        }
    }
    
    /**
     * @brief Notify await expression
     */
    void onAwait(size_t depth) {
        if (!currentStep_.asyncAware || !asyncInfo_.isInAsync) return;
        
        // If stepping over await, we need to pause when it resumes
        if (currentStep_.mode == StepMode::StepOver && 
            depth <= asyncInfo_.asyncStartDepth) {
            asyncInfo_.awaitPending = true;
        }
    }
    
    /**
     * @brief Notify async resume after await
     */
    void onAsyncResume(size_t depth, const std::string& file, uint32_t line) {
        if (asyncInfo_.awaitPending && currentStep_.mode == StepMode::StepOver) {
            asyncInfo_.awaitPending = false;
            asyncInfo_.isInAsync = false;
            
            // Update step to pause at this location
            currentStep_.startFrameDepth = depth;
            currentStep_.startFile = file;
            currentStep_.startLine = line;
        }
    }
    
    // -------------------------------------------------------------------------
    // Exception Handling
    // -------------------------------------------------------------------------
    
    /**
     * @brief Configure exception pause behavior
     */
    void setExceptionConfig(const ExceptionConfig& config) {
        exceptionConfig_ = config;
    }
    
    /**
     * @brief Handle exception throw
     * @return true if execution should pause
     */
    bool onException(const std::string& message, bool isCaught) {
        if (exceptionConfig_.shouldPauseOn(isCaught, message)) {
            pause("Exception: " + message);
            
            if (exceptionCallback_) {
                exceptionCallback_(message, isCaught);
            }
            
            return true;
        }
        return false;
    }
    
    // -------------------------------------------------------------------------
    // Watchdog/Timeout
    // -------------------------------------------------------------------------
    
    /**
     * @brief Configure execution timeout
     */
    void configureWatchdog(const WatchdogConfig& config) {
        watchdogConfig_ = config;
    }
    
    /**
     * @brief Reset watchdog timer (called periodically during execution)
     */
    void resetWatchdog() {
        watchdogStart_ = std::chrono::steady_clock::now();
    }
    
    /**
     * @brief Check if execution has timed out
     */
    bool checkWatchdog() {
        if (!watchdogConfig_.enabled) return false;
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - watchdogStart_);
        
        if (elapsed >= watchdogConfig_.timeout) {
            pause("Execution timeout");
            return true;
        }
        
        return false;
    }
    
    // -------------------------------------------------------------------------
    // Callbacks
    // -------------------------------------------------------------------------
    
    void setPauseCallback(PauseCallback cb) { pauseCallback_ = std::move(cb); }
    void setResumeCallback(ResumeCallback cb) { resumeCallback_ = std::move(cb); }
    void setExceptionCallback(ExceptionCallback cb) { exceptionCallback_ = std::move(cb); }
    
    /**
     * @brief Get pause reason
     */
    const std::string& pauseReason() const { return pauseReason_; }
    
private:
    ExecutionState state_;
    std::atomic<bool> pauseRequested_;
    std::string pauseReason_;
    
    StepOperation currentStep_;
    
    // Async stepping state
    struct AsyncInfo {
        bool isInAsync = false;
        size_t asyncStartDepth = 0;
        bool awaitPending = false;
    } asyncInfo_;
    
    ExceptionConfig exceptionConfig_;
    WatchdogConfig watchdogConfig_;
    std::chrono::steady_clock::time_point watchdogStart_;
    
    PauseCallback pauseCallback_;
    ResumeCallback resumeCallback_;
    ExceptionCallback exceptionCallback_;
};

// =============================================================================
// Global ExecutionController Instance
// =============================================================================

static ExecutionController* globalExecutionController = nullptr;

ExecutionController* getExecutionController() {
    if (!globalExecutionController) {
        globalExecutionController = new ExecutionController();
    }
    return globalExecutionController;
}

void shutdownExecutionController() {
    delete globalExecutionController;
    globalExecutionController = nullptr;
}

} // namespace Zepra::Debug
