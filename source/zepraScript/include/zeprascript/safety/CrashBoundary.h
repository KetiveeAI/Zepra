/**
 * @file CrashBoundary.h
 * @brief Crash Containment and Recovery
 * 
 * Implements SpiderMonkey/WebKit-inspired patterns:
 * - OOM → uncatchable exception (not process crash)
 * - WASM trap → engine exception (contained)
 * - Worker crash isolation
 * - JIT failure → interpreter fallback
 * - Engine reset and recovery paths
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <functional>
#include <exception>
#include <memory>
#include <atomic>
#include <signal.h>
#include <setjmp.h>

namespace Zepra::Safety {

// =============================================================================
// Uncatchable Exceptions (SpiderMonkey Pattern)
// =============================================================================

/**
 * @brief Exception that cannot be caught by JS try/catch
 * 
 * These propagate to the embedding layer, allowing graceful shutdown
 * without corrupting engine state. Used for:
 * - Out of memory
 * - Stack overflow
 * - Internal engine errors
 */
class UncatchableException : public std::exception {
public:
    enum class Kind {
        OutOfMemory,
        StackOverflow,
        InternalError,
        SecurityViolation,
        Terminated
    };
    
    explicit UncatchableException(Kind k, const std::string& msg = "")
        : kind_(k), message_(msg) {}
    
    Kind kind() const { return kind_; }
    const char* what() const noexcept override { return message_.c_str(); }
    
    static UncatchableException OOM() {
        return UncatchableException(Kind::OutOfMemory, "Out of memory");
    }
    
    static UncatchableException StackOverflow() {
        return UncatchableException(Kind::StackOverflow, "Stack overflow");
    }
    
    static UncatchableException Internal(const std::string& msg) {
        return UncatchableException(Kind::InternalError, msg);
    }
    
    // Check if exception should propagate (not caught by JS)
    bool isUncatchable() const { return true; }
    
private:
    Kind kind_;
    std::string message_;
};

// =============================================================================
// OOM Handler (SpiderMonkey Pattern)
// =============================================================================

/**
 * @brief OOM handling with graceful degradation
 * 
 * SpiderMonkey converts OOM to uncatchable exceptions, allowing embedders
 * to handle memory exhaustion without crashing.
 */
class OOMHandler {
public:
    using OOMCallback = std::function<void(size_t requestedBytes)>;
    
    static OOMHandler& instance() {
        static OOMHandler inst;
        return inst;
    }
    
    // Attempt allocation, throw UncatchableException on failure
    void* allocateOrThrow(size_t bytes) {
        void* ptr = tryAllocate(bytes);
        if (!ptr) {
            // Invoke callback for cleanup opportunity
            if (oomCallback_) {
                oomCallback_(bytes);
                
                // Retry once after callback (GC may have freed memory)
                ptr = tryAllocate(bytes);
                if (ptr) return ptr;
            }
            
            // Still failed - throw uncatchable exception
            stats_.oomCount++;
            throw UncatchableException::OOM();
        }
        return ptr;
    }
    
    // Check if we're in a low-memory state
    bool isLowMemory() const {
        return getAvailableMemory() < lowMemoryThreshold_;
    }
    
    // Set OOM callback (usually triggers GC)
    void setOOMCallback(OOMCallback cb) {
        oomCallback_ = std::move(cb);
    }
    
    // Set low memory threshold
    void setLowMemoryThreshold(size_t bytes) {
        lowMemoryThreshold_ = bytes;
    }
    
    struct Stats {
        size_t oomCount = 0;
        size_t recoveryCount = 0;
        size_t lastRequestedBytes = 0;
    };
    
    Stats stats() const { return stats_; }
    
private:
    OOMHandler() = default;
    
    void* tryAllocate(size_t bytes) {
        stats_.lastRequestedBytes = bytes;
        return std::malloc(bytes);
    }
    
    size_t getAvailableMemory() const {
        // Platform-specific: would check /proc/meminfo on Linux
        return 1024 * 1024 * 1024;  // Placeholder: 1GB
    }
    
