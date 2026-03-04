/**
 * @file interpreter.cpp
 * @brief Debug/step-through interpreter
 * 
 * Provides instruction-level stepping for the debugger.
 * Wraps VM::run() with breakpoint checking and single-step support.
 * The primary execution path remains VM::run() for performance.
 */

#include "config.hpp"
#include "runtime/execution/vm.hpp"
#include "bytecode/opcode.hpp"
#include <functional>

namespace Zepra::Interpreter {

enum class StepMode {
    Run,        // Normal execution
    StepInto,   // Break at next instruction
    StepOver,   // Break at next instruction at same or lower depth
    StepOut     // Break when returning from current function
};

/**
 * Debug-capable interpreter that wraps the VM dispatch loop
 * with breakpoint and stepping support.
 */
class DebugInterpreter {
public:
    using BreakpointCallback = std::function<bool(size_t ip, size_t callDepth)>;
    using StepCallback = std::function<void(size_t ip, Bytecode::Opcode op, size_t callDepth)>;

    explicit DebugInterpreter(Runtime::VM* vm)
        : vm_(vm), stepMode_(StepMode::Run), stepDepth_(0) {}

    void setBreakpointCallback(BreakpointCallback cb) { breakpointCb_ = std::move(cb); }
    void setStepCallback(StepCallback cb) { stepCb_ = std::move(cb); }

    void setStepMode(StepMode mode) {
        stepMode_ = mode;
        stepDepth_ = vm_->getCallDepth();
    }

    StepMode stepMode() const { return stepMode_; }

    /**
     * Check if execution should pause at the current instruction.
     * Called by the debug hooks in the VM dispatch loop.
     */
    bool shouldPause(size_t ip, Bytecode::Opcode op) {
        size_t currentDepth = vm_->getCallDepth();

        switch (stepMode_) {
            case StepMode::Run:
                // Only pause on breakpoints
                if (breakpointCb_ && breakpointCb_(ip, currentDepth)) {
                    return true;
                }
                return false;

            case StepMode::StepInto:
                return true;

            case StepMode::StepOver:
                return currentDepth <= stepDepth_;

            case StepMode::StepOut:
                return currentDepth < stepDepth_;
        }
        return false;
    }

    /**
     * Notify the debugger of instruction execution.
     */
    void onInstruction(size_t ip, Bytecode::Opcode op) {
        if (stepCb_) {
            stepCb_(ip, op, vm_->getCallDepth());
        }
    }

private:
    Runtime::VM* vm_;
    StepMode stepMode_;
    size_t stepDepth_;
    BreakpointCallback breakpointCb_;
    StepCallback stepCb_;
};

} // namespace Zepra::Interpreter
