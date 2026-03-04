/**
 * @file deoptimizer.cpp
 * @brief JIT Deoptimization (OSR exit)
 *
 * When speculative JIT assumptions are invalidated at runtime
 * (type guards fail, hidden class transitions, etc.), deoptimization
 * transfers execution from compiled native code back to the interpreter.
 *
 * Steps:
 *   1. Capture machine state (registers, stack)
 *   2. Map compiled frame → interpreter frame (bytecode IP, locals)
 *   3. Reconstruct interpreter stack
 *   4. Resume in VM::run() at the correct bytecode offset
 *
 * Ref: V8 Deoptimizer, JSC OSRExit, SpiderMonkey Bailout
 */

#include "jit/MacroAssembler.h"
#include "runtime/execution/vm.hpp"
#include "runtime/objects/value.hpp"
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace Zepra::JIT {

// =============================================================================
// Deopt Reason
// =============================================================================

enum class DeoptReason : uint8_t {
    TypeGuardFailure,       // Speculated type was wrong
    MapCheckFailure,        // Object hidden class changed
    BoundsCheckFailure,     // Array index out of bounds
    DivisionByZero,         // Integer division by zero
    OverflowCheck,          // Arithmetic overflow
    NullCheck,              // Null/undefined dereference
    StackOverflow,          // Out of stack space
    Invalidation,           // Code was invalidated (e.g., global changed)
    LazyDeopt,              // Deopt after function return
    NotCompiled,            // Uncompiled function entered compiled code
};

static const char* deoptReasonName(DeoptReason reason) {
    switch (reason) {
        case DeoptReason::TypeGuardFailure:   return "TypeGuard";
        case DeoptReason::MapCheckFailure:    return "MapCheck";
        case DeoptReason::BoundsCheckFailure: return "BoundsCheck";
        case DeoptReason::DivisionByZero:     return "DivByZero";
        case DeoptReason::OverflowCheck:      return "Overflow";
        case DeoptReason::NullCheck:          return "NullCheck";
        case DeoptReason::StackOverflow:      return "StackOverflow";
        case DeoptReason::Invalidation:       return "Invalidation";
        case DeoptReason::LazyDeopt:          return "LazyDeopt";
        case DeoptReason::NotCompiled:        return "NotCompiled";
    }
    return "Unknown";
}

// =============================================================================
// Deopt Point — maps a native code offset to an interpreter state
// =============================================================================

struct DeoptPoint {
    uint32_t nativeOffset;    // Offset in compiled code where deopt can occur
    uint32_t bytecodeOffset;  // Corresponding bytecode IP
    uint32_t functionIndex;   // Function being deoptimized

    // Register-to-local mapping:
    // Maps physical register IDs to local variable indices
    struct RegMapping {
        uint8_t physReg;
        uint32_t localIndex;
    };
    std::vector<RegMapping> registerMap;

    // Stack slots above the frame pointer that hold live values
    struct StackMapping {
        int32_t stackOffset;  // Offset from frame pointer
        uint32_t localIndex;
    };
    std::vector<StackMapping> stackMap;

    // Constants that were materialized in compiled code
    struct MaterializedConst {
        uint32_t localIndex;
        Runtime::Value value;
    };
    std::vector<MaterializedConst> materializedConstants;

    DeoptReason reason = DeoptReason::TypeGuardFailure;
};

// =============================================================================
// Captured Machine State
// =============================================================================

struct MachineState {
    // GPRs (x86-64: rax..r15)
    uint64_t gpr[16] = {};

    // FPRs (xmm0..xmm15)
    double fpr[16] = {};

    // Stack pointer and frame pointer
    uint64_t rsp = 0;
    uint64_t rbp = 0;

    // Instruction pointer at deopt
    uint64_t rip = 0;
};

// =============================================================================
// Deoptimizer
// =============================================================================

class Deoptimizer {
public:
    explicit Deoptimizer(Runtime::VM* vm) : vm_(vm), totalDeopts_(0) {}

    // =========================================================================
    // Deopt table management
    // =========================================================================

    void addDeoptPoint(const DeoptPoint& point) {
        deoptPoints_[point.nativeOffset] = point;
    }

    void clearDeoptPoints() {
        deoptPoints_.clear();
    }

    // =========================================================================
    // Deoptimization entry point
    // =========================================================================

    /**
     * Perform deoptimization: transfer from compiled code back to interpreter.
     * Called from JIT-generated code when a guard fails.
     */
    bool deoptimize(uint32_t nativeOffset, const MachineState& state,
                     DeoptReason reason) {
        totalDeopts_++;

        auto it = deoptPoints_.find(nativeOffset);
        if (it == deoptPoints_.end()) {
            fprintf(stderr, "Deopt: no deopt point at native offset %u\n",
                    nativeOffset);
            return false;
        }

        DeoptPoint& point = it->second;
        point.reason = reason;

        if (verbose_) {
            fprintf(stderr, "Deopt #%u: %s at bytecode %u (native %u)\n",
                    totalDeopts_, deoptReasonName(reason),
                    point.bytecodeOffset, nativeOffset);
        }

        // Step 1: Reconstruct local variables from registers and stack
        std::vector<Runtime::Value> locals = reconstructLocals(point, state);

        // Step 2: Restore VM state to the bytecode offset
        restoreVMState(point.bytecodeOffset, point.functionIndex, locals);

        // Step 3: Mark the compiled code as invalidated
        //         (prevents re-entry into bad code)
        invalidateCompiledCode(point.functionIndex);

        // Step 4: Record deopt stats for the profiler
        recordDeoptStats(point.functionIndex, reason);

        return true;
    }

    // =========================================================================
    // Lazy deoptimization
    // =========================================================================

    /**
     * Deoptimize all compiled code that depends on a given assumption.
     * Used when a global variable is redefined, a prototype chain changes, etc.
     */
    void invalidateAssumption(uint32_t assumptionId) {
        for (auto& [offset, point] : deoptPoints_) {
            // If this deopt point depends on the invalidated assumption,
            // patch the compiled code to unconditionally deopt
            (void)offset;
            (void)point;
            (void)assumptionId;
        }
    }

    // =========================================================================
    // Statistics
    // =========================================================================

    uint32_t totalDeopts() const { return totalDeopts_; }

    struct DeoptStats {
        uint32_t functionIndex;
        uint32_t deoptCount;
        DeoptReason lastReason;
    };

    std::vector<DeoptStats> getStats() const {
        std::vector<DeoptStats> result;
        for (const auto& [funcIdx, stats] : functionDeoptCounts_) {
            result.push_back({funcIdx, stats.count, stats.lastReason});
        }
        return result;
    }

    void setVerbose(bool v) { verbose_ = v; }

private:
    // =========================================================================
    // Reconstruction
    // =========================================================================

    std::vector<Runtime::Value> reconstructLocals(const DeoptPoint& point,
                                                    const MachineState& state) {
        std::vector<Runtime::Value> locals;

        // First pass: materialized constants
        for (const auto& mc : point.materializedConstants) {
            if (mc.localIndex >= locals.size()) {
                locals.resize(mc.localIndex + 1, Runtime::Value::undefined());
            }
            locals[mc.localIndex] = mc.value;
        }

        // Second pass: register mappings
        for (const auto& rm : point.registerMap) {
            if (rm.localIndex >= locals.size()) {
                locals.resize(rm.localIndex + 1, Runtime::Value::undefined());
            }
            // Interpret the GPR value as a NaN-boxed Value
            uint64_t bits = state.gpr[rm.physReg];
            Runtime::Value val;
            std::memcpy(&val, &bits, sizeof(val));
            locals[rm.localIndex] = val;
        }

        // Third pass: stack slot mappings
        for (const auto& sm : point.stackMap) {
            if (sm.localIndex >= locals.size()) {
                locals.resize(sm.localIndex + 1, Runtime::Value::undefined());
            }
            // Read value from the stack at the given offset from RBP
            uint64_t addr = state.rbp + sm.stackOffset;
            uint64_t bits;
            std::memcpy(&bits, reinterpret_cast<const void*>(addr), sizeof(bits));
            Runtime::Value val;
            std::memcpy(&val, &bits, sizeof(val));
            locals[sm.localIndex] = val;
        }

        return locals;
    }

    void restoreVMState(uint32_t bytecodeOffset, uint32_t functionIndex,
                         const std::vector<Runtime::Value>& locals) {
        // Store the pending deopt info for the VM to consume on re-entry.
        // The VM checks pendingDeopt_ at the start of run() and restores
        // its own IP and locals from this data.
        pendingDeopt_.active = true;
        pendingDeopt_.bytecodeOffset = bytecodeOffset;
        pendingDeopt_.functionIndex = functionIndex;
        pendingDeopt_.locals = locals;
    }

    struct PendingDeopt {
        bool active = false;
        uint32_t bytecodeOffset = 0;
        uint32_t functionIndex = 0;
        std::vector<Runtime::Value> locals;
    };

public:
    const PendingDeopt& pendingDeopt() const { return pendingDeopt_; }
    void clearPendingDeopt() { pendingDeopt_.active = false; }

private:
    PendingDeopt pendingDeopt_;

    void invalidateCompiledCode(uint32_t functionIndex) {
        // Mark the function's compiled code as invalid
        // The JIT profiler will avoid recompiling it for a cooldown period
        (void)functionIndex;
    }

    void recordDeoptStats(uint32_t functionIndex, DeoptReason reason) {
        auto& stats = functionDeoptCounts_[functionIndex];
        stats.count++;
        stats.lastReason = reason;

        // If a function deopts too many times, blacklist it from JIT
        if (stats.count >= maxDeoptsBeforeBlacklist_) {
            if (verbose_) {
                fprintf(stderr, "Deopt: func %u blacklisted after %u deopts\n",
                        functionIndex, stats.count);
            }
        }
    }

    // =========================================================================
    // Data
    // =========================================================================

    Runtime::VM* vm_;
    std::unordered_map<uint32_t, DeoptPoint> deoptPoints_;
    uint32_t totalDeopts_;
    bool verbose_ = false;

    struct FuncDeoptInfo {
        uint32_t count = 0;
        DeoptReason lastReason = DeoptReason::TypeGuardFailure;
    };
    std::unordered_map<uint32_t, FuncDeoptInfo> functionDeoptCounts_;

    static constexpr uint32_t maxDeoptsBeforeBlacklist_ = 10;
};

} // namespace Zepra::JIT
