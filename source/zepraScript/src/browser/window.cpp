/**
 * @file window.cpp
 * @brief JavaScript Window object implementation
 */

#include "zeprascript/browser/window.hpp"
#include "zeprascript/browser/document.hpp"
#include "zeprascript/runtime/vm.hpp"
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
// Dialogs
// =============================================================================

void Window::alert(const std::string& message) {
    // TODO: Hook into UI layer
    // For now, just print
    std::printf("ALERT: %s\n", message.c_str());
}

bool Window::confirm(const std::string& message) {
    // TODO: Hook into UI layer
    std::printf("CONFIRM: %s\n", message.c_str());
    return true; // Default to true
}

std::string Window::prompt(const std::string& message, const std::string& defaultValue) {
    // TODO: Hook into UI layer
    std::printf("PROMPT: %s\n", message.c_str());
    return defaultValue;
}

// =============================================================================
// Navigation
// =============================================================================

void Window::open(const std::string& url) {
    // TODO: Open new window/tab
    location_ = url;
}

void Window::close() {
    // TODO: Close window
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
// WindowBuiltin Implementation
// =============================================================================

Value WindowBuiltin::setTimeout(Runtime::Context*, const std::vector<Value>&) {
    // TODO: Implement with callback handling
    return Value::number(0);
}

Value WindowBuiltin::setInterval(Runtime::Context*, const std::vector<Value>&) {
    // TODO: Implement with callback handling
    return Value::number(0);
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
