/**
 * @file WasmDebug.h
 * @brief WASM Debug and Trace Hooks
 * 
 * Implements:
 * - Bytecode-level tracing
 * - Memory access logging
 * - Call stack inspection
 * - Breakpoint infrastructure
 */

#pragma once

#include "wasm.hpp"
#include <vector>
#include <string>
#include <functional>
#include <unordered_set>
#include <cstdint>

namespace Zepra::Wasm {

// =============================================================================
// Debug Events
// =============================================================================

/**
 * @brief Types of debug events
 */
enum class DebugEventType {
    Instruction,      // Before instruction execution
    Call,             // Function call
    Return,           // Function return
    MemoryRead,       // Memory load
    MemoryWrite,      // Memory store
    TableAccess,      // Table get/set
    Exception,        // Exception thrown
    Trap,             // Trap triggered
    Breakpoint        // Breakpoint hit
};

/**
 * @brief Debug event data
 */
struct DebugEvent {
    DebugEventType type;
    uint32_t funcIndex = 0;
    uint32_t instructionOffset = 0;
    uint32_t memoryAddress = 0;
    uint32_t memorySize = 0;
    uint64_t value = 0;
    std::string message;
};

/**
 * @brief Call frame for stack inspection
 */
struct DebugFrame {
    uint32_t funcIndex;
    std::string funcName;
    uint32_t instructionOffset;
    std::vector<uint64_t> locals;
    std::vector<uint64_t> stack;
};

// =============================================================================
// Trace Hooks
// =============================================================================

/**
 * @brief Trace hook callback
 */
using TraceCallback = std::function<void(const DebugEvent&)>;

/**
 * @brief Instruction tracer
 */
class InstructionTracer {
public:
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    void setCallback(TraceCallback callback) { callback_ = std::move(callback); }
    
    void traceInstruction(uint32_t funcIndex, uint32_t offset, uint8_t opcode) {
        if (enabled_ && callback_) {
            DebugEvent event;
            event.type = DebugEventType::Instruction;
            event.funcIndex = funcIndex;
            event.instructionOffset = offset;
            event.value = opcode;
            callback_(event);
        }
    }
    
    void traceCall(uint32_t callerFunc, uint32_t calleeFunc) {
        if (enabled_ && callback_) {
            DebugEvent event;
            event.type = DebugEventType::Call;
            event.funcIndex = callerFunc;
            event.value = calleeFunc;
            callback_(event);
        }
    }
    
    void traceReturn(uint32_t funcIndex) {
        if (enabled_ && callback_) {
            DebugEvent event;
            event.type = DebugEventType::Return;
            event.funcIndex = funcIndex;
            callback_(event);
        }
    }
    
private:
    bool enabled_ = false;
    TraceCallback callback_;
};

/**
 * @brief Memory access tracer
 */
class MemoryTracer {
public:
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    void setCallback(TraceCallback callback) { callback_ = std::move(callback); }
    
    // Add a memory region to watch
    void watchRegion(uint32_t start, uint32_t end) {
        watchedRegions_.push_back({start, end});
    }
    
    void clearWatches() { watchedRegions_.clear(); }
    
    void traceRead(uint32_t address, uint32_t size, uint64_t value) {
        if (!enabled_) return;
        
        // Check if in watched region
        bool watched = watchedRegions_.empty();
        for (const auto& [start, end] : watchedRegions_) {
            if (address >= start && address < end) {
                watched = true;
                break;
            }
        }
        
        if (watched && callback_) {
            DebugEvent event;
            event.type = DebugEventType::MemoryRead;
            event.memoryAddress = address;
            event.memorySize = size;
            event.value = value;
            callback_(event);
        }
    }
    
    void traceWrite(uint32_t address, uint32_t size, uint64_t value) {
        if (!enabled_) return;
        
        bool watched = watchedRegions_.empty();
        for (const auto& [start, end] : watchedRegions_) {
            if (address >= start && address < end) {
                watched = true;
                break;
            }
        }
        
        if (watched && callback_) {
            DebugEvent event;
            event.type = DebugEventType::MemoryWrite;
            event.memoryAddress = address;
            event.memorySize = size;
            event.value = value;
            callback_(event);
        }
    }
    
private:
    bool enabled_ = false;
    TraceCallback callback_;
    std::vector<std::pair<uint32_t, uint32_t>> watchedRegions_;
};

// =============================================================================
// Breakpoints
// =============================================================================

/**
 * @brief Breakpoint descriptor
 */
struct Breakpoint {
    uint32_t id;
    uint32_t funcIndex;
    uint32_t instructionOffset;
    bool enabled = true;
    std::string condition;  // Optional conditional expression
};

/**
 * @brief Breakpoint callback
 */
using BreakpointCallback = std::function<bool(const Breakpoint&, const DebugFrame&)>;

/**
 * @brief Breakpoint manager
 */
class BreakpointManager {
public:
    // Add a breakpoint, returns ID
    uint32_t addBreakpoint(uint32_t funcIndex, uint32_t offset,
                          const std::string& condition = "") {
        uint32_t id = nextId_++;
        breakpoints_[id] = {id, funcIndex, offset, true, condition};
        locationIndex_[makeKey(funcIndex, offset)].insert(id);
        return id;
    }
    
