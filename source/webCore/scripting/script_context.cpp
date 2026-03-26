// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file script_context.cpp
 * @brief ScriptContext — wired to ZepraScript VM (real implementation)
 *
 * Uses the internal VM directly:
 *   SourceCode::fromString → Parser → SyntaxChecker → BytecodeGenerator → VM::execute
 */

#include "scripting/script_context.hpp"
#include <iostream>
#include <algorithm>
#include <chrono>

// ZepraScript internal headers
#include "runtime/execution/vm.hpp"
#include "runtime/execution/context.hpp"
#include "runtime/execution/global_object.hpp"
#include "runtime/objects/function.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/objects/value.hpp"
#include "frontend/source_code.hpp"
#include "frontend/parser.hpp"
#include "frontend/syntax_checker.hpp"
#include "bytecode/bytecode_generator.hpp"
#include "builtins/console.hpp"

namespace Zepra::WebCore {

// Internal state — owns VM and Context lifetime
static std::unique_ptr<Zepra::Runtime::Context> s_vmContext;
static std::unique_ptr<Zepra::Runtime::VM> s_vm;
static bool s_vmReady = false;

static void initVM() {
    if (s_vmReady) return;
    
    try {
        // Create VM (Context wraps VM — circular reference, VM owns execution)
        s_vm = std::make_unique<Zepra::Runtime::VM>(nullptr);
        s_vmContext = std::make_unique<Zepra::Runtime::Context>(s_vm.get());
        
        // Create global object with builtins (console, Math, JSON, etc.)
        auto* globalObj = new Zepra::Runtime::GlobalObject(s_vmContext.get());
        s_vm->setGlobal("globalThis", Zepra::Runtime::Value::object(globalObj));
        
        s_vmReady = true;
        std::cout << "[ScriptContext] ZepraScript VM initialized" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ScriptContext] VM init failed: " << e.what() << std::endl;
        s_vm.reset();
        s_vmContext.reset();
        s_vmReady = false;
    }
}

ScriptContext::ScriptContext() : vm_(nullptr) {
    initVM();
    if (s_vmReady) {
        vm_ = s_vm.get();
    }
}

ScriptContext::~ScriptContext() {
    // VM lifetime managed by statics
}

void ScriptContext::initialize(DOMDocument* document) {
    document_ = document;
    setupGlobals();
}

