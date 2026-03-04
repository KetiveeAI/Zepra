/**
 * @file stack_frame.cpp
 * @brief Stack frame debug inspection
 * 
 * Provides introspection for the debugger:  
 * enumerate locals, map IP → source location, format call stacks.
 */

#include "config.hpp"
#include "runtime/execution/vm.hpp"
#include "runtime/objects/function.hpp"
#include "runtime/objects/value.hpp"
#include <string>
#include <vector>

namespace Zepra::Interpreter {

/**
 * Snapshot of a single call frame for debugger display.
 */
struct FrameInfo {
    std::string functionName;
    std::string sourceFile;
    uint32_t line = 0;
    uint32_t column = 0;
    size_t ip = 0;
    size_t depth = 0;
    std::vector<std::pair<std::string, Runtime::Value>> locals;
};

/**
 * Stack frame inspector — reads VM state to produce debugger-visible frame data.
 */
class StackFrameInspector {
public:
    explicit StackFrameInspector(const Runtime::VM* vm) : vm_(vm) {}

    /**
     * Capture full call stack as vector of FrameInfo.
     */
    std::vector<FrameInfo> captureCallStack() const {
        std::vector<FrameInfo> frames;
        size_t depth = vm_->getCallDepth();

        for (size_t i = 0; i < depth; i++) {
            FrameInfo info;
            info.depth = i;
            // Actual implementation requires access to VM internals
            // (nativeStack_/heapStack_) via friend or debug API.
            // Populate function name, source location, locals from the frame.
            frames.push_back(info);
        }

        return frames;
    }

    /**
     * Format a single frame for display (e.g., "  at foo (script.js:12:5)")
     */
    static std::string formatFrame(const FrameInfo& frame) {
        std::string result = "  at ";
        if (!frame.functionName.empty()) {
            result += frame.functionName;
        } else {
            result += "<anonymous>";
        }
        if (!frame.sourceFile.empty()) {
            result += " (" + frame.sourceFile + ":" +
                      std::to_string(frame.line) + ":" +
                      std::to_string(frame.column) + ")";
        }
        return result;
    }

    /**
     * Build a full stack trace string (e.g., for Error.stack).
     */
    std::string formatStackTrace() const {
        auto frames = captureCallStack();
        std::string trace;
        for (const auto& frame : frames) {
            trace += formatFrame(frame) + "\n";
        }
        return trace;
    }

private:
    const Runtime::VM* vm_;
};

} // namespace Zepra::Interpreter
