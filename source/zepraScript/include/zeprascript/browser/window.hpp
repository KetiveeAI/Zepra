#pragma once

/**
 * @file window.hpp
 * @brief JavaScript Window object for browser environment
 */

#include "../config.hpp"
#include "../runtime/object.hpp"
#include "../runtime/value.hpp"
#include <functional>
#include <unordered_map>
#include <vector>

namespace Zepra::Runtime { class Context; class VM; }

namespace Zepra::Browser {

using Runtime::Value;
using Runtime::Object;

// Forward declarations
class Document;
class Console;

/**
 * @brief Timer callback info
 */
struct TimerInfo {
    uint32_t id;
    std::function<void()> callback;
    uint32_t delay;
    bool repeat;
    uint64_t nextTrigger;
};

/**
 * @brief JavaScript Window object
 * 
 * Global object in browser context.
 */
class Window : public Object {
public:
    explicit Window(Runtime::VM* vm);
    
    // Window properties
    Document* document() const { return document_; }
    Console* console() const { return console_; }
    Window* parent() const { return parent_; }
    Window* top() const { return top_; }
    
    // Location
    std::string location() const { return location_; }
    void setLocation(const std::string& url) { location_ = url; }
    
    // Dimensions
    int innerWidth() const { return innerWidth_; }
    int innerHeight() const { return innerHeight_; }
    int outerWidth() const { return outerWidth_; }
    int outerHeight() const { return outerHeight_; }
    
    // Timers
    uint32_t setTimeout(std::function<void()> callback, uint32_t delay);
    uint32_t setInterval(std::function<void()> callback, uint32_t delay);
    void clearTimeout(uint32_t id);
    void clearInterval(uint32_t id);
    
    // Process pending timers
    void processTimers(uint64_t currentTime);
    
    // Alert/Confirm/Prompt
    void alert(const std::string& message);
    bool confirm(const std::string& message);
    std::string prompt(const std::string& message, const std::string& defaultValue = "");
    
    // Navigation
    void open(const std::string& url);
    void close();
    
    // Animation frame
    uint32_t requestAnimationFrame(std::function<void(double)> callback);
    void cancelAnimationFrame(uint32_t id);
    
private:
    Runtime::VM* vm_;
    Document* document_ = nullptr;
    Console* console_ = nullptr;
    Window* parent_ = nullptr;
    Window* top_ = nullptr;
    
    std::string location_;
    int innerWidth_ = 1024;
    int innerHeight_ = 768;
    int outerWidth_ = 1024;
    int outerHeight_ = 768;
    
    std::unordered_map<uint32_t, TimerInfo> timers_;
    uint32_t nextTimerId_ = 1;
    
    std::vector<std::pair<uint32_t, std::function<void(double)>>> animationFrameCallbacks_;
    uint32_t nextAnimationFrameId_ = 1;
};

/**
 * @brief Window builtin functions
 */
class WindowBuiltin {
public:
    static Value setTimeout(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value setInterval(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value clearTimeout(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value alert(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value confirm(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value requestAnimationFrame(Runtime::Context* ctx, const std::vector<Value>& args);
};

} // namespace Zepra::Browser