    OOMCallback oomCallback_;
    size_t lowMemoryThreshold_ = 64 * 1024 * 1024;  // 64MB
    Stats stats_;
};

// =============================================================================
// WASM Trap Containment
// =============================================================================

/**
 * @brief Contains WASM traps within the engine
 * 
 * WASM traps (integer overflow, invalid memory access, etc.) must not
 * crash the browser. Convert to catchable exceptions at WASM/JS boundary.
 */
class WasmTrapHandler {
public:
    enum class TrapKind {
        None,
        Unreachable,
        IntegerOverflow,
        IntegerDivideByZero,
        OutOfBoundsMemory,
        OutOfBoundsTable,
        IndirectCallTypeMismatch,
        StackOverflow,
        UndefinedElement,
        UnalignedAccess
    };
    
    // Trap context saved for recovery
    struct TrapContext {
        TrapKind kind = TrapKind::None;
        uint32_t pc = 0;  // Program counter at trap
        uint32_t stackDepth = 0;
        std::string moduleName;
        std::string functionName;
    };
    
    static WasmTrapHandler& instance() {
        static WasmTrapHandler inst;
        return inst;
    }
    
    // Install signal handlers for WASM traps
    void installHandlers() {
#ifndef _WIN32
        struct sigaction sa;
        sa.sa_sigaction = signalHandler;
        sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
        sigemptyset(&sa.sa_mask);
        
        sigaction(SIGFPE, &sa, nullptr);   // Integer overflow, divide by zero
        sigaction(SIGSEGV, &sa, nullptr);  // Out of bounds
        sigaction(SIGBUS, &sa, nullptr);   // Unaligned access
#endif
        handlersInstalled_ = true;
    }
    
    // Set recovery point before WASM execution
    bool setRecoveryPoint() {
        return sigsetjmp(recoveryPoint_, 1) == 0;
    }
    
    // Trigger trap (called from WASM code or signal handler)
    [[noreturn]] void trap(TrapKind kind, const char* message = nullptr) {
        currentTrap_.kind = kind;
        stats_.trapCount++;
        
        // Jump to recovery point
        siglongjmp(recoveryPoint_, static_cast<int>(kind));
    }
    
    // Get last trap info
    const TrapContext& lastTrap() const { return currentTrap_; }
    
    // Convert trap to JS exception
    std::string trapToMessage(TrapKind kind) const {
        switch (kind) {
            case TrapKind::Unreachable: return "unreachable executed";
            case TrapKind::IntegerOverflow: return "integer overflow";
            case TrapKind::IntegerDivideByZero: return "integer divide by zero";
            case TrapKind::OutOfBoundsMemory: return "out of bounds memory access";
            case TrapKind::OutOfBoundsTable: return "out of bounds table access";
            case TrapKind::IndirectCallTypeMismatch: return "indirect call type mismatch";
            case TrapKind::StackOverflow: return "call stack exhausted";
            default: return "wasm trap";
        }
    }
    
    struct Stats {
        size_t trapCount = 0;
        size_t recoveredCount = 0;
    };
    
    Stats stats() const { return stats_; }
    
private:
    WasmTrapHandler() = default;
    
    static void signalHandler(int sig, siginfo_t* info, void* context) {
        (void)info; (void)context;
        
        TrapKind kind = TrapKind::None;
        switch (sig) {
            case SIGFPE: kind = TrapKind::IntegerDivideByZero; break;
            case SIGSEGV: kind = TrapKind::OutOfBoundsMemory; break;
            case SIGBUS: kind = TrapKind::UnalignedAccess; break;
        }
        
        instance().trap(kind);
    }
    
    sigjmp_buf recoveryPoint_;
    TrapContext currentTrap_;
    Stats stats_;
    bool handlersInstalled_ = false;
};

// =============================================================================
// Stack Overflow Protection
// =============================================================================

/**
 * @brief Stack depth monitoring and overflow prevention
 */
class StackGuard {
public:
    explicit StackGuard(size_t maxDepth = 1024) : maxDepth_(maxDepth) {}
    
