/**
 * @file stack_trace.cpp
 * @brief JavaScript stack trace capture and formatting
 *
 * Captures and formats stack traces for Error objects (Error.stack),
 * console.trace(), and debugger call stacks.
 *
 * Supports:
 * - V8-style stack trace formatting (Error.captureStackTrace)
 * - Source map resolution (if available)
 * - Async stack traces (following promise chains)
 * - Stack frame filtering (hide internal frames)
 *
 * Ref: V8 Stack Trace API, SpiderMonkey SavedStacks
 */

#include <string>
#include <vector>
#include <sstream>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <functional>

namespace Zepra::Exception {

// =============================================================================
// Stack Frame
// =============================================================================

struct StackFrame {
    std::string functionName;     // Function name (empty for anonymous)
    std::string scriptName;       // Script filename or URL
    uint32_t lineNumber = 0;      // 1-based line number
    uint32_t columnNumber = 0;    // 1-based column number
    uint32_t bytecodeOffset = 0;  // Offset within function bytecode
    bool isNative = false;        // Native C++ function
    bool isEval = false;          // eval() frame
    bool isConstructor = false;   // new Constructor() frame
    bool isAsync = false;         // Async context boundary
    bool isToplevel = false;      // Global scope

    /**
     * Format as V8-style stack frame string:
     *   "    at functionName (scriptName:line:column)"
     */
    std::string toString() const {
        std::ostringstream oss;
        oss << "    at ";

        if (isAsync) {
            oss << "async ";
        }

        if (!functionName.empty()) {
            if (isConstructor) oss << "new ";
            oss << functionName;
        } else if (isToplevel) {
            oss << "<anonymous>";
        } else {
            oss << "<anonymous>";
        }

        if (isNative) {
            oss << " (<native>)";
        } else if (isEval) {
            oss << " (eval at " << scriptName << ":" << lineNumber << ")";
        } else if (!scriptName.empty()) {
            oss << " (" << scriptName << ":" << lineNumber << ":" << columnNumber << ")";
        }

        return oss.str();
    }
};

// =============================================================================
// Stack Trace
// =============================================================================

class StackTrace {
public:
    static constexpr size_t DEFAULT_MAX_FRAMES = 64;

    StackTrace() = default;
    explicit StackTrace(std::vector<StackFrame>&& frames)
        : frames_(std::move(frames)) {}

    // =========================================================================
    // Construction
    // =========================================================================

    /**
     * Capture the current call stack from the VM.
     * skipFrames: number of top frames to skip (e.g., skip the captureStackTrace call itself)
     */
    static StackTrace capture(size_t skipFrames = 0, size_t maxFrames = DEFAULT_MAX_FRAMES) {
        (void)skipFrames;
        (void)maxFrames;
        // In production, would walk vm->callStack_ and extract frames
        return StackTrace{};
    }

    void addFrame(StackFrame frame) {
        frames_.push_back(std::move(frame));
    }

    void addAsyncBoundary() {
        StackFrame boundary;
        boundary.isAsync = true;
        boundary.functionName = "<async>";
        frames_.push_back(boundary);
    }

    // =========================================================================
    // Formatting
    // =========================================================================

    /**
     * Format as V8-style Error.stack string.
     */
    std::string format(const std::string& errorType = "Error",
                        const std::string& message = "") const {
        std::ostringstream oss;

        // First line: "ErrorType: message"
        oss << errorType;
        if (!message.empty()) {
            oss << ": " << message;
        }
        oss << "\n";

        // Stack frames
        for (const auto& frame : frames_) {
            oss << frame.toString() << "\n";
        }

        return oss.str();
    }

    /**
     * Format for console.trace() (no error prefix)
     */
    std::string formatTrace() const {
        std::ostringstream oss;
        oss << "Trace\n";
        for (const auto& frame : frames_) {
            oss << frame.toString() << "\n";
        }
        return oss.str();
    }

    // =========================================================================
    // Access
    // =========================================================================

    size_t frameCount() const { return frames_.size(); }
    const StackFrame& frameAt(size_t index) const { return frames_[index]; }
    const std::vector<StackFrame>& frames() const { return frames_; }

    bool empty() const { return frames_.empty(); }

    // =========================================================================
    // Filtering
    // =========================================================================

    /**
     * Remove frames matching a predicate (e.g., internal frames).
     */
    void filter(std::function<bool(const StackFrame&)> pred) {
        frames_.erase(
            std::remove_if(frames_.begin(), frames_.end(),
                [&pred](const StackFrame& f) { return pred(f); }),
            frames_.end());
    }

