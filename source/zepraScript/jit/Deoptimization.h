/**
 * @file Deoptimization.h
 * @brief Deoptimization and bailout from JIT to interpreter
 * 
 * Implements:
 * - Deopt reasons and triggers
 * - Frame reconstruction
 * - OSR exit handling
 * - Deopt metadata
 * 
 * Based on V8 Deoptimizer and JSC OSR exit
 */

#pragma once

#include "MacroAssembler.h"
#include <vector>
#include <string>
#include <functional>

namespace Zepra::JIT {

// =============================================================================
// Deoptimization Reasons
// =============================================================================

enum class DeoptReason : uint8_t {
    // Type mismatches
    TypeMismatch,
    NotSmi,
    NotNumber,
    NotString,
    NotObject,
    NotArray,
    NotFunction,
    WrongMap,           // Hidden class mismatch
    
    // Arithmetic
    Overflow,
    DivisionByZero,
    NegativeZero,
    
    // Array bounds
    OutOfBounds,
    HoleCheck,
    
    // Object operations
    MissingProperty,
    WrongCallTarget,
    ReceiverNotObject,
    
    // Control flow
    UnexpectedValue,
    UnhandledException,
    
    // Debugging
    Debugger,
    
    // Explicit
    ForceDeopt,
    
    // Unknown
    Unknown
};

inline const char* deoptReasonName(DeoptReason reason) {
    switch (reason) {
        case DeoptReason::TypeMismatch: return "TypeMismatch";
        case DeoptReason::NotSmi: return "NotSmi";
        case DeoptReason::NotNumber: return "NotNumber";
        case DeoptReason::NotString: return "NotString";
        case DeoptReason::NotObject: return "NotObject";
        case DeoptReason::NotArray: return "NotArray";
        case DeoptReason::NotFunction: return "NotFunction";
        case DeoptReason::WrongMap: return "WrongMap";
        case DeoptReason::Overflow: return "Overflow";
        case DeoptReason::DivisionByZero: return "DivisionByZero";
        case DeoptReason::NegativeZero: return "NegativeZero";
        case DeoptReason::OutOfBounds: return "OutOfBounds";
        case DeoptReason::HoleCheck: return "HoleCheck";
        case DeoptReason::MissingProperty: return "MissingProperty";
        case DeoptReason::WrongCallTarget: return "WrongCallTarget";
        case DeoptReason::ReceiverNotObject: return "ReceiverNotObject";
        case DeoptReason::UnexpectedValue: return "UnexpectedValue";
        case DeoptReason::UnhandledException: return "UnhandledException";
        case DeoptReason::Debugger: return "Debugger";
        case DeoptReason::ForceDeopt: return "ForceDeopt";
        default: return "Unknown";
    }
}

// =============================================================================
// Deopt Location
// =============================================================================

/**
 * @brief Location of value during deoptimization
 */
enum class ValueLocation : uint8_t {
    Register,
    Stack,
    Constant,
    Uninitialized
};

struct DeoptValue {
    ValueLocation location;
    union {
        Register reg;
        int32_t stackOffset;
        int64_t constant;
    };
    
    static DeoptValue inRegister(Register r) {
        DeoptValue v;
        v.location = ValueLocation::Register;
        v.reg = r;
        return v;
    }
    
    static DeoptValue onStack(int32_t offset) {
        DeoptValue v;
        v.location = ValueLocation::Stack;
        v.stackOffset = offset;
        return v;
    }
    
    static DeoptValue asConstant(int64_t value) {
        DeoptValue v;
        v.location = ValueLocation::Constant;
        v.constant = value;
        return v;
    }
    
    static DeoptValue uninitialized() {
        DeoptValue v;
        v.location = ValueLocation::Uninitialized;
        return v;
    }
};

// =============================================================================
// Deopt Exit Point
// =============================================================================

/**
 * @brief Information about a single deoptimization point
 */
struct DeoptExit {
    size_t codeOffset;          // Offset in JIT code
    uint32_t bytecodeOffset;    // Target bytecode position
    DeoptReason reason;
    
    // Frame state at this point
    std::vector<DeoptValue> locals;
    std::vector<DeoptValue> stack;
    
