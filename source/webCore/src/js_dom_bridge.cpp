/**
 * @file js_dom_bridge.cpp
 * @brief Implementation of JS-DOM bridge (minimal stub version)
 * 
 * This is a minimal stub that compiles. Full implementation requires
 * deeper integration with ZepraScript's Context and native function system.
 */

#include "webcore/js_dom_bridge.hpp"
#include "webcore/dom.hpp"

// Include actual ZepraScript headers
#include "zeprascript/runtime/value.hpp"
#include "zeprascript/runtime/object.hpp"
#include "zeprascript/runtime/vm.hpp"

#include <iostream>

namespace Zepra::Bridge {

// Store bridge instance for static callbacks
static JSDOMBridge* g_bridge = nullptr;

JSDOMBridge::JSDOMBridge() {
    g_bridge = this;
}

JSDOMBridge::~JSDOMBridge() {
    g_bridge = nullptr;
}

void JSDOMBridge::initialize(Runtime::VM* vm, WebCore::DOMDocument* document) {
    vm_ = vm;
    document_ = document;
    
    // TODO: Once ZepraScript Context API is integrated, expose globals
    std::cout << "[JSDOMBridge] Initialized with document" << std::endl;
}

Runtime::Object* JSDOMBridge::wrapDocument(WebCore::DOMDocument* doc) {
    // TODO: Implement when Object creation API is available
    return nullptr;
}

Runtime::Object* JSDOMBridge::wrapElement(WebCore::DOMElement* element) {
    // TODO: Implement when Object creation API is available
    return nullptr;
}

Runtime::Object* JSDOMBridge::wrapNode(WebCore::DOMNode* node) {
    // TODO: Implement when Object creation API is available
    return nullptr;
}

Runtime::Object* JSDOMBridge::wrapEvent(WebCore::DOMEvent* event) {
    // TODO: Implement when Object creation API is available
    return nullptr;
}

WebCore::DOMNode* JSDOMBridge::unwrapNode(Runtime::Object* obj) {
    // TODO: Implement
    return nullptr;
}

WebCore::DOMElement* JSDOMBridge::unwrapElement(Runtime::Object* obj) {
    // TODO: Implement
    return nullptr;
}

void JSDOMBridge::exposeWindowObject() {
    // TODO: Expose window globals via Context
    std::cout << "[JSDOMBridge] Window object exposed (stub)" << std::endl;
}

void JSDOMBridge::exposeDocumentObject() {
    // TODO: Expose document via Context
    std::cout << "[JSDOMBridge] Document object exposed (stub)" << std::endl;
}

void JSDOMBridge::exposeConsoleObject() {
    // Console already provided by ZepraScript builtins
}

Runtime::Value JSDOMBridge::executeScript(const std::string& code) {
    // TODO: Compile and execute code through VM
    std::cout << "[JSDOMBridge] executeScript: " << code.substr(0, 50) << "..." << std::endl;
    return Runtime::Value::undefined();
}

void JSDOMBridge::dispatchEventToJS(WebCore::DOMEvent* event) {
    // TODO: Dispatch event to registered handlers
}

// Static DOM methods - implementations

Runtime::Value JSDOMBridge::js_getElementById(Runtime::Context*,
                                               const std::vector<Runtime::Value>& args) {
    if (!g_bridge || !g_bridge->document_ || args.empty()) {
        return Runtime::Value::null();
    }
    
    std::string id = args[0].toString();
    WebCore::DOMElement* element = g_bridge->document_->getElementById(id);
    
    if (element) {
        // For now, return a simple object with basic info
        // Full wrapping would create a proper Element object
        std::cout << "[JSDOMBridge] getElementById('" << id << "') -> found: " << element->tagName() << "\n";
        // TODO: Return wrapped object when Object API is ready
        // Return a truthy value to indicate found (1.0 = found)
        return Runtime::Value::number(1.0);
    }
    
    std::cout << "[JSDOMBridge] getElementById('" << id << "') -> null\n";
    return Runtime::Value::null();
}

Runtime::Value JSDOMBridge::js_getElementsByTagName(Runtime::Context*,
                                                     const std::vector<Runtime::Value>&) {
    return Runtime::Value::null();
}

Runtime::Value JSDOMBridge::js_querySelector(Runtime::Context*,
                                              const std::vector<Runtime::Value>&) {
    return Runtime::Value::null();
}

Runtime::Value JSDOMBridge::js_querySelectorAll(Runtime::Context*,
                                                 const std::vector<Runtime::Value>&) {
    return Runtime::Value::null();
}

Runtime::Value JSDOMBridge::js_createElement(Runtime::Context*,
                                              const std::vector<Runtime::Value>&) {
    return Runtime::Value::null();
}

Runtime::Value JSDOMBridge::js_appendChild(Runtime::Context*,
                                            const std::vector<Runtime::Value>&) {
    return Runtime::Value::undefined();
}

Runtime::Value JSDOMBridge::js_removeChild(Runtime::Context*,
                                            const std::vector<Runtime::Value>&) {
    return Runtime::Value::undefined();
}

Runtime::Value JSDOMBridge::js_addEventListener(Runtime::Context*,
                                                 const std::vector<Runtime::Value>&) {
    return Runtime::Value::undefined();
}

Runtime::Value JSDOMBridge::js_setAttribute(Runtime::Context*,
                                             const std::vector<Runtime::Value>&) {
    return Runtime::Value::undefined();
}

Runtime::Value JSDOMBridge::js_getAttribute(Runtime::Context*,
                                             const std::vector<Runtime::Value>&) {
    return Runtime::Value::undefined();
}

Runtime::Value JSDOMBridge::js_setInnerHTML(Runtime::Context*,
                                             const std::vector<Runtime::Value>&) {
    return Runtime::Value::undefined();
}

Runtime::Value JSDOMBridge::js_getInnerHTML(Runtime::Context*,
                                             const std::vector<Runtime::Value>&) {
    return Runtime::Value::undefined();
}

// JSWindow implementation - stubs

void JSWindow::expose(Runtime::VM*, JSDOMBridge*) {
    // Already done via bridge
}

Runtime::Value JSWindow::js_alert(Runtime::Context*,
                                   const std::vector<Runtime::Value>& args) {
    if (!args.empty()) {
        std::cout << "[ALERT] " << args[0].toString() << std::endl;
    }
    return Runtime::Value::undefined();
}

Runtime::Value JSWindow::js_confirm(Runtime::Context*,
                                     const std::vector<Runtime::Value>&) {
    return Runtime::Value::boolean(true);
}

Runtime::Value JSWindow::js_prompt(Runtime::Context*,
                                    const std::vector<Runtime::Value>&) {
    return Runtime::Value::null();
}

Runtime::Value JSWindow::js_setTimeout(Runtime::Context*,
                                        const std::vector<Runtime::Value>&) {
    static int timerId = 0;
    return Runtime::Value::number(++timerId);
}

Runtime::Value JSWindow::js_setInterval(Runtime::Context*,
                                         const std::vector<Runtime::Value>&) {
    static int intervalId = 0;
    return Runtime::Value::number(++intervalId);
}

Runtime::Value JSWindow::js_clearTimeout(Runtime::Context*,
                                          const std::vector<Runtime::Value>&) {
    return Runtime::Value::undefined();
}

Runtime::Value JSWindow::js_clearInterval(Runtime::Context*,
                                           const std::vector<Runtime::Value>&) {
    return Runtime::Value::undefined();
}

Runtime::Value JSWindow::js_requestAnimationFrame(Runtime::Context*,
                                                   const std::vector<Runtime::Value>&) {
    static int rafId = 0;
    return Runtime::Value::number(++rafId);
}

Runtime::Value JSWindow::js_fetch(Runtime::Context*,
                                   const std::vector<Runtime::Value>&) {
    // TODO: Return a Promise
    return Runtime::Value::undefined();
}

} // namespace Zepra::Bridge
