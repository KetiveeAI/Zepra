#pragma once

/**
 * @file async.hpp
 * @brief async/await support
 */

#include "config.hpp"
#include "runtime/objects/value.hpp"
#include "promise.hpp"

namespace Zepra::Runtime {

/**
 * @brief Async function context
 * Manages state for async function execution
 */
class AsyncContext {
public:
    AsyncContext(Promise* promise);
    
    Promise* promise() const { return promise_; }
    
    // State machine support
    void setState(int state) { state_ = state; }
    int state() const { return state_; }
    
private:
    Promise* promise_;
    int state_;  // Current state in state machine
};

} // namespace Zepra::Runtime
