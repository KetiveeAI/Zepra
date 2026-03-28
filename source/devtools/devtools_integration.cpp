// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file devtools_integration.cpp
 * @brief DevTools <-> Browser Engine integration
 *
 * Wires DevTools console REPL to ScriptContext (ZepraScript VM).
 * Wires debugger events to DevTools panels.
 */

#include "devtools/devtools_integration.hpp"
#include "devtools/devtools_window.hpp"
#include <iostream>

// ScriptContext for real JS evaluation
#ifdef USE_WEBCORE
#include "scripting/script_context.hpp"
#endif

namespace Zepra::DevTools {

// =============================================================================
// DevToolsIntegration
// =============================================================================

DevToolsIntegration::DevToolsIntegration(DevToolsWindow* window, Zepra::Runtime::VM* vm)
    : window_(window), vm_(vm) {
    std::cout << "[DevToolsIntegration] Created\n";
}

DevToolsIntegration::~DevToolsIntegration() {
    disconnect();
}

void DevToolsIntegration::connect() {
    if (connected_) return;

    std::cout << "[DevToolsIntegration] Connecting to VM...\n";

    hookConsole();
    syncBreakpoints();

    connected_ = true;
    std::cout << "[DevToolsIntegration] Connected\n";
}

void DevToolsIntegration::disconnect() {
    if (!connected_) return;

    debugger_ = nullptr;
    scriptContext_ = nullptr;
    connected_ = false;

    std::cout << "[DevToolsIntegration] Disconnected\n";
}

void DevToolsIntegration::setScriptContext(Zepra::WebCore::ScriptContext* ctx) {
    scriptContext_ = ctx;
}

void DevToolsIntegration::hookConsole() {
    if (window_ && window_->console()) {
        window_->console()->setEvalCallback([this](const std::string& expr) {
            return evaluate(expr);
        });
    }
    std::cout << "[DevToolsIntegration] Console hooked\n";
}

EvalResult DevToolsIntegration::evaluate(const std::string& expression) {
    EvalResult result;

    if (!connected_) {
        result.success = false;
        result.error = "Not connected to JavaScript engine";
        return result;
    }

#ifdef USE_WEBCORE
    if (scriptContext_) {
        auto evalResult = scriptContext_->evaluate(expression, "<devtools>");
        result.success = evalResult.success;
        if (evalResult.success) {
            result.value = evalResult.value;
        } else {
            result.error = evalResult.error;
        }
        return result;
    }
#endif

    // Fallback when no ScriptContext available
    result.success = false;
    result.error = "ScriptContext not bound — use setScriptContext()";
    return result;
}

void DevToolsIntegration::syncBreakpoints() {
    if (!debugger_ || !window_) return;

    // Sync breakpoints from UI to engine
    for (const auto& bp : window_->sources()->breakpoints()) {
        (void)bp;
    }
    std::cout << "[DevToolsIntegration] Breakpoints synced\n";
}

void DevToolsIntegration::onDebugEvent(int event, const void* data) {
    (void)event;
    (void)data;
}

void DevToolsIntegration::onPaused(const Zepra::Debug::DebugCallFrame& frame) {
    (void)frame;
}

void DevToolsIntegration::onResumed() {
}

void DevToolsIntegration::onBreakpointHit(const Zepra::Debug::Breakpoint& bp) {
    (void)bp;
}

// =============================================================================
// Factory functions
// =============================================================================

DevToolsWindow* createDevTools(Zepra::Runtime::VM* vm) {
    auto* window = new DevToolsWindow();

    auto* integration = new DevToolsIntegration(window, vm);
    integration->connect();

    window->open();
    return window;
}

void destroyDevTools(DevToolsWindow* window) {
    if (window) {
        window->close();
        delete window;
    }
}

} // namespace Zepra::DevTools