    // Check before function call
    bool canPush() const {
        return currentDepth_ < maxDepth_;
    }
    
    // Push frame (throws if overflow)
    void push() {
        if (!canPush()) {
            throw UncatchableException::StackOverflow();
        }
        currentDepth_++;
        if (currentDepth_ > peakDepth_) peakDepth_ = currentDepth_;
    }
    
    // Pop frame
    void pop() {
        if (currentDepth_ > 0) currentDepth_--;
    }
    
    size_t depth() const { return currentDepth_; }
    size_t peak() const { return peakDepth_; }
    size_t remaining() const { return maxDepth_ - currentDepth_; }
    
    // RAII guard
    class Frame {
    public:
        explicit Frame(StackGuard& guard) : guard_(guard) { guard_.push(); }
        ~Frame() { guard_.pop(); }
        Frame(const Frame&) = delete;
        Frame& operator=(const Frame&) = delete;
    private:
        StackGuard& guard_;
    };
    
private:
    size_t maxDepth_;
    size_t currentDepth_ = 0;
    size_t peakDepth_ = 0;
};

// =============================================================================
// JIT Failure Fallback
// =============================================================================

/**
 * @brief Fallback to interpreter when JIT fails
 * 
 * WebKit pattern: If JIT compilation fails (memory, internal error),
 * fall back to baseline interpreter. Never crash.
 */
class JITFallback {
public:
    enum class Tier {
        Interpreter,
        Baseline,
        Optimizing
    };
    
    static JITFallback& instance() {
        static JITFallback inst;
        return inst;
    }
    
    // Record JIT failure
    void recordFailure(Tier tier, const std::string& reason) {
        failures_.push_back({tier, reason});
        
        // Disable higher tiers after too many failures
        if (failures_.size() > failureThreshold_) {
            if (tier == Tier::Optimizing) {
                optimizingEnabled_ = false;
            } else if (tier == Tier::Baseline) {
                baselineEnabled_ = false;
            }
        }
    }
    
    // Get best available tier
    Tier bestAvailableTier() const {
        if (optimizingEnabled_) return Tier::Optimizing;
        if (baselineEnabled_) return Tier::Baseline;
        return Tier::Interpreter;
    }
    
    // Check if tier is available
    bool isTierAvailable(Tier tier) const {
        switch (tier) {
            case Tier::Optimizing: return optimizingEnabled_;
            case Tier::Baseline: return baselineEnabled_;
            case Tier::Interpreter: return true;  // Always available
        }
        return false;
    }
    
    // Reset (for testing or recovery)
    void reset() {
        failures_.clear();
        optimizingEnabled_ = true;
        baselineEnabled_ = true;
    }
    
    void setFailureThreshold(size_t n) { failureThreshold_ = n; }
    
private:
    JITFallback() = default;
    
    struct Failure {
        Tier tier;
        std::string reason;
    };
    std::vector<Failure> failures_;
    bool optimizingEnabled_ = true;
    bool baselineEnabled_ = true;
    size_t failureThreshold_ = 3;
};

// =============================================================================
// Engine Reset and Recovery
// =============================================================================

/**
 * @brief Engine reset without process restart
 * 
 * When engine state is corrupted, reset to clean state instead of crashing.
 */
class EngineRecovery {
public:
    using ResetCallback = std::function<void()>;
    
    static EngineRecovery& instance() {
        static EngineRecovery inst;
        return inst;
    }
    
    // Register reset handler
    void registerResetHandler(const std::string& name, ResetCallback cb) {
        handlers_[name] = std::move(cb);
    }
    
    // Trigger full reset
    void resetAll() {
        resetCount_++;
        for (auto& [name, handler] : handlers_) {
            try {
                handler();
            } catch (...) {
                // Handler failed, continue with others
            }
        }
    }
    
    // Trigger partial reset
    void reset(const std::string& subsystem) {
        auto it = handlers_.find(subsystem);
        if (it != handlers_.end()) {
            try {
                it->second();
            } catch (...) {
                // Log failure
            }
        }
    }
    
