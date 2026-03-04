/**
 * @file ErrorEnhancementsAPI.h
 * @brief Error Enhancements Implementation
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <exception>
#include <sstream>
#include <typeinfo>

namespace Zepra::Runtime {

// =============================================================================
// Stack Frame
// =============================================================================

struct StackFrame {
    std::string functionName;
    std::string fileName;
    int lineNumber = 0;
    int columnNumber = 0;
    bool isNative = false;
    bool isEval = false;
    bool isConstructor = false;
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "    at ";
        if (!functionName.empty()) {
            oss << functionName;
        } else {
            oss << "<anonymous>";
        }
        if (!fileName.empty()) {
            oss << " (" << fileName;
            if (lineNumber > 0) {
                oss << ":" << lineNumber;
                if (columnNumber > 0) {
                    oss << ":" << columnNumber;
                }
            }
            oss << ")";
        }
        return oss.str();
    }
};

// =============================================================================
// Stack Trace
// =============================================================================

class StackTrace {
public:
    void push(StackFrame frame) { frames_.push_back(std::move(frame)); }
    void pop() { if (!frames_.empty()) frames_.pop_back(); }
    
    const std::vector<StackFrame>& frames() const { return frames_; }
    
    std::string toString() const {
        std::ostringstream oss;
        for (const auto& frame : frames_) {
            oss << frame.toString() << "\n";
        }
        return oss.str();
    }
    
    static StackTrace capture(int skipFrames = 0) {
        StackTrace trace;
        return trace;
    }

private:
    std::vector<StackFrame> frames_;
};

// =============================================================================
// Enhanced Error
// =============================================================================

class EnhancedError : public std::exception {
public:
    EnhancedError(const std::string& message, const std::string& name = "Error")
        : message_(message), name_(name), stack_(StackTrace::capture(1)) {}
    
    const char* what() const noexcept override { return message_.c_str(); }
    
    const std::string& message() const { return message_; }
    const std::string& name() const { return name_; }
    const StackTrace& stack() const { return stack_; }
    
    std::shared_ptr<EnhancedError> cause() const { return cause_; }
    void setCause(std::shared_ptr<EnhancedError> cause) { cause_ = std::move(cause); }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << name_ << ": " << message_ << "\n";
        oss << stack_.toString();
        if (cause_) {
            oss << "Caused by:\n" << cause_->toString();
        }
        return oss.str();
    }

private:
    std::string message_;
    std::string name_;
    StackTrace stack_;
    std::shared_ptr<EnhancedError> cause_;
};

// =============================================================================
// Error.isError
// =============================================================================

inline bool isError(const std::exception& e) {
    return dynamic_cast<const EnhancedError*>(&e) != nullptr;
}

template<typename T>
bool isError(const T&) {
    return std::is_base_of_v<std::exception, T>;
}

// =============================================================================
// Structured Errors
// =============================================================================

class StructuredError : public EnhancedError {
public:
    StructuredError(const std::string& message, const std::string& code)
        : EnhancedError(message, "StructuredError"), code_(code) {}
    
    const std::string& code() const { return code_; }
    
    void setDetail(const std::string& key, const std::string& value) {
        details_[key] = value;
    }
    
    std::string getDetail(const std::string& key) const {
        auto it = details_.find(key);
        return it != details_.end() ? it->second : "";
    }

private:
    std::string code_;
    std::map<std::string, std::string> details_;
};

// =============================================================================
// Error Extensions
// =============================================================================

class ErrorExtensions {
public:
    static void captureStackTrace(EnhancedError& error, int skipFrames = 0) {
    }
    
    static void prepareStackTrace(std::function<std::string(const EnhancedError&, const StackTrace&)> formatter) {
        formatter_ = std::move(formatter);
    }
    
    static size_t stackTraceLimit() { return stackTraceLimit_; }
    static void setStackTraceLimit(size_t limit) { stackTraceLimit_ = limit; }

private:
    static inline std::function<std::string(const EnhancedError&, const StackTrace&)> formatter_;
    static inline size_t stackTraceLimit_ = 10;
};

} // namespace Zepra::Runtime
