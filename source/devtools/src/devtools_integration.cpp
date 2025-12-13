/**
 * @file devtools_integration.cpp
 * @brief DevTools <-> Debug API integration
 */

#include "devtools/devtools_integration.hpp"
#include <iostream>

// Note: In a real implementation, these would include actual headers
// #include "zeprascript/debug/debugger.hpp"
// #include "zeprascript/runtime/vm.hpp"

namespace Zepra::DevTools {

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
    
    // Get debugger from VM
    // debugger_ = vm_->getDebugger();
    
    // Hook console
    hookConsole();
    
    // Sync breakpoints
    syncBreakpoints();
    
    connected_ = true;
    std::cout << "[DevToolsIntegration] Connected\n";
}

void DevToolsIntegration::disconnect() {
    if (!connected_) return;
    
    debugger_ = nullptr;
    connected_ = false;
    
    std::cout << "[DevToolsIntegration] Disconnected\n";
}

void DevToolsIntegration::hookConsole() {
    // Hook console.log to send messages to DevTools Console panel
    window_->console()->setEvalCallback([this](const std::string& expr) {
        return evaluate(expr);
    });
    
    std::cout << "[DevToolsIntegration] Console hooked\n";
}

EvalResult DevToolsIntegration::evaluate(const std::string& expression) {
    EvalResult result;
    
    if (!connected_) {
        result.success = false;
        result.error = "Not connected to JavaScript engine";
        return result;
    }
    
    // In real implementation:
    // auto value = debugger_->evaluate(expression);
    // result.success = !value.isException();
    // result.value = value.toString();
    
    // Demo implementation
    result.success = true;
    if (expression == "1 + 1") {
        result.value = "2";
        result.type = "number";
    } else if (expression.substr(0, 8) == "console.") {
        result.value = "undefined";
        result.type = "undefined";
    } else {
        result.value = "undefined";
        result.type = "undefined";
    }
    
    return result;
}

void DevToolsIntegration::syncBreakpoints() {
    if (!debugger_ || !window_) return;
    
    // Sync breakpoints from UI to engine
    for (const auto& bp : window_->sources()->breakpoints()) {
        // debugger_->setBreakpoint(bp.url, bp.line, bp.condition);
    }
    
    std::cout << "[DevToolsIntegration] Breakpoints synced\n";
}

void DevToolsIntegration::onDebugEvent(int event, const void* data) {
    // Handle events from JS engine
    // switch (static_cast<Debug::DebugEvent>(event)) {
    //     case Debug::DebugEvent::BreakpointHit:
    //         onBreakpointHit(*static_cast<const Debug::Breakpoint*>(data));
    //         break;
    //     case Debug::DebugEvent::ExecutionPaused:
    //         onPaused(*static_cast<const Debug::DebugCallFrame*>(data));
    //         break;
    //     case Debug::DebugEvent::ExecutionResumed:
    //         onResumed();
    //         break;
    // }
}

void DevToolsIntegration::onPaused(const Zepra::Debug::DebugCallFrame& frame) {
    // Update Sources panel
    // window_->sources()->openSource(frame.sourceFile, frame.line);
    // window_->setActivePanel(PanelType::Sources);
}

void DevToolsIntegration::onResumed() {
    // Clear paused indicators
}

void DevToolsIntegration::onBreakpointHit(const Zepra::Debug::Breakpoint& bp) {
    // TODO: Implement when integrated with zepra-core
    // std::cout << "[DevToolsIntegration] Breakpoint hit: " << bp.id << "\n";
    (void)bp; // Suppress unused warning
}

// =============================================================================
// Factory functions
// =============================================================================

DevToolsWindow* createDevTools(Zepra::Runtime::VM* vm) {
    auto* window = new DevToolsWindow();
    
    // Create integration
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