    size_t resetCount() const { return resetCount_; }
    
private:
    EngineRecovery() = default;
    
    std::unordered_map<std::string, ResetCallback> handlers_;
    size_t resetCount_ = 0;
};

// =============================================================================
// Worker Crash Isolation
// =============================================================================

/**
 * @brief Isolates worker crashes from main thread
 */
class WorkerIsolation {
public:
    struct WorkerState {
        uint64_t id;
        std::atomic<bool> alive{true};
        std::atomic<bool> crashed{false};
        std::string crashReason;
    };
    
    // Register worker
    uint64_t registerWorker() {
        uint64_t id = nextId_++;
        workers_[id] = std::make_unique<WorkerState>();
        workers_[id]->id = id;
        return id;
    }
    
    // Mark worker as crashed
    void workerCrashed(uint64_t id, const std::string& reason) {
        auto it = workers_.find(id);
        if (it != workers_.end()) {
            it->second->alive = false;
            it->second->crashed = true;
            it->second->crashReason = reason;
        }
        crashCount_++;
    }
    
    // Check if worker is alive
    bool isWorkerAlive(uint64_t id) const {
        auto it = workers_.find(id);
        return it != workers_.end() && it->second->alive;
    }
    
    // Cleanup worker
    void removeWorker(uint64_t id) {
        workers_.erase(id);
    }
    
    // Stats
    size_t activeWorkers() const {
        size_t count = 0;
        for (const auto& [id, w] : workers_) {
            if (w->alive) count++;
        }
        return count;
    }
    
    size_t crashCount() const { return crashCount_; }
    
private:
    std::unordered_map<uint64_t, std::unique_ptr<WorkerState>> workers_;
    std::atomic<uint64_t> nextId_{1};
    std::atomic<size_t> crashCount_{0};
};

// =============================================================================
// Crash Boundary (Main Entry Point)
// =============================================================================

/**
 * @brief Safe execution boundary
 * 
 * Wraps code execution with proper crash containment.
 */
class CrashBoundary {
public:
    enum class Result {
        Success,
        JSException,        // Catchable JS exception
        OOM,                // Out of memory
        StackOverflow,      // Stack exhausted
        WasmTrap,           // WASM trap
        InternalError,      // Engine bug
        Timeout             // Execution timeout
    };
    
    struct ExecutionResult {
        Result result = Result::Success;
        std::string errorMessage;
        bool recoverable = true;
    };
    
    // Execute with crash containment
    template<typename F>
    static ExecutionResult execute(F&& func) {
        ExecutionResult res;
        
        // Set up WASM trap recovery
        if (!WasmTrapHandler::instance().setRecoveryPoint()) {
            // Returned from trap
            auto trap = WasmTrapHandler::instance().lastTrap();
            res.result = Result::WasmTrap;
            res.errorMessage = WasmTrapHandler::instance().trapToMessage(trap.kind);
            return res;
        }
        
        try {
            func();
            res.result = Result::Success;
        } catch (const UncatchableException& e) {
            switch (e.kind()) {
                case UncatchableException::Kind::OutOfMemory:
                    res.result = Result::OOM;
                    break;
                case UncatchableException::Kind::StackOverflow:
                    res.result = Result::StackOverflow;
                    break;
                default:
                    res.result = Result::InternalError;
            }
            res.errorMessage = e.what();
            res.recoverable = (e.kind() != UncatchableException::Kind::SecurityViolation);
        } catch (const std::exception& e) {
            res.result = Result::JSException;
            res.errorMessage = e.what();
        } catch (...) {
            res.result = Result::InternalError;
            res.errorMessage = "Unknown exception";
            res.recoverable = false;
        }
        
        return res;
    }
    
    // Execute with timeout
    template<typename F>
    static ExecutionResult executeWithTimeout(F&& func, uint32_t timeoutMs) {
        // Would use platform-specific timeout mechanism
        (void)timeoutMs;
        return execute(std::forward<F>(func));
    }
};

} // namespace Zepra::Safety