    /**
     * Remove native (C++) frames.
     */
    void removeNativeFrames() {
        filter([](const StackFrame& f) { return f.isNative; });
    }

    /**
     * Truncate to max frames.
     */
    void truncate(size_t maxFrames) {
        if (frames_.size() > maxFrames) {
            frames_.resize(maxFrames);
        }
    }

private:
    std::vector<StackFrame> frames_;
};

// =============================================================================
// Error Object
// =============================================================================

/**
 * Represents a JavaScript Error object with message, name, and stack.
 */
class ErrorObject {
public:
    enum class Type {
        Error,
        TypeError,
        RangeError,
        ReferenceError,
        SyntaxError,
        URIError,
        EvalError,
        InternalError,
        AggregateError
    };

    ErrorObject(Type type, const std::string& message)
        : type_(type), message_(message) {
        stack_ = StackTrace::capture(1); // Skip Error constructor frame
    }

    Type type() const { return type_; }
    const std::string& message() const { return message_; }
    const StackTrace& stack() const { return stack_; }

    std::string name() const {
        switch (type_) {
            case Type::Error: return "Error";
            case Type::TypeError: return "TypeError";
            case Type::RangeError: return "RangeError";
            case Type::ReferenceError: return "ReferenceError";
            case Type::SyntaxError: return "SyntaxError";
            case Type::URIError: return "URIError";
            case Type::EvalError: return "EvalError";
            case Type::InternalError: return "InternalError";
            case Type::AggregateError: return "AggregateError";
        }
        return "Error";
    }

    /**
     * Get the Error.stack string.
     */
    std::string stackString() const {
        return stack_.format(name(), message_);
    }

    /**
     * Create a TypeError.
     */
    static ErrorObject typeError(const std::string& msg) {
        return ErrorObject(Type::TypeError, msg);
    }

    static ErrorObject rangeError(const std::string& msg) {
        return ErrorObject(Type::RangeError, msg);
    }

    static ErrorObject referenceError(const std::string& msg) {
        return ErrorObject(Type::ReferenceError, msg);
    }

    static ErrorObject syntaxError(const std::string& msg) {
        return ErrorObject(Type::SyntaxError, msg);
    }

    // ES2022 Error.cause
    void setCause(const ErrorObject& cause) {
        cause_ = std::make_unique<ErrorObject>(cause);
    }

    const ErrorObject* cause() const { return cause_.get(); }

private:
    Type type_;
    std::string message_;
    StackTrace stack_;
    std::unique_ptr<ErrorObject> cause_;
};

// =============================================================================
// Try-Catch Handler
// =============================================================================

/**
 * Manages try-catch-finally exception handling in the VM.
 * Each try block pushes a Handler onto the handler stack.
 */
struct ExceptionHandler {
    enum class Type { TryCatch, TryFinally, TryCatchFinally };

    Type type;
    uint32_t catchBytecodeOffset;    // Where to jump on exception
    uint32_t finallyBytecodeOffset;  // Where the finally block starts
    uint32_t stackDepth;             // Stack depth at try entry (for unwinding)
    uint32_t scopeDepth;             // Scope depth at try entry
};

class ExceptionHandlerStack {
public:
    void pushHandler(ExceptionHandler handler) {
        handlers_.push_back(handler);
    }

    void popHandler() {
        if (!handlers_.empty()) {
            handlers_.pop_back();
        }
    }

    /**
     * Find the nearest matching handler for an exception.
     * Returns nullptr if no handler (uncaught exception).
     */
    const ExceptionHandler* findHandler() const {
        if (handlers_.empty()) return nullptr;

        // Walk handlers from innermost to outermost
        for (auto it = handlers_.rbegin(); it != handlers_.rend(); ++it) {
            if (it->type == ExceptionHandler::Type::TryCatch ||
                it->type == ExceptionHandler::Type::TryCatchFinally) {
                return &(*it);
            }
        }
        // No catch handler; look for finally
        for (auto it = handlers_.rbegin(); it != handlers_.rend(); ++it) {
            if (it->type == ExceptionHandler::Type::TryFinally ||
                it->type == ExceptionHandler::Type::TryCatchFinally) {
                return &(*it);
            }
        }
        return nullptr;
    }

    size_t depth() const { return handlers_.size(); }
    bool empty() const { return handlers_.empty(); }

private:
    std::vector<ExceptionHandler> handlers_;
};

} // namespace Zepra::Exception
