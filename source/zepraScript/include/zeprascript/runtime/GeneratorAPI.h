/**
 * @file GeneratorAPI.h
 * @brief Generator Function Implementation
 * 
 * ECMAScript Generators:
 * - Generator object with yield
 * - Generator state machine
 * - Generator.prototype.next/return/throw
 */

#pragma once

#include "IteratorAPI.h"
#include <memory>
#include <functional>
#include <optional>
#include <variant>

namespace Zepra::Runtime {

// =============================================================================
// Generator State
// =============================================================================

enum class GeneratorState {
    SuspendedStart,   // Created but not started
    SuspendedYield,   // Paused at yield
    Executing,        // Running
    Completed         // Finished (return or throw)
};

// =============================================================================
// Generator Context
// =============================================================================

/**
 * @brief Execution context for generator
 */
class GeneratorContext {
public:
    using YieldCallback = std::function<void(IteratorValue)>;
    
    GeneratorContext() = default;
    
    // Set yield callback (called when yield is executed)
    void setYieldCallback(YieldCallback cb) { yieldCallback_ = std::move(cb); }
    
    // Yield a value
    void yield(IteratorValue value) {
        if (yieldCallback_) {
            yieldCallback_(std::move(value));
        }
    }
    
    // Set return value
    void setReturnValue(IteratorValue value) {
        returnValue_ = std::move(value);
    }
    
    // Get return value
    const std::optional<IteratorValue>& returnValue() const {
        return returnValue_;
    }
    
    // Resume value (passed via next())
    void setResumeValue(IteratorValue value) {
        resumeValue_ = std::move(value);
    }
    
    const std::optional<IteratorValue>& resumeValue() const {
        return resumeValue_;
    }
    
    void clearResumeValue() { resumeValue_.reset(); }
    
private:
    YieldCallback yieldCallback_;
    std::optional<IteratorValue> returnValue_;
    std::optional<IteratorValue> resumeValue_;
};

// =============================================================================
// Generator
// =============================================================================

/**
 * @brief Generator object implementing Iterator protocol
 */
class Generator : public Iterator {
public:
    using Body = std::function<void(GeneratorContext&)>;
    
    explicit Generator(Body body)
        : body_(std::move(body))
        , state_(GeneratorState::SuspendedStart) {}
    
    // Iterator interface
    IteratorResult next() override {
        return next(std::monostate{});
    }
    
    IteratorResult next(IteratorValue value) {
        if (state_ == GeneratorState::Completed) {
            return IteratorResult::done();
        }
        
        if (state_ == GeneratorState::Executing) {
            throw std::runtime_error("Generator is already executing");
        }
        
        // Set resume value
        context_.setResumeValue(std::move(value));
        
        // Execute
        state_ = GeneratorState::Executing;
        
        std::optional<IteratorValue> yieldedValue;
        
        context_.setYieldCallback([&yieldedValue, this](IteratorValue v) {
            yieldedValue = std::move(v);
            state_ = GeneratorState::SuspendedYield;
            // Would suspend execution here (requires coroutines)
        });
        
        try {
            if (body_) {
                body_(context_);
            }
            
            if (!yieldedValue) {
                // Generator completed normally
                state_ = GeneratorState::Completed;
                auto retVal = context_.returnValue();
                return IteratorResult{
                    retVal.value_or(std::monostate{}),
                    true
                };
            }
        } catch (const std::exception& e) {
            state_ = GeneratorState::Completed;
            throw;
        }
        
        // Yielded value
        return IteratorResult{*yieldedValue, false};
    }
    
    IteratorResult return_(IteratorValue value) override {
        if (state_ == GeneratorState::Completed) {
            return IteratorResult{value, true};
        }
        
        state_ = GeneratorState::Completed;
        return IteratorResult{value, true};
    }
    
    IteratorResult throw_(IteratorValue exception) override {
        if (state_ == GeneratorState::Completed) {
            throw std::runtime_error("Generator throw after completion");
        }
        
        state_ = GeneratorState::Completed;
        
        if (auto* str = std::get_if<std::string>(&exception)) {
            throw std::runtime_error(*str);
        }
        throw std::runtime_error("Generator exception");
    }
    
    GeneratorState state() const { return state_; }
    
private:
    Body body_;
    GeneratorContext context_;
    GeneratorState state_;
};

// =============================================================================
// Generator Function
// =============================================================================

/**
 * @brief Factory for creating generators
 */
class GeneratorFunction {
public:
    using Body = Generator::Body;
    
    explicit GeneratorFunction(Body body) : body_(std::move(body)) {}
    
    std::unique_ptr<Generator> operator()() const {
        return std::make_unique<Generator>(body_);
    }
    
private:
    Body body_;
};

// =============================================================================
// Yield Helper (for C++ generators)
// =============================================================================

/**
 * @brief Helper for implementing generators in C++
 */
class YieldBuilder {
public:
    void operator<<(IteratorValue value) {
        values_.push_back(std::move(value));
    }
    
    std::unique_ptr<Generator> build() {
        auto values = std::make_shared<std::vector<IteratorValue>>(std::move(values_));
        size_t index = 0;
        
        return std::make_unique<Generator>([values, index](GeneratorContext& ctx) mutable {
            for (const auto& value : *values) {
                ctx.yield(value);
            }
        });
    }
    
private:
    std::vector<IteratorValue> values_;
};

} // namespace Zepra::Runtime
