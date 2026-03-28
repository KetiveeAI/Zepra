/**
 * @file devtools_integration.hpp
 * @brief Browser-DevTools Integration Layer
 * 
 * Provides the interface between the main browser and DevTools.
 * Two usage patterns:
 *   1. Browser-embedded: show(vm) / hide() lifecycle from F12
 *   2. Standalone window: DevToolsIntegration(window, vm) factory
 */

#pragma once

#include <memory>
#include <atomic>
#include <string>

// Forward declarations
namespace Zepra::Runtime {
    class VM;
    class Context;
}

namespace Zepra::Debug {
    class Debugger;
    struct DebugCallFrame;
    struct Breakpoint;
}

namespace Zepra::WebCore {
    class ScriptContext;
}

namespace Zepra::DevTools {

class DevToolsWindow;
class DevToolsPanel;
struct NetworkEntry;

/**
 * @brief Result of evaluating an expression in DevTools console
 */
struct EvalResult {
    bool success = false;
    std::string value;
    std::string type;
    std::string error;
};

/**
 * @brief Manages DevTools lifecycle and connection to browser
 */
class DevToolsIntegration {
public:
    // Standalone window mode
    DevToolsIntegration(DevToolsWindow* window, Zepra::Runtime::VM* vm);
    ~DevToolsIntegration();

    // Connection
    void connect();
    void disconnect();
    bool isConnected() const { return connected_; }

    // Console evaluation — wired to ScriptContext when available
    EvalResult evaluate(const std::string& expression);

    // ScriptContext binding (from browser side)
    void setScriptContext(Zepra::WebCore::ScriptContext* ctx);

    // Debugger events
    void onDebugEvent(int event, const void* data);

private:
    void hookConsole();
    void syncBreakpoints();
    void onPaused(const Zepra::Debug::DebugCallFrame& frame);
    void onResumed();
    void onBreakpointHit(const Zepra::Debug::Breakpoint& bp);

    DevToolsWindow* window_ = nullptr;
    Zepra::Runtime::VM* vm_ = nullptr;
    Zepra::Debug::Debugger* debugger_ = nullptr;
    Zepra::WebCore::ScriptContext* scriptContext_ = nullptr;
    bool connected_ = false;
};

// Factory functions
DevToolsWindow* createDevTools(Zepra::Runtime::VM* vm);
void destroyDevTools(DevToolsWindow* window);

} // namespace Zepra::DevTools
