/**
 * @file ErrorAPI.h
 * @brief Error Hierarchy Implementation
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>

namespace Zepra::Runtime {

class Error : public std::exception {
public:
    Error() = default;
    explicit Error(std::string message) : message_(std::move(message)) {}
    Error(std::string message, std::shared_ptr<Error> cause)
        : message_(std::move(message)), cause_(std::move(cause)) {}
    
    const char* what() const noexcept override { return message_.c_str(); }
    const std::string& message() const { return message_; }
    virtual const std::string& name() const { static std::string n = "Error"; return n; }
    
    std::shared_ptr<Error> cause() const { return cause_; }
    void setCause(std::shared_ptr<Error> c) { cause_ = std::move(c); }
    
    const std::vector<std::string>& stack() const { return stack_; }
    void addStackFrame(std::string frame) { stack_.push_back(std::move(frame)); }
    
    std::string toString() const { return name() + ": " + message_; }

protected:
    std::string message_;
    std::shared_ptr<Error> cause_;
    std::vector<std::string> stack_;
};

class TypeError : public Error {
public:
    TypeError() = default;
    explicit TypeError(std::string message) : Error(std::move(message)) {}
    TypeError(std::string message, std::shared_ptr<Error> cause) : Error(std::move(message), std::move(cause)) {}
    const std::string& name() const override { static std::string n = "TypeError"; return n; }
};

class RangeError : public Error {
public:
    RangeError() = default;
    explicit RangeError(std::string message) : Error(std::move(message)) {}
    RangeError(std::string message, std::shared_ptr<Error> cause) : Error(std::move(message), std::move(cause)) {}
    const std::string& name() const override { static std::string n = "RangeError"; return n; }
};

class SyntaxError : public Error {
public:
    SyntaxError() = default;
    explicit SyntaxError(std::string message) : Error(std::move(message)) {}
    SyntaxError(std::string message, std::shared_ptr<Error> cause) : Error(std::move(message), std::move(cause)) {}
    const std::string& name() const override { static std::string n = "SyntaxError"; return n; }
};

class ReferenceError : public Error {
public:
    ReferenceError() = default;
    explicit ReferenceError(std::string message) : Error(std::move(message)) {}
    ReferenceError(std::string message, std::shared_ptr<Error> cause) : Error(std::move(message), std::move(cause)) {}
    const std::string& name() const override { static std::string n = "ReferenceError"; return n; }
};

class URIError : public Error {
public:
    URIError() = default;
    explicit URIError(std::string message) : Error(std::move(message)) {}
    URIError(std::string message, std::shared_ptr<Error> cause) : Error(std::move(message), std::move(cause)) {}
    const std::string& name() const override { static std::string n = "URIError"; return n; }
};

class EvalError : public Error {
public:
    EvalError() = default;
    explicit EvalError(std::string message) : Error(std::move(message)) {}
    EvalError(std::string message, std::shared_ptr<Error> cause) : Error(std::move(message), std::move(cause)) {}
    const std::string& name() const override { static std::string n = "EvalError"; return n; }
};

class AggregateError : public Error {
public:
    AggregateError() = default;
    explicit AggregateError(std::vector<std::shared_ptr<Error>> errors, std::string message = "")
        : Error(std::move(message)), errors_(std::move(errors)) {}
    
    const std::string& name() const override { static std::string n = "AggregateError"; return n; }
    const std::vector<std::shared_ptr<Error>>& errors() const { return errors_; }

private:
    std::vector<std::shared_ptr<Error>> errors_;
};

class InternalError : public Error {
public:
    InternalError() = default;
    explicit InternalError(std::string message) : Error(std::move(message)) {}
    const std::string& name() const override { static std::string n = "InternalError"; return n; }
};

// Factory functions
inline std::shared_ptr<Error> createError(const std::string& message) {
    return std::make_shared<Error>(message);
}

inline std::shared_ptr<TypeError> createTypeError(const std::string& message) {
    return std::make_shared<TypeError>(message);
}

inline std::shared_ptr<RangeError> createRangeError(const std::string& message) {
    return std::make_shared<RangeError>(message);
}

inline std::shared_ptr<SyntaxError> createSyntaxError(const std::string& message) {
    return std::make_shared<SyntaxError>(message);
}

inline std::shared_ptr<ReferenceError> createReferenceError(const std::string& message) {
    return std::make_shared<ReferenceError>(message);
}

} // namespace Zepra::Runtime