    // Remove a breakpoint
    void removeBreakpoint(uint32_t id) {
        auto it = breakpoints_.find(id);
        if (it != breakpoints_.end()) {
            uint64_t key = makeKey(it->second.funcIndex, it->second.instructionOffset);
            locationIndex_[key].erase(id);
            breakpoints_.erase(it);
        }
    }
    
    // Enable/disable a breakpoint
    void setEnabled(uint32_t id, bool enabled) {
        auto it = breakpoints_.find(id);
        if (it != breakpoints_.end()) {
            it->second.enabled = enabled;
        }
    }
    
    // Check if there's a breakpoint at location
    bool hasBreakpoint(uint32_t funcIndex, uint32_t offset) const {
        uint64_t key = makeKey(funcIndex, offset);
        auto it = locationIndex_.find(key);
        if (it == locationIndex_.end()) return false;
        
        for (uint32_t id : it->second) {
            auto bpIt = breakpoints_.find(id);
            if (bpIt != breakpoints_.end() && bpIt->second.enabled) {
                return true;
            }
        }
        return false;
    }
    
    // Get breakpoints at location
    std::vector<const Breakpoint*> getBreakpoints(uint32_t funcIndex, uint32_t offset) const {
        std::vector<const Breakpoint*> result;
        uint64_t key = makeKey(funcIndex, offset);
        auto it = locationIndex_.find(key);
        if (it != locationIndex_.end()) {
            for (uint32_t id : it->second) {
                auto bpIt = breakpoints_.find(id);
                if (bpIt != breakpoints_.end() && bpIt->second.enabled) {
                    result.push_back(&bpIt->second);
                }
            }
        }
        return result;
    }
    
    // Set callback for breakpoint hits
    void setCallback(BreakpointCallback callback) { callback_ = std::move(callback); }
    
    // Called when a breakpoint is hit
    bool onBreakpoint(uint32_t funcIndex, uint32_t offset, const DebugFrame& frame) {
        auto bps = getBreakpoints(funcIndex, offset);
        for (const auto* bp : bps) {
            if (callback_ && !callback_(*bp, frame)) {
                return false;  // Pause execution
            }
        }
        return true;  // Continue execution
    }
    
    // Get all breakpoints
    std::vector<Breakpoint> getAllBreakpoints() const {
        std::vector<Breakpoint> result;
        for (const auto& [id, bp] : breakpoints_) {
            result.push_back(bp);
        }
        return result;
    }
    
private:
    static uint64_t makeKey(uint32_t funcIndex, uint32_t offset) {
        return (static_cast<uint64_t>(funcIndex) << 32) | offset;
    }
    
    std::unordered_map<uint32_t, Breakpoint> breakpoints_;
    std::unordered_map<uint64_t, std::unordered_set<uint32_t>> locationIndex_;
    BreakpointCallback callback_;
    uint32_t nextId_ = 1;
};

// =============================================================================
// Debug Manager
// =============================================================================

/**
 * @brief Centralized debug manager
 */
class DebugManager {
public:
    // Enable/disable all debugging
    void setEnabled(bool enabled) {
        instructionTracer_.setEnabled(enabled);
        memoryTracer_.setEnabled(enabled);
    }
    
    bool isEnabled() const { return instructionTracer_.isEnabled(); }
    
    // Tracers
    InstructionTracer& instructionTracer() { return instructionTracer_; }
    MemoryTracer& memoryTracer() { return memoryTracer_; }
    BreakpointManager& breakpoints() { return breakpoints_; }
    
    // Call stack
    void pushFrame(const DebugFrame& frame) { callStack_.push_back(frame); }
    void popFrame() { if (!callStack_.empty()) callStack_.pop_back(); }
    const std::vector<DebugFrame>& callStack() const { return callStack_; }
    
    // Current frame
    const DebugFrame* currentFrame() const {
        return callStack_.empty() ? nullptr : &callStack_.back();
    }
    
    // Step control
    enum class StepMode { None, Into, Over, Out };
    void setStepMode(StepMode mode) { stepMode_ = mode; }
    StepMode stepMode() const { return stepMode_; }
    
    // Pause/resume
    void pause() { paused_ = true; }
    void resume() { paused_ = false; }
    bool isPaused() const { return paused_; }
    
private:
    InstructionTracer instructionTracer_;
    MemoryTracer memoryTracer_;
    BreakpointManager breakpoints_;
    std::vector<DebugFrame> callStack_;
    StepMode stepMode_ = StepMode::None;
    bool paused_ = false;
};

} // namespace Zepra::Wasm
