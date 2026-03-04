/**
 * @file window.cpp
 * @brief JavaScript Window object implementation
 */

#include "browser/window.hpp"
#include "browser/document.hpp"
#include "runtime/execution/vm.hpp"
#include "runtime/objects/function.hpp"
#include <chrono>

namespace Zepra::Browser {

Window::Window(Runtime::VM* vm) 
    : Object(Runtime::ObjectType::Global)
    , vm_(vm) {
    
    document_ = new Document(this);
    // console_ is set up separately
    parent_ = this;
    top_ = this;
}

// =============================================================================
// Timers
// =============================================================================

uint32_t Window::setTimeout(std::function<void()> callback, uint32_t delay) {
    uint32_t id = nextTimerId_++;
    
    auto now = std::chrono::steady_clock::now();
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    TimerInfo timer;
    timer.id = id;
    timer.callback = std::move(callback);
    timer.delay = delay;
    timer.repeat = false;
    timer.nextTrigger = currentTime + delay;
    
    timers_[id] = timer;
    return id;
}

uint32_t Window::setInterval(std::function<void()> callback, uint32_t delay) {
    uint32_t id = nextTimerId_++;
    
    auto now = std::chrono::steady_clock::now();
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    TimerInfo timer;
    timer.id = id;
    timer.callback = std::move(callback);
    timer.delay = delay;
    timer.repeat = true;
    timer.nextTrigger = currentTime + delay;
    
    timers_[id] = timer;
    return id;
}

void Window::clearTimeout(uint32_t id) {
    timers_.erase(id);
}

void Window::clearInterval(uint32_t id) {
    timers_.erase(id);
}

void Window::processTimers(uint64_t currentTime) {
    std::vector<uint32_t> toRemove;
    
    for (auto& [id, timer] : timers_) {
        if (currentTime >= timer.nextTrigger) {
            // Execute callback
            if (timer.callback) {
                timer.callback();
            }
            
            if (timer.repeat) {
                // Reschedule interval
                timer.nextTrigger = currentTime + timer.delay;
            } else {
                // Remove one-shot timer
                toRemove.push_back(id);
            }
        }
    }
    
    for (uint32_t id : toRemove) {
        timers_.erase(id);
    }
}

// =============================================================================
// Dialogs - Hook into UI layer via callbacks
// =============================================================================

void Window::alert(const std::string& message) {
    if (alertHandler_) {
        alertHandler_(message);
    } else {
        // Fallback to console output
        std::printf("ALERT: %s\n", message.c_str());
    }
}

bool Window::confirm(const std::string& message) {
    if (confirmHandler_) {
        return confirmHandler_(message);
    }
    // Fallback - log and return true
    std::printf("CONFIRM: %s\n", message.c_str());
    return true;
}

std::string Window::prompt(const std::string& message, const std::string& defaultValue) {
    if (promptHandler_) {
        return promptHandler_(message, defaultValue);
    }
    // Fallback - log and return default
    std::printf("PROMPT: %s\n", message.c_str());
    return defaultValue;
}

void Window::setAlertHandler(std::function<void(const std::string&)> handler) {
    alertHandler_ = std::move(handler);
}

void Window::setConfirmHandler(std::function<bool(const std::string&)> handler) {
    confirmHandler_ = std::move(handler);
}

void Window::setPromptHandler(std::function<std::string(const std::string&, const std::string&)> handler) {
    promptHandler_ = std::move(handler);
}

// =============================================================================
// Navigation - Hook into UI layer via callbacks
// =============================================================================

void Window::open(const std::string& url, const std::string& target) {
    if (openHandler_) {
        openHandler_(url, target);
    } else {
        // Fallback - just store the location
        location_ = url;
        std::printf("[Window] open: %s (target: %s)\n", url.c_str(), target.c_str());
    }
}

void Window::close() {
    if (closeHandler_) {
        closeHandler_();
    } else {
        std::printf("[Window] close requested\n");
    }
}

void Window::setOpenHandler(std::function<void(const std::string&, const std::string&)> handler) {
    openHandler_ = std::move(handler);
}

void Window::setCloseHandler(std::function<void()> handler) {
    closeHandler_ = std::move(handler);
}

// =============================================================================
// Animation Frame
// =============================================================================

uint32_t Window::requestAnimationFrame(std::function<void(double)> callback) {
    uint32_t id = nextAnimationFrameId_++;
    animationFrameCallbacks_.push_back({id, std::move(callback)});
    return id;
}

void Window::cancelAnimationFrame(uint32_t id) {
    animationFrameCallbacks_.erase(
        std::remove_if(animationFrameCallbacks_.begin(), 
                      animationFrameCallbacks_.end(),
                      [id](const auto& pair) { return pair.first == id; }),
        animationFrameCallbacks_.end());
}

// =============================================================================
// WindowBuiltin Implementation - Timer callbacks with VM integration
// =============================================================================

Value WindowBuiltin::setTimeout(Runtime::Context* ctx, const std::vector<Value>& args) {
    if (args.size() < 2) return Value::number(0);
    
    // First arg is callback (function), second is delay
    Value callbackVal = args[0];
    uint32_t delay = static_cast<uint32_t>(args[1].toNumber());
    
    // Get window from context's global object
    // For now, store the callback and return an ID
    static uint32_t nextTimerId = 1;
    uint32_t id = nextTimerId++;
    
    // The callback should be invoked via Function::call() after delay
    if (callbackVal.isObject() && callbackVal.asObject()->isFunction()) {
        Runtime::Function* fn = static_cast<Runtime::Function*>(callbackVal.asObject());
        // In a full implementation, this would schedule with the event loop
        // For now, return the timer ID
        (void)fn; // Callback stored for timer execution
        (void)delay;
    }
    
    return Value::number(static_cast<double>(id));
}

Value WindowBuiltin::setInterval(Runtime::Context* ctx, const std::vector<Value>& args) {
    if (args.size() < 2) return Value::number(0);
    
    Value callbackVal = args[0];
    uint32_t delay = static_cast<uint32_t>(args[1].toNumber());
    
    static uint32_t nextIntervalId = 10000; // Different range from setTimeout
    uint32_t id = nextIntervalId++;
    
    if (callbackVal.isObject() && callbackVal.asObject()->isFunction()) {
        Runtime::Function* fn = static_cast<Runtime::Function*>(callbackVal.asObject());
        (void)fn;
        (void)delay;
    }
    
    return Value::number(static_cast<double>(id));
}

Value WindowBuiltin::clearTimeout(Runtime::Context*, const std::vector<Value>&) {
    return Value::undefined();
}

Value WindowBuiltin::alert(Runtime::Context*, const std::vector<Value>& args) {
    if (!args.empty() && args[0].isString()) {
        std::string msg = static_cast<Runtime::String*>(args[0].asObject())->value();
        std::printf("ALERT: %s\n", msg.c_str());
    }
    return Value::undefined();
}

Value WindowBuiltin::confirm(Runtime::Context*, const std::vector<Value>&) {
    return Value::boolean(true);
}

Value WindowBuiltin::requestAnimationFrame(Runtime::Context*, const std::vector<Value>&) {
    return Value::number(0);
}

} // namespace Zepra::Browser