    // Source location for debugging
    uint32_t lineNumber = 0;
    uint32_t columnNumber = 0;
};

// =============================================================================
// Deopt Info (metadata for JIT code)
// =============================================================================

/**
 * @brief Deoptimization metadata for a compiled function
 */
class DeoptInfo {
public:
    void addExit(DeoptExit exit) {
        exits_.push_back(std::move(exit));
    }
    
    const DeoptExit* findExit(size_t codeOffset) const {
        for (const auto& exit : exits_) {
            if (exit.codeOffset == codeOffset) {
                return &exit;
            }
        }
        return nullptr;
    }
    
    const std::vector<DeoptExit>& exits() const { return exits_; }
    
private:
    std::vector<DeoptExit> exits_;
};

// =============================================================================
// Deoptimizer
// =============================================================================

/**
 * @brief Handles deoptimization from JIT to interpreter
 */
class Deoptimizer {
public:
    struct Frame {
        void* jitFrame;         // JIT stack frame
        void* bytecodeFrame;    // Reconstructed interpreter frame
        uint32_t bytecodeOffset;
    };
    
    /**
     * @brief Perform deoptimization
     * @param jitFrame Current JIT frame pointer
     * @param deoptInfo Metadata for the function
     * @param codeOffset Offset where deopt occurred
     * @return Target address in interpreter
     */
    static void* deoptimize(void* jitFrame, 
                            const DeoptInfo& deoptInfo,
                            size_t codeOffset);
    
    /**
     * @brief Reconstruct interpreter frame from JIT frame
     */
    static Frame reconstructFrame(void* jitFrame, 
                                  const DeoptExit& exit);
    
    /**
     * @brief Read value from JIT frame
     */
    static int64_t readValue(void* frame, const DeoptValue& value);
    
    /**
     * @brief Statistics
     */
    struct Stats {
        size_t deoptCount = 0;
        size_t reasonCounts[static_cast<size_t>(DeoptReason::Unknown) + 1] = {0};
    };
    
    static Stats& stats() { return stats_; }
    
private:
    static Stats stats_;
};

// =============================================================================
// Lazy Deopt
// =============================================================================

/**
 * @brief Lazy deoptimization marker
 * 
 * Used to invalidate JIT code without immediately deoptimizing.
 * Deopt happens when code is next entered.
 */
class LazyDeopt {
public:
    /**
     * @brief Mark function for lazy deoptimization
     */
    static void markForDeopt(void* codeEntry, DeoptReason reason);
    
    /**
     * @brief Check if code is marked for deopt
     */
    static bool isMarked(void* codeEntry);
    
    /**
     * @brief Install deopt trampoline at code entry
     */
    static void installTrampoline(void* codeEntry, void* deoptTarget);
};

// =============================================================================
// Deopt Trigger (for code generation)
// =============================================================================

/**
 * @brief Emits deoptimization triggers during JIT compilation
 */
class DeoptTrigger {
public:
    explicit DeoptTrigger(MacroAssembler& masm, DeoptInfo& info)
        : masm_(masm), info_(info) {}
    
    /**
     * @brief Emit guard with deopt on failure
     */
    void emitGuard(Condition cond, DeoptReason reason, uint32_t bytecodeOffset);
    
    /**
     * @brief Emit unconditional deopt
     */
    void emitDeopt(DeoptReason reason, uint32_t bytecodeOffset);
    
    /**
     * @brief Record current frame state for deopt
     */
    void recordFrameState(const std::vector<DeoptValue>& locals,
                          const std::vector<DeoptValue>& stack);
    
private:
    MacroAssembler& masm_;
    DeoptInfo& info_;
    std::vector<DeoptValue> currentLocals_;
    std::vector<DeoptValue> currentStack_;
};

// =============================================================================
// OSR Exit (On-Stack Replacement exit)
// =============================================================================

/**
 * @brief OSR exit point for tier-down
 */
struct OSRExit {
    size_t jitOffset;
    uint32_t bytecodeOffset;
    std::vector<Register> liveRegisters;
    std::vector<int32_t> liveStackSlots;
};

/**
 * @brief OSR exit handler
 */
class OSRExitHandler {
public:
    /**
     * @brief Perform OSR exit from optimized to baseline
     */
    static void* exitToBaseline(void* optimizedFrame, const OSRExit& exit);
    
    /**
     * @brief Perform OSR exit from baseline to interpreter
     */
    static void* exitToInterpreter(void* baselineFrame, const OSRExit& exit);
};

} // namespace Zepra::JIT
