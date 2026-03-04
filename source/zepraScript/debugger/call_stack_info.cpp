/**
 * @file call_stack_info.cpp
 * @brief Call stack introspection for debugging
 * 
 * Provides runtime stack traversal with:
 * - Frame-by-frame local variable extraction
 * - Closure variable capture information
 * - 'this' binding resolution per frame
 * - Async stack trace support
 */

#include "debugger/debugger.hpp"
#include "runtime/objects/value.hpp"
#include "config.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <functional>

namespace Zepra::Debug {

using Runtime::Value;

// =============================================================================
// ScopeType - Types of variable scopes
// =============================================================================

enum class ScopeType {
    Local,      // Function local variables
    Closure,    // Variables captured from enclosing scope
    Block,      // Block-scoped variables (let/const)
    Catch,      // Catch block parameter
    With,       // With statement scope
    Module,     // Module-level variables
    Global      // Global scope
};

// =============================================================================
// VariableInfo - Information about a single variable
// =============================================================================

struct VariableInfo {
    std::string name;
    Value value;
    ScopeType scope;
    bool isConst;
    bool isArgumentsObject;     // Is the 'arguments' pseudo-array
    int argumentIndex;          // If function parameter, which position (-1 if not)
    
    VariableInfo()
        : scope(ScopeType::Local)
        , isConst(false)
        , isArgumentsObject(false)
        , argumentIndex(-1) {}
};

// =============================================================================
// StackFrame - Detailed information about a single stack frame
// =============================================================================

struct StackFrame {
    // Identification
    std::string functionName;
    std::string sourceFile;
    uint32_t line;
    uint32_t column;
    size_t frameIndex;          // Position in call stack (0 = top)
    
    // Function info
    bool isNative;              // Native (C++) function
    bool isConstructor;         // Called with 'new'
    bool isAsync;               // Async function
    bool isGenerator;           // Generator function
    
    // Variables
    std::vector<VariableInfo> variables;
    Value thisValue;
    Value returnValue;          // Set when stepping out
    
    // Bytecode info
    size_t bytecodeOffset;
    
    StackFrame()
        : line(0), column(0), frameIndex(0)
        , isNative(false), isConstructor(false)
        , isAsync(false), isGenerator(false)
        , bytecodeOffset(0) {}
};

// =============================================================================
// AsyncFrame - For async stack trace reconstruction
// =============================================================================

struct AsyncFrame {
    std::string description;    // "async" or "Promise.then" etc.
    std::vector<StackFrame> frames;
    
    bool empty() const { return frames.empty(); }
};

// =============================================================================
// CallStackInspector - Stack introspection API
// =============================================================================

class CallStackInspector {
public:
    using FrameCallback = std::function<void(const StackFrame&)>;
    using VariableCallback = std::function<void(const VariableInfo&)>;
    
    /**
     * @brief Set the VM callback for getting call depth
     */
    void setGetCallDepthCallback(std::function<size_t()> callback) {
        getCallDepth_ = std::move(callback);
    }
    
    /**
     * @brief Set the VM callback for getting frame info
     */
    void setGetFrameCallback(
        std::function<StackFrame(size_t frameIndex)> callback) {
        getFrame_ = std::move(callback);
    }
    
    /**
     * @brief Set the VM callback for evaluating expressions
     */
    void setEvaluateCallback(
        std::function<Value(size_t frameIndex, const std::string& expr)> callback) {
        evaluate_ = std::move(callback);
    }
    
    // -------------------------------------------------------------------------
    // Stack Traversal
    // -------------------------------------------------------------------------
    
    /**
     * @brief Get depth of the call stack
     */
    size_t getDepth() const {
        if (getCallDepth_) {
            return getCallDepth_();
        }
        return 0;
    }
    
    /**
     * @brief Get information about a specific frame
     * @param frameIndex 0 = top of stack (current frame)
     */
    std::optional<StackFrame> getFrame(size_t frameIndex) const {
        if (!getFrame_) return std::nullopt;
        
        size_t depth = getDepth();
        if (frameIndex >= depth) return std::nullopt;
        
        return getFrame_(frameIndex);
    }
    
    /**
     * @brief Get all frames in the call stack
     */
    std::vector<StackFrame> getAllFrames() const {
        std::vector<StackFrame> frames;
        size_t depth = getDepth();
        
        for (size_t i = 0; i < depth; ++i) {
            auto frame = getFrame(i);
            if (frame) {
                frames.push_back(*frame);
            }
        }
        
        return frames;
    }
    
    /**
     * @brief Iterate over frames
     */
    void forEachFrame(FrameCallback callback) const {
        size_t depth = getDepth();
        for (size_t i = 0; i < depth; ++i) {
            auto frame = getFrame(i);
            if (frame) {
                callback(*frame);
            }
        }
    }
    
    // -------------------------------------------------------------------------
    // Variable Inspection
    // -------------------------------------------------------------------------
    
    /**
     * @brief Get all variables visible in a frame
     */
    std::vector<VariableInfo> getVariables(size_t frameIndex) const {
        auto frame = getFrame(frameIndex);
        if (!frame) return {};
        return frame->variables;
    }
    
    /**
     * @brief Get variables of a specific scope type
     */
    std::vector<VariableInfo> getVariablesByScope(size_t frameIndex, 
                                                   ScopeType scope) const {
        std::vector<VariableInfo> result;
        auto vars = getVariables(frameIndex);
        
        for (const auto& var : vars) {
            if (var.scope == scope) {
                result.push_back(var);
            }
        }
        
        return result;
    }
    
