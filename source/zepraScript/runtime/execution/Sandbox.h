/**
 * @file Sandbox.h
 * @brief Sandboxing and Security Infrastructure for ZepraScript
 * 
 * Provides execution limits, timeout mechanism, and secure isolation
 * for safe JavaScript execution in browser environments.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <chrono>
#include <functional>
#include <string>
#include <unordered_set>
#include <atomic>
#include <stdexcept>

namespace Zepra::Runtime {

// =============================================================================
// Execution Limits
// =============================================================================

/**
 * @brief Resource limits for script execution
 */
struct ExecutionLimits {
    // Memory limits
    size_t maxHeapBytes = 256 * 1024 * 1024;     // 256MB default
    size_t maxStackBytes = 8 * 1024 * 1024;       // 8MB stack
    size_t maxStringLength = 512 * 1024 * 1024;   // 512MB strings
    size_t maxArrayLength = 4294967295;           // 2^32 - 1 (JS max)
    
    // CPU limits
    uint64_t maxInstructions = 0;                 // 0 = unlimited
    std::chrono::milliseconds maxExecutionTime{0}; // 0 = unlimited
    
    // Call depth
    size_t maxCallStackDepth = 10000;             // Recursion limit
    size_t maxLoopIterations = 0;                 // 0 = unlimited
    
    // Object limits
    size_t maxObjectProperties = 100000;
    size_t maxRegexLength = 65536;
    size_t maxRegexBacktracks = 1000000;
    
    // Default limits for browser context
    static ExecutionLimits browser() {
        ExecutionLimits limits;
        limits.maxHeapBytes = 512 * 1024 * 1024;        // 512MB
        limits.maxExecutionTime = std::chrono::seconds(30);
        limits.maxInstructions = 1000000000;             // 1B instructions
        limits.maxCallStackDepth = 10000;
        return limits;
    }
    
    // Strict limits for untrusted scripts
    static ExecutionLimits untrusted() {
        ExecutionLimits limits;
        limits.maxHeapBytes = 64 * 1024 * 1024;         // 64MB
        limits.maxExecutionTime = std::chrono::seconds(5);
        limits.maxInstructions = 100000000;              // 100M instructions
        limits.maxCallStackDepth = 1000;
        limits.maxLoopIterations = 10000000;
        return limits;
    }
};

// =============================================================================
// Security Policy
// =============================================================================

/**
 * @brief Security policy for script execution
 */
struct SecurityPolicy {
    // Allowed/denied APIs
    std::unordered_set<std::string> allowedGlobals;
    std::unordered_set<std::string> deniedGlobals;
    
    // Feature flags
    bool allowEval = false;
    bool allowFetch = true;
    bool allowWorkers = false;
    bool allowWasm = true;
    bool allowDynamicImport = false;
    bool allowFileAccess = false;
    bool allowNetworkAccess = true;
    
    // Content Security Policy
    std::string scriptSrc = "'self'";
    std::string connectSrc = "'self'";
    std::string frameSrc = "'none'";
    
    // Origin restrictions
    std::unordered_set<std::string> allowedOrigins;
    bool crossOriginIsolated = false;
    
    // Default browser policy
    static SecurityPolicy browser() {
        SecurityPolicy policy;
        policy.allowEval = false;
        policy.allowFetch = true;
        policy.allowWorkers = true;
        policy.allowWasm = true;
        return policy;
    }
    
    // Strict sandbox policy
    static SecurityPolicy strict() {
        SecurityPolicy policy;
        policy.allowEval = false;
        policy.allowFetch = false;
        policy.allowWorkers = false;
        policy.allowWasm = false;
        policy.allowDynamicImport = false;
        policy.allowFileAccess = false;
        policy.allowNetworkAccess = false;
        return policy;
    }
};

// =============================================================================
// Resource Monitor
// =============================================================================

/**
 * @brief Tracks and enforces resource limits during execution
 */
class ResourceMonitor {
public:
    explicit ResourceMonitor(const ExecutionLimits& limits)
        : limits_(limits)
        , startTime_(std::chrono::steady_clock::now()) {}
    
    // Instruction counting
    void addInstructions(uint64_t count) {
        instructionCount_ += count;
    }
    
    bool checkInstructionLimit() const {
        return limits_.maxInstructions == 0 || 
               instructionCount_ < limits_.maxInstructions;
    }
    
    // Timeout checking
    bool checkTimeout() const {
        if (limits_.maxExecutionTime.count() == 0) return true;
        auto elapsed = std::chrono::steady_clock::now() - startTime_;
        return elapsed < limits_.maxExecutionTime;
    }
    
    // Memory tracking
    void addHeapAllocation(size_t bytes) {
        heapUsed_ += bytes;
    }
    
    void removeHeapAllocation(size_t bytes) {
        if (bytes <= heapUsed_) heapUsed_ -= bytes;
    }
    
    bool checkHeapLimit() const {
        return heapUsed_ < limits_.maxHeapBytes;
    }
    
    // Stack depth tracking
    void pushCall() { callDepth_++; }
    void popCall() { if (callDepth_ > 0) callDepth_--; }
    
    bool checkStackLimit() const {
        return callDepth_ < limits_.maxCallStackDepth;
    }
    
    // Combined check - call periodically in hot loops
    bool checkLimits() const {
        return checkInstructionLimit() && 
               checkTimeout() && 
               checkHeapLimit() && 
               checkStackLimit();
    }
    
    // Statistics
    uint64_t instructionCount() const { return instructionCount_; }
    size_t heapUsed() const { return heapUsed_; }
    size_t callDepth() const { return callDepth_; }
    
    auto elapsedTime() const {
        return std::chrono::steady_clock::now() - startTime_;
    }
    
    // Termination request
    void requestTermination() { terminationRequested_ = true; }
    bool isTerminationRequested() const { return terminationRequested_; }
    
private:
    ExecutionLimits limits_;
    std::chrono::steady_clock::time_point startTime_;
    
    std::atomic<uint64_t> instructionCount_{0};
    std::atomic<size_t> heapUsed_{0};
    std::atomic<size_t> callDepth_{0};
    std::atomic<bool> terminationRequested_{false};
};

// =============================================================================
// Sandbox Configuration
// =============================================================================

/**
 * @brief Complete sandbox configuration
 */
struct SandboxConfig {
    ExecutionLimits limits;
    SecurityPolicy policy;
    
    // Callbacks
    std::function<void()> onTimeout;
    std::function<void(const std::string&)> onSecurityViolation;
    std::function<void(size_t)> onMemoryLimitApproaching;
    
    static SandboxConfig browser() {
        return SandboxConfig{
            ExecutionLimits::browser(),
            SecurityPolicy::browser()
        };
    }
    
    static SandboxConfig strict() {
        return SandboxConfig{
            ExecutionLimits::untrusted(),
            SecurityPolicy::strict()
        };
    }
};

// =============================================================================
// Security Exception
// =============================================================================

/**
 * @brief Exception thrown on security violations
 */
class SecurityError : public std::runtime_error {
public:
    enum class Type {
        Timeout,
        MemoryLimit,
        StackOverflow,
        InstructionLimit,
        DeniedAPI,
        CSPViolation,
        EvalBlocked,
        NetworkBlocked
    };
    
    SecurityError(Type type, const std::string& message)
        : std::runtime_error(message)
        , type_(type) {}
    
    Type type() const { return type_; }
    
private:
    Type type_;
};

} // namespace Zepra::Runtime
