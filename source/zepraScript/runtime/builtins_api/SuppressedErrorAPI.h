/**
 * @file SuppressedErrorAPI.h
 * @brief SuppressedError Implementation for Disposal
 */

#pragma once

#include <string>
#include <vector>
#include <exception>
#include <memory>

namespace Zepra::Runtime {

// =============================================================================
// SuppressedError
// =============================================================================

class SuppressedError : public std::exception {
public:
    SuppressedError(std::exception_ptr error, std::exception_ptr suppressed,
                    const std::string& message = "")
        : error_(error), suppressed_(suppressed), message_(message) {
        buildFullMessage();
    }
    
    const char* what() const noexcept override { return fullMessage_.c_str(); }
    
    std::exception_ptr error() const { return error_; }
    std::exception_ptr suppressed() const { return suppressed_; }
    const std::string& message() const { return message_; }
    
    std::string errorMessage() const {
        try {
            if (error_) std::rethrow_exception(error_);
        } catch (const std::exception& e) {
            return e.what();
        } catch (...) {
            return "Unknown error";
        }
        return "";
    }
    
    std::string suppressedMessage() const {
        try {
            if (suppressed_) std::rethrow_exception(suppressed_);
        } catch (const std::exception& e) {
            return e.what();
        } catch (...) {
            return "Unknown error";
        }
        return "";
    }

private:
    void buildFullMessage() {
        fullMessage_ = "SuppressedError: ";
        if (!message_.empty()) {
            fullMessage_ += message_;
        } else {
            fullMessage_ += errorMessage();
        }
        fullMessage_ += " (suppressed: " + suppressedMessage() + ")";
    }
    
    std::exception_ptr error_;
    std::exception_ptr suppressed_;
    std::string message_;
    std::string fullMessage_;
};

// =============================================================================
// Error Aggregator
// =============================================================================

class ErrorAggregator {
public:
    void add(std::exception_ptr error) {
        errors_.push_back(error);
    }
    
    bool hasErrors() const { return !errors_.empty(); }
    size_t count() const { return errors_.size(); }
    
    const std::vector<std::exception_ptr>& errors() const { return errors_; }
    
    std::exception_ptr primary() const {
        return errors_.empty() ? nullptr : errors_.front();
    }
    
    std::exception_ptr toSuppressedError(const std::string& message = "") const {
        if (errors_.empty()) return nullptr;
        if (errors_.size() == 1) return errors_[0];
        
        std::exception_ptr result = errors_[0];
        for (size_t i = 1; i < errors_.size(); ++i) {
            try {
                throw SuppressedError(result, errors_[i], message);
            } catch (...) {
                result = std::current_exception();
            }
        }
        return result;
    }
    
    void rethrowIfAny() const {
        if (!errors_.empty()) {
            std::rethrow_exception(toSuppressedError());
        }
    }

private:
    std::vector<std::exception_ptr> errors_;
};

// =============================================================================
// AggregateError
// =============================================================================

class AggregateError : public std::exception {
public:
    AggregateError(const std::vector<std::exception_ptr>& errors,
                   const std::string& message = "")
        : errors_(errors), message_(message) {
        if (message_.empty()) {
            fullMessage_ = "AggregateError: " + std::to_string(errors_.size()) + " errors";
        } else {
            fullMessage_ = "AggregateError: " + message_;
        }
    }
    
    const char* what() const noexcept override { return fullMessage_.c_str(); }
    
    const std::vector<std::exception_ptr>& errors() const { return errors_; }
    const std::string& message() const { return message_; }

private:
    std::vector<std::exception_ptr> errors_;
    std::string message_;
    std::string fullMessage_;
};

} // namespace Zepra::Runtime