    /**
     * @brief Look up a specific variable by name
     */
    std::optional<VariableInfo> findVariable(size_t frameIndex,
                                              const std::string& name) const {
        auto vars = getVariables(frameIndex);
        
        for (const auto& var : vars) {
            if (var.name == name) {
                return var;
            }
        }
        
        return std::nullopt;
    }
    
    /**
     * @brief Get 'this' value for a frame
     */
    Value getThisValue(size_t frameIndex) const {
        auto frame = getFrame(frameIndex);
        if (!frame) return Value::undefined();
        return frame->thisValue;
    }
    
    /**
     * @brief Get return value (when stepping out)
     */
    Value getReturnValue(size_t frameIndex) const {
        auto frame = getFrame(frameIndex);
        if (!frame) return Value::undefined();
        return frame->returnValue;
    }
    
    // -------------------------------------------------------------------------
    // Expression Evaluation
    // -------------------------------------------------------------------------
    
    /**
     * @brief Evaluate an expression in the context of a frame
     */
    Value evaluate(size_t frameIndex, const std::string& expression) const {
        if (!evaluate_) return Value::undefined();
        return evaluate_(frameIndex, expression);
    }
    
    /**
     * @brief Check if a variable name is valid in scope
     */
    bool isVariableInScope(size_t frameIndex, const std::string& name) const {
        return findVariable(frameIndex, name).has_value();
    }
    
    // -------------------------------------------------------------------------
    // Async Stack Traces
    // -------------------------------------------------------------------------
    
    /**
     * @brief Get async stack trace (captures Promise chains)
     */
    std::vector<AsyncFrame> getAsyncStack() const {
        return asyncStack_;
    }
    
    /**
     * @brief Push an async frame (called when entering async context)
     */
    void pushAsyncFrame(const AsyncFrame& frame) {
        // Limit async stack depth to prevent memory issues
        if (asyncStack_.size() < MAX_ASYNC_DEPTH) {
            asyncStack_.push_back(frame);
        }
    }
    
    /**
     * @brief Pop an async frame (called when leaving async context)
     */
    void popAsyncFrame() {
        if (!asyncStack_.empty()) {
            asyncStack_.pop_back();
        }
    }
    
    /**
     * @brief Clear async stack
     */
    void clearAsyncStack() {
        asyncStack_.clear();
    }
    
    // -------------------------------------------------------------------------
    // Stack Trace Formatting
    // -------------------------------------------------------------------------
    
    /**
     * @brief Format stack trace as string
     */
    std::string formatStackTrace() const {
        std::string result;
        
        forEachFrame([&result](const StackFrame& frame) {
            result += "    at ";
            
            if (!frame.functionName.empty()) {
                result += frame.functionName;
            } else {
                result += "<anonymous>";
            }
            
            result += " (";
            result += frame.sourceFile;
            result += ":";
            result += std::to_string(frame.line);
            result += ":";
            result += std::to_string(frame.column);
            result += ")\n";
        });
        
        // Include async stack if available
        if (!asyncStack_.empty()) {
            result += "    --- async ---\n";
            for (const auto& asyncFrame : asyncStack_) {
                result += "    " + asyncFrame.description + "\n";
                for (const auto& frame : asyncFrame.frames) {
                    result += "        at " + frame.functionName;
                    result += " (" + frame.sourceFile + ":";
                    result += std::to_string(frame.line) + ")\n";
                }
            }
        }
        
        return result;
    }
    
    /**
     * @brief Format a single frame as string
     */
    static std::string formatFrame(const StackFrame& frame) {
        std::string result = frame.functionName.empty() ? 
            "<anonymous>" : frame.functionName;
        result += " at ";
        result += frame.sourceFile;
        result += ":";
        result += std::to_string(frame.line);
        if (frame.column > 0) {
            result += ":";
            result += std::to_string(frame.column);
        }
        return result;
    }
    
private:
    static constexpr size_t MAX_ASYNC_DEPTH = 64;
    
    std::function<size_t()> getCallDepth_;
    std::function<StackFrame(size_t)> getFrame_;
    std::function<Value(size_t, const std::string&)> evaluate_;
    
    std::vector<AsyncFrame> asyncStack_;
};

// =============================================================================
// ClosureInspector - For inspecting closure captures
// =============================================================================

class ClosureInspector {
public:
    /**
     * @brief Captured variable information
     */
    struct CapturedVar {
        std::string name;
        Value value;
        std::string originalScope;  // File/function where it was defined
        uint32_t originalLine;
    };
    
    /**
     * @brief Get captured variables for a closure
     */
    std::vector<CapturedVar> getCapturedVariables(Value functionValue) const {
        std::vector<CapturedVar> result;
        
        // TODO: Implement actual closure inspection
        // This requires Function object to expose closure environment
        (void)functionValue;
        
        return result;
    }
};

// =============================================================================
// Global CallStackInspector Instance
// =============================================================================

static CallStackInspector* globalCallStackInspector = nullptr;

CallStackInspector* getCallStackInspector() {
    if (!globalCallStackInspector) {
        globalCallStackInspector = new CallStackInspector();
    }
    return globalCallStackInspector;
}

void shutdownCallStackInspector() {
    delete globalCallStackInspector;
    globalCallStackInspector = nullptr;
}

} // namespace Zepra::Debug
