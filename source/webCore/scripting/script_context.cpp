// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file script_context.cpp
 * @brief ScriptContext implementation with ZepraScript VM integration
 */

#include "scripting/script_context.hpp"
#include <iostream>
#include <algorithm>
#include <chrono>

// NOTE: ZepraScript VM integration requires exposing Context/Compiler headers.
// For now using stub mode. Navigation still works, just no JS execution.

namespace Zepra::WebCore {

ScriptContext::ScriptContext() : vm_(nullptr) {
    std::cout << "[ScriptContext] Initialized (stub mode - URL bar works)" << std::endl;
}

ScriptContext::~ScriptContext() {
    // VM cleanup would go here when integrated
}

void ScriptContext::initialize(DOMDocument* document) {
    document_ = document;
    setupGlobals();
}

ScriptResult ScriptContext::evaluate(const std::string& code, const std::string& filename) {
    ScriptResult result;
    
    // Stub mode - navigation works, JS execution is placeholder
    result.success = true;
    result.value = "undefined";
    
    // Log execution for debugging
    if (!filename.empty() && consoleHandler_) {
        consoleHandler_("debug", "[ScriptContext] evaluate (stub): " + filename);
    }
    
    return result;
}

void ScriptContext::log(const std::string& message) {
    if (consoleHandler_) {
        consoleHandler_("log", message);
    } else {
        std::cout << "[console] " << message << std::endl;
    }
}

int ScriptContext::setTimeout(std::function<void()> callback, int delay) {
    int id = nextTimerId_++;
    double now = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    timers_.push_back({id, callback, delay, false, now + delay});
    return id;
}

int ScriptContext::setInterval(std::function<void()> callback, int interval) {
    int id = nextTimerId_++;
    double now = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    timers_.push_back({id, callback, interval, true, now + interval});
    return id;
}

void ScriptContext::clearTimeout(int id) {
    timers_.erase(std::remove_if(timers_.begin(), timers_.end(),
        [id](const TimerCallback& t) { return t.id == id; }), timers_.end());
}

void ScriptContext::clearInterval(int id) {
    clearTimeout(id);
}

void ScriptContext::processTimers() {
    double now = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    for (auto it = timers_.begin(); it != timers_.end(); ) {
        if (now >= it->scheduledTime) {
            if (it->callback) {
                it->callback();
            }
            
            if (it->repeating) {
                it->scheduledTime = now + it->delay;
                ++it;
            } else {
                it = timers_.erase(it);
            }
        } else {
            ++it;
        }
    }
}

void ScriptContext::setGlobal(const std::string& name, const std::string& value) {
    // Would set VM global when ZepraScript is linked
}

void ScriptContext::fireDOMContentLoaded() {
    for (auto& listener : domContentLoadedListeners_) {
        if (listener) listener();
    }
}

void ScriptContext::fireLoadEvent() {
    for (auto& listener : loadListeners_) {
        if (listener) listener();
    }
}

void ScriptContext::addEventListener(const std::string& eventType, std::function<void()> callback) {
    if (eventType == "DOMContentLoaded") {
        domContentLoadedListeners_.push_back(callback);
    } else if (eventType == "load") {
        loadListeners_.push_back(callback);
    }
}

void ScriptContext::alert(const std::string& message) {
    if (alertHandler_) {
        alertHandler_(message);
    } else {
        std::cout << "[alert] " << message << std::endl;
    }
}

bool ScriptContext::confirm(const std::string& message) {
    if (confirmHandler_) {
        return confirmHandler_(message);
    }
    return true;
}

std::string ScriptContext::prompt(const std::string& message, const std::string& defaultValue) {
    if (promptHandler_) {
        return promptHandler_(message, defaultValue);
    }
    return defaultValue;
}

void ScriptContext::setupGlobals() {
    setupWindowGlobals();
    setupDocumentGlobals();
}

void ScriptContext::setupWindowGlobals() {
    // Setup window object when ZepraScript is linked
}

void ScriptContext::setupDocumentGlobals() {
    // Setup document object when ZepraScript is linked
}

// DevToolsConsole
DevToolsConsole::DevToolsConsole() {}

void DevToolsConsole::log(const std::string& message, const std::string& source, int line) {
    addEntry("log", message, source, line);
}

void DevToolsConsole::clear() {
    entries_.clear();
}

void DevToolsConsole::addEntry(const std::string& level, const std::string& message, 
                                const std::string& source, int line) {
    LogEntry entry;
    entry.level = level;
    entry.message = message;
    entry.source = source;
    entry.line = line;
    entry.timestamp = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    entries_.push_back(entry);
    
    if (entries_.size() > maxEntries_) {
        entries_.erase(entries_.begin());
    }
    
    if (onLog_) {
        onLog_(entry);
    }
}

// DevToolsPanel
DevToolsPanel::DevToolsPanel() {}

} // namespace Zepra::WebCore