ScriptResult ScriptContext::evaluate(const std::string& code, const std::string& filename) {
    ScriptResult result;
    
    if (code.empty()) {
        result.success = true;
        result.value = "undefined";
        return result;
    }
    
    if (!vm_ || !s_vmReady) {
        result.success = false;
        result.error = "VM not available";
        return result;
    }
    
    try {
        // Full compile pipeline: source → AST → bytecode → execute
        auto sourceCode = Zepra::Frontend::SourceCode::fromString(
            code, filename.empty() ? "<script>" : filename);
        
        Zepra::Frontend::Parser parser(sourceCode.get());
        auto ast = parser.parseProgram();
        
        if (parser.hasErrors()) {
            result.success = false;
            result.error = "SyntaxError: Parse failed";
            if (consoleHandler_) {
                consoleHandler_("error", result.error);
            }
            return result;
        }
        
        Zepra::Frontend::SyntaxChecker checker;
        if (!checker.check(ast.get())) {
            result.success = false;
            result.error = "SyntaxError: Validation failed";
            if (consoleHandler_) {
                consoleHandler_("error", result.error);
            }
            return result;
        }
        
        Zepra::Bytecode::BytecodeGenerator generator;
        auto chunk = generator.compile(ast.get());
        
        if (generator.hasErrors() || !chunk) {
            result.success = false;
            result.error = "CompileError: Bytecode generation failed";
            if (consoleHandler_) {
                consoleHandler_("error", result.error);
            }
            return result;
        }
        
        // Execute bytecode on VM
        auto execResult = vm_->execute(chunk.get());
        
        if (execResult.status == Zepra::Runtime::ExecutionResult::Status::Success) {
            result.success = true;
            result.value = execResult.value.toString();
        } else {
            result.success = false;
            result.error = execResult.error;
            if (consoleHandler_) {
                consoleHandler_("error", execResult.error);
            }
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.error = std::string("RuntimeError: ") + e.what();
        if (consoleHandler_) {
            consoleHandler_("error", result.error);
        }
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
    if (vm_) {
        vm_->setGlobal(name, Zepra::Runtime::Value::string(
            new Zepra::Runtime::String(value)));
    }
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
    if (alertHandler_) alertHandler_(message);
    else std::cout << "[alert] " << message << std::endl;
}

bool ScriptContext::confirm(const std::string& message) {
    if (confirmHandler_) return confirmHandler_(message);
    return true;
}

std::string ScriptContext::prompt(const std::string& message, const std::string& defaultValue) {
    if (promptHandler_) return promptHandler_(message, defaultValue);
    return defaultValue;
}

void ScriptContext::setupGlobals() {
    setupWindowGlobals();
    setupDocumentGlobals();
}

void ScriptContext::setupWindowGlobals() {
    if (!vm_) return;
    
    using namespace Zepra::Runtime;
    
    // window.alert(message)
    auto* alertFn = createNativeFunction("alert",
        [this](Context*, const std::vector<Value>& args) -> Value {
            std::string msg = args.empty() ? "" : args[0].toString();
            alert(msg);
            return Value::undefined();
        }, 1);
    vm_->setGlobal("alert", Value::object(alertFn));
    
    // window.confirm(message)
    auto* confirmFn = createNativeFunction("confirm",
        [this](Context*, const std::vector<Value>& args) -> Value {
            std::string msg = args.empty() ? "" : args[0].toString();
            return Value::boolean(confirm(msg));
        }, 1);
    vm_->setGlobal("confirm", Value::object(confirmFn));
    
    // window.prompt(message, default)
    auto* promptFn = createNativeFunction("prompt",
        [this](Context*, const std::vector<Value>& args) -> Value {
            std::string msg = args.empty() ? "" : args[0].toString();
            std::string def = args.size() > 1 ? args[1].toString() : "";
            return Value::string(new String(prompt(msg, def)));
        }, 2);
    vm_->setGlobal("prompt", Value::object(promptFn));
    
    // console.log — bridge to console handler
    auto handler = consoleHandler_;
    auto* logFn = createNativeFunction("log",
        [handler](Context*, const std::vector<Value>& args) -> Value {
            std::string msg;
            for (size_t i = 0; i < args.size(); i++) {
                if (i > 0) msg += " ";
                msg += args[i].toString();
            }
            if (handler) handler("log", msg);
            else std::cout << "[console.log] " << msg << std::endl;
            return Value::undefined();
        }, 0);  // variadic
    
    auto* consoleObj = new Object();
    consoleObj->set("log", Value::object(logFn));
    
    auto* warnFn = createNativeFunction("warn",
        [handler](Context*, const std::vector<Value>& args) -> Value {
            std::string msg;
            for (auto& a : args) msg += a.toString() + " ";
            if (handler) handler("warn", msg);
            else std::cerr << "[console.warn] " << msg << std::endl;
            return Value::undefined();
        }, 0);
    consoleObj->set("warn", Value::object(warnFn));
    
    auto* errorFn = createNativeFunction("error",
        [handler](Context*, const std::vector<Value>& args) -> Value {
            std::string msg;
            for (auto& a : args) msg += a.toString() + " ";
            if (handler) handler("error", msg);
            else std::cerr << "[console.error] " << msg << std::endl;
            return Value::undefined();
        }, 0);
    consoleObj->set("error", Value::object(errorFn));
    vm_->setGlobal("console", Value::object(consoleObj));
    
    // setTimeout(callback, delay)
    auto* setTimeoutFn = createNativeFunction("setTimeout",
        [this](Context*, const std::vector<Value>& args) -> Value {
            // For now, just register as timer with delay
            int delay = args.size() > 1 ? (int)args[1].toNumber() : 0;
            int id = setTimeout([]{}, delay);
            return Value::number(id);
        }, 2);
    vm_->setGlobal("setTimeout", Value::object(setTimeoutFn));
    
    // setInterval(callback, interval)
    auto* setIntervalFn = createNativeFunction("setInterval",
        [this](Context*, const std::vector<Value>& args) -> Value {
            int interval = args.size() > 1 ? (int)args[1].toNumber() : 0;
            int id = setInterval([]{}, interval);
            return Value::number(id);
        }, 2);
    vm_->setGlobal("setInterval", Value::object(setIntervalFn));
    
    // clearTimeout / clearInterval
    auto* clearTimeoutFn = createNativeFunction("clearTimeout",
        [this](Context*, const std::vector<Value>& args) -> Value {
            if (!args.empty()) clearTimeout((int)args[0].toNumber());
            return Value::undefined();
        }, 1);
    vm_->setGlobal("clearTimeout", Value::object(clearTimeoutFn));
    vm_->setGlobal("clearInterval", Value::object(clearTimeoutFn));
    
    // navigator.userAgent
    auto* navObj = new Object();
    navObj->set("userAgent", Value::string(new String("ZepraBrowser/1.0")));
    navObj->set("platform", Value::string(new String("Linux")));
    navObj->set("language", Value::string(new String("en-US")));
    vm_->setGlobal("navigator", Value::object(navObj));
    
    // location object
    auto* locObj = new Object();
    locObj->set("href", Value::string(new String("")));
    locObj->set("hostname", Value::string(new String("")));
    locObj->set("pathname", Value::string(new String("/")));
    locObj->set("protocol", Value::string(new String("https:")));
    vm_->setGlobal("location", Value::object(locObj));
    
    // window dimensions
    vm_->setGlobal("innerWidth", Value::number(1280));
    vm_->setGlobal("innerHeight", Value::number(720));
    
    std::cout << "[ScriptContext] Window globals registered (alert, console, setTimeout, navigator, location)" << std::endl;
}

void ScriptContext::setupDocumentGlobals() {
    if (!vm_ || !document_) return;
    
    using namespace Zepra::Runtime;
    
    // Helper: wrap a DOMElement* into a JS Object with properties and methods
    auto wrapElement = [this](DOMElement* elem) -> Value {
        if (!elem) return Value::null();
        
        auto* obj = new Object();
        
        // Properties
        obj->set("tagName", Value::string(new String(elem->tagName())));
        obj->set("id", Value::string(new String(elem->id())));
        obj->set("className", Value::string(new String(elem->className())));
        obj->set("textContent", Value::string(new String(elem->textContent())));
        obj->set("nodeName", Value::string(new String(elem->nodeName())));
        
        // innerHTML (approximation: textContent)
        obj->set("innerHTML", Value::string(new String(elem->textContent())));
        
        // getAttribute(name)
        auto* getAttrFn = createNativeFunction("getAttribute",
            [elem](Context*, const std::vector<Value>& args) -> Value {
                if (args.empty()) return Value::undefined();
                std::string name = args[0].toString();
                return Value::string(new String(elem->getAttribute(name)));
            }, 1);
        obj->set("getAttribute", Value::object(getAttrFn));
        
        // setAttribute(name, value)
        auto* setAttrFn = createNativeFunction("setAttribute",
            [elem](Context*, const std::vector<Value>& args) -> Value {
                if (args.size() < 2) return Value::undefined();
                elem->setAttribute(args[0].toString(), args[1].toString());
                return Value::undefined();
            }, 2);
        obj->set("setAttribute", Value::object(setAttrFn));
        
        // style object (property bag — sets CSS inline styles)
        auto* styleObj = new Object();
        obj->set("style", Value::object(styleObj));
        
        // addEventListener (stored on the EventTarget)
        auto* addEventFn = createNativeFunction("addEventListener",
            [](Context*, const std::vector<Value>&) -> Value {
                // Event binding requires callback storage — placeholder for now
                return Value::undefined();
            }, 2);
        obj->set("addEventListener", Value::object(addEventFn));
        
        // children array
        auto* childrenArray = new Object(ObjectType::Array);
        int idx = 0;
        for (const auto& childNode : elem->childNodes()) {
            if (auto* childElem = dynamic_cast<DOMElement*>(childNode.get())) {
                auto* childObj = new Object();
                childObj->set("tagName", Value::string(new String(childElem->tagName())));
                childObj->set("id", Value::string(new String(childElem->id())));
                childObj->set("textContent", Value::string(new String(childElem->textContent())));
                childrenArray->set(idx++, Value::object(childObj));
            }
        }
        obj->set("children", Value::object(childrenArray));
        
        // querySelector on element
        auto wrapElem = [this](DOMElement* e) -> Value {
            if (!e) return Value::null();
            auto* o = new Object();
            o->set("tagName", Value::string(new String(e->tagName())));
            o->set("id", Value::string(new String(e->id())));
            o->set("className", Value::string(new String(e->className())));
            o->set("textContent", Value::string(new String(e->textContent())));
            return Value::object(o);
        };
        
        auto* qsFn = createNativeFunction("querySelector",
            [elem, wrapElem](Context*, const std::vector<Value>& args) -> Value {
                if (args.empty()) return Value::null();
                return wrapElem(elem->querySelector(args[0].toString()));
            }, 1);
        obj->set("querySelector", Value::object(qsFn));
        
        return Value::object(obj);
    };
    
    // Create 'document' object
    auto* docObj = new Object();
    DOMDocument* doc = document_;
    
    // document.getElementById(id)
    auto* getById = createNativeFunction("getElementById",
        [doc, wrapElement](Context*, const std::vector<Value>& args) -> Value {
            if (args.empty()) return Value::null();
            return wrapElement(doc->getElementById(args[0].toString()));
        }, 1);
    docObj->set("getElementById", Value::object(getById));
    
    // document.querySelector(selector)
    auto* querySelector = createNativeFunction("querySelector",
        [doc, wrapElement](Context*, const std::vector<Value>& args) -> Value {
            if (args.empty()) return Value::null();
            return wrapElement(doc->querySelector(args[0].toString()));
        }, 1);
    docObj->set("querySelector", Value::object(querySelector));
    
    // document.querySelectorAll(selector) → returns array
    auto* querySelectorAll = createNativeFunction("querySelectorAll",
        [doc, wrapElement](Context*, const std::vector<Value>& args) -> Value {
            if (args.empty()) return Value::object(new Object(ObjectType::Array));
            auto elems = doc->querySelectorAll(args[0].toString());
            auto* arr = new Object(ObjectType::Array);
            for (size_t i = 0; i < elems.size(); i++) {
                arr->set(i, wrapElement(elems[i]));
            }
            return Value::object(arr);
        }, 1);
    docObj->set("querySelectorAll", Value::object(querySelectorAll));
    
    // document.createElement(tagName)
    auto* createElement = createNativeFunction("createElement",
        [doc](Context*, const std::vector<Value>& args) -> Value {
            if (args.empty()) return Value::null();
            auto elem = doc->createElement(args[0].toString());
            if (!elem) return Value::null();
            auto* obj = new Object();
            obj->set("tagName", Value::string(new String(elem->tagName())));
            return Value::object(obj);
        }, 1);
    docObj->set("createElement", Value::object(createElement));
    
    // document.createTextNode(data)
    auto* createTextNode = createNativeFunction("createTextNode",
        [doc](Context*, const std::vector<Value>& args) -> Value {
            if (args.empty()) return Value::null();
            auto* obj = new Object();
            obj->set("nodeType", Value::number(3));
            obj->set("textContent", Value::string(new String(args[0].toString())));
            return Value::object(obj);
        }, 1);
    docObj->set("createTextNode", Value::object(createTextNode));
    
    // document.body
    DOMElement* bodyElem = doc->body();
    docObj->set("body", wrapElement(bodyElem));
    
    // document.documentElement
    DOMElement* htmlElem = doc->documentElement();
    docObj->set("documentElement", wrapElement(htmlElem));
    
    // document.head
    DOMElement* headElem = doc->head();
    docObj->set("head", wrapElement(headElem));
    
    // document.title
    docObj->set("title", Value::string(new String(doc->title())));
    
    // document.addEventListener
    auto* docAddEvent = createNativeFunction("addEventListener",
        [this](Context*, const std::vector<Value>& args) -> Value {
            if (args.size() < 2) return Value::undefined();
            std::string eventType = args[0].toString();
            addEventListener(eventType, []{});
            return Value::undefined();
        }, 2);
    docObj->set("addEventListener", Value::object(docAddEvent));
    
    // Register document global
    vm_->setGlobal("document", Value::object(docObj));
    
    std::cout << "[ScriptContext] DOM API registered (document.getElementById, querySelector, createElement, body, title)" << std::endl;
}

// ============================================================================
// DevToolsConsole
// ============================================================================

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

// ============================================================================
// DevToolsPanel
// ============================================================================

DevToolsPanel::DevToolsPanel() {}

} // namespace Zepra::WebCore
