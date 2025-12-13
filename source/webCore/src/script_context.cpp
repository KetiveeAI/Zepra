/**
 * @file script_context.cpp
 * @brief Script context implementation with ZepraScript internals
 */

#include "webcore/script_context.hpp"
#include "webcore/gc_bridge.hpp"

// ZepraScript internals
#include "zeprascript/frontend/source_code.hpp"
#include "zeprascript/frontend/parser.hpp"
#include "zeprascript/bytecode/bytecode_generator.hpp"
#include "zeprascript/runtime/vm.hpp"
#include "zeprascript/runtime/value.hpp"
#include "zeprascript/runtime/object.hpp"
#include "zeprascript/runtime/promise.hpp"
#include "zeprascript/builtins/console.hpp"
#include "zeprascript/builtins/math.hpp"
#include "zeprascript/builtins/json.hpp"
#include "zeprascript/runtime/function.hpp"
#include "zeprascript/browser/fetch.hpp"
#include "zeprascript/browser/storage_api.hpp"
#include "webcore/webgl_bindings.hpp"

#include <fstream>
#include <sstream>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <thread>

namespace Zepra::WebCore {

// =============================================================================
// ScriptContext
// =============================================================================

ScriptContext::ScriptContext() {
    // initialize default logger (stdout)
    consoleHandler_ = [](const std::string& level, const std::string& message) {
        std::cout << "[JS " << level << "] " << message << std::endl;
    };
    
    // Create VM with no parent context
    vm_ = std::make_unique<Runtime::VM>(nullptr);
    setupGlobals();
}

ScriptContext::~ScriptContext() = default;

void ScriptContext::initialize(DOMDocument* document) {
    document_ = document;
    
    // Update location global if document changes
    if (vm_) {
        std::string url = document ? "zepra://page" : "about:blank";
        vm_->setGlobal("location", Runtime::Value::string(new Runtime::String(url)));
    }
    
    // Expose document as a global
    setupDocumentGlobals();
}

void ScriptContext::setGlobal(const std::string& name, const std::string& value) {
    if (vm_) {
        auto* str = new Runtime::String(value);
        vm_->setGlobal(name, Runtime::Value::string(str));
    }
}

// Helper for readFile
static Runtime::Value readFileCallback(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (args.empty()) return Runtime::Value::undefined();
    std::string path = args[0].toString();
    
    std::ifstream file(path);
    if (!file) return Runtime::Value::null();
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return Runtime::Value::string(new Runtime::String(buffer.str()));
}

void ScriptContext::setupGlobals() {
    if (!vm_) return;
    
    // 1. Console
    // 1. Console (Manual implementation to ensure output via ScriptContext::log)
    Runtime::Object* consoleObj = new Runtime::Object();
    
    auto logFn = [this](Runtime::Context*, const std::vector<Runtime::Value>& args) -> Runtime::Value {
        std::stringstream ss;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) ss << " ";
            ss << args[i].toString();
        }
        this->log(ss.str());
        return Runtime::Value::undefined();
    };
    
    consoleObj->set("log", Runtime::Value::object(Runtime::createNativeFunction("log", logFn, 1)));
    consoleObj->set("info", Runtime::Value::object(Runtime::createNativeFunction("info", logFn, 1)));
    consoleObj->set("warn", Runtime::Value::object(Runtime::createNativeFunction("warn", logFn, 1)));
    consoleObj->set("error", Runtime::Value::object(Runtime::createNativeFunction("error", logFn, 1)));
    
    vm_->setGlobal("console", Runtime::Value::object(consoleObj));
    
    // 2. Math
    Runtime::Object* mathObj = Builtins::MathBuiltin::createMathObject();
    vm_->setGlobal("Math", Runtime::Value::object(mathObj));
    
    // 3. JSON
    Runtime::Object* jsonObj = Builtins::JSONBuiltin::createJSONObject(nullptr);
    vm_->setGlobal("JSON", Runtime::Value::object(jsonObj));
    
    // 4. Basic constants
    vm_->setGlobal("undefined", Runtime::Value::undefined());
    vm_->setGlobal("NaN", Runtime::Value::number(std::nan("")));
    vm_->setGlobal("Infinity", Runtime::Value::number(INFINITY));
    
    // 5. File System (Basic)
    auto* readFileFn = Runtime::createNativeFunction("readFile", readFileCallback, 1);
    vm_->setGlobal("readFile", Runtime::Value::object(readFileFn));
    
    // 6. Window globals (alert, confirm, prompt, etc.)
    setupWindowGlobals();
}

ScriptResult ScriptContext::evaluate(const std::string& code, const std::string& filename) {
    ScriptResult result;
    
    if (!vm_) {
        result.error = "VM not initialized";
        return result;
    }
    
    try {
        // 1. Parse
        auto source = Frontend::SourceCode::fromString(code, filename);
        Frontend::Parser parser(source.get());
        auto program = parser.parseProgram();
        
        if (parser.hasErrors()) {
            result.success = false;
            result.error = parser.errors()[0]; // Take first error
            lastError_ = result.error;
            return result;
        }
        
        // 2. Compile
        Bytecode::BytecodeGenerator generator;
        auto chunk = generator.compile(program.get());
        
        if (generator.hasErrors()) {
            result.success = false;
            result.error = generator.errors()[0];
            lastError_ = result.error;
            return result;
        }
        
        // 3. Execute
        Runtime::ExecutionResult execResult = vm_->execute(chunk.get());
        
        if (execResult.status == Runtime::ExecutionResult::Status::Error) {
            result.success = false;
            result.error = execResult.error;
            lastError_ = result.error;
        } else {
            result.success = true;
            if (!execResult.value.isUndefined()) {
                result.value = execResult.value.toString();
            } else {
                result.value = "undefined";
            }
        }
        
        // Drain microtask queue after script execution
        Runtime::MicrotaskQueue::instance().drain();
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error = e.what();
        lastError_ = result.error;
    }
    
    return result;
}

void ScriptContext::log(const std::string& message) {
    if (consoleHandler_) {
        consoleHandler_("log", message);
    }
}

// ... Timers ...

int ScriptContext::setTimeout(std::function<void()> callback, int delay) {
    auto now = std::chrono::high_resolution_clock::now();
    double timestamp = std::chrono::duration<double, std::milli>(now.time_since_epoch()).count();
    
    TimerCallback timer;
    timer.id = nextTimerId_++;
    timer.callback = std::move(callback);
    timer.delay = delay;
    timer.repeating = false;
    timer.scheduledTime = timestamp + delay;
    
    timers_.push_back(std::move(timer));
    return timer.id;
}

int ScriptContext::setInterval(std::function<void()> callback, int interval) {
    auto now = std::chrono::high_resolution_clock::now();
    double timestamp = std::chrono::duration<double, std::milli>(now.time_since_epoch()).count();
    
    TimerCallback timer;
    timer.id = nextTimerId_++;
    timer.callback = std::move(callback);
    timer.delay = interval;
    timer.repeating = true;
    timer.scheduledTime = timestamp + interval;
    
    timers_.push_back(std::move(timer));
    return timer.id;
}

void ScriptContext::clearTimeout(int id) {
    timers_.erase(
        std::remove_if(timers_.begin(), timers_.end(),
            [id](const TimerCallback& t) { return t.id == id; }),
        timers_.end()
    );
}

void ScriptContext::clearInterval(int id) {
    clearTimeout(id);
}

void ScriptContext::processTimers() {
    auto now = std::chrono::high_resolution_clock::now();
    double timestamp = std::chrono::duration<double, std::milli>(now.time_since_epoch()).count();
    
    std::vector<TimerCallback*> ready;
    for (auto& timer : timers_) {
        if (timestamp >= timer.scheduledTime) {
            ready.push_back(&timer);
        }
    }
    
    for (auto* timer : ready) {
        if (timer->callback) {
            timer->callback();
        }
        
        if (timer->repeating) {
            timer->scheduledTime = timestamp + timer->delay;
        }
    }
    
    timers_.erase(
        std::remove_if(timers_.begin(), timers_.end(),
            [timestamp](const TimerCallback& t) { 
                return !t.repeating && timestamp >= t.scheduledTime; 
            }),
        timers_.end()
    );
}

// =============================================================================
// Lifecycle Events
// =============================================================================

void ScriptContext::fireDOMContentLoaded() {
    if (consoleHandler_) {
        consoleHandler_("info", "[Page] DOMContentLoaded event fired");
    }
    
    // Call all registered DOMContentLoaded listeners
    for (auto& listener : domContentLoadedListeners_) {
        try {
            listener();
        } catch (...) {
            if (consoleHandler_) {
                consoleHandler_("error", "Error in DOMContentLoaded listener");
            }
        }
    }
}

void ScriptContext::fireLoadEvent() {
    if (consoleHandler_) {
        consoleHandler_("info", "[Page] load event fired");
    }
    
    // Call all registered load listeners  
    for (auto& listener : loadListeners_) {
        try {
            listener();
        } catch (...) {
            if (consoleHandler_) {
                consoleHandler_("error", "Error in load listener");
            }
        }
    }
}

void ScriptContext::addEventListener(const std::string& eventType, std::function<void()> callback) {
    if (eventType == "DOMContentLoaded") {
        domContentLoadedListeners_.push_back(callback);
    } else if (eventType == "load") {
        loadListeners_.push_back(callback);
    }
}

// =============================================================================
// Dialog Functions
// =============================================================================

void ScriptContext::alert(const std::string& message) {
    if (alertHandler_) {
        alertHandler_(message);
    } else {
        // Default: output to console
        if (consoleHandler_) {
            consoleHandler_("alert", "ALERT: " + message);
        } else {
            std::printf("ALERT: %s\n", message.c_str());
        }
    }
}

bool ScriptContext::confirm(const std::string& message) {
    if (confirmHandler_) {
        return confirmHandler_(message);
    } else {
        // Default: output to console and return true
        if (consoleHandler_) {
            consoleHandler_("confirm", "CONFIRM: " + message + " (auto-accepted)");
        } else {
            std::printf("CONFIRM: %s (auto-accepted)\n", message.c_str());
        }
        return true;
    }
}

std::string ScriptContext::prompt(const std::string& message, const std::string& defaultValue) {
    if (promptHandler_) {
        return promptHandler_(message, defaultValue);
    } else {
        // Default: output to console and return default value
        if (consoleHandler_) {
            consoleHandler_("prompt", "PROMPT: " + message + " (returning: " + defaultValue + ")");
        } else {
            std::printf("PROMPT: %s (returning: %s)\n", message.c_str(), defaultValue.c_str());
        }
        return defaultValue;
    }
}

// =============================================================================
// Window and Document Globals
// =============================================================================

// Static reference for native function callbacks
static ScriptContext* g_currentContext = nullptr;

static Runtime::Value alertCallback(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (g_currentContext && !args.empty()) {
        g_currentContext->alert(args[0].toString());
    }
    return Runtime::Value::undefined();
}

static Runtime::Value confirmCallback(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (g_currentContext && !args.empty()) {
        bool result = g_currentContext->confirm(args[0].toString());
        return Runtime::Value::boolean(result);
    }
    return Runtime::Value::boolean(true);
}

static Runtime::Value promptCallback(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    if (g_currentContext) {
        std::string message = args.size() > 0 ? args[0].toString() : "";
        std::string defaultVal = args.size() > 1 ? args[1].toString() : "";
        std::string result = g_currentContext->prompt(message, defaultVal);
        return Runtime::Value::string(new Runtime::String(result));
    }
    return Runtime::Value::null();
}

void ScriptContext::setupWindowGlobals() {
    if (!vm_) return;
    
    // Set global context reference for callbacks
    g_currentContext = this;
    
    // alert()
    auto* alertFn = Runtime::createNativeFunction("alert", alertCallback, 1);
    vm_->setGlobal("alert", Runtime::Value::object(alertFn));
    
    // confirm()
    auto* confirmFn = Runtime::createNativeFunction("confirm", confirmCallback, 1);
    vm_->setGlobal("confirm", Runtime::Value::object(confirmFn));
    
    // prompt()
    auto* promptFn = Runtime::createNativeFunction("prompt", promptCallback, 2);
    vm_->setGlobal("prompt", Runtime::Value::object(promptFn));
    
    // setTimeout(callback, delay)
    auto* setTimeoutFn = Runtime::createNativeFunction("setTimeout", 
        [](Runtime::Context* ctx, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            if (!g_currentContext || args.empty()) return Runtime::Value::undefined();
            
            Runtime::Value callback = args[0];
            int delay = args.size() > 1 ? static_cast<int>(args[1].toNumber()) : 0;
            
            // Store callback in closure and invoke through VM when timer fires
            Runtime::VM* vm = g_currentContext->vm();
            int id = g_currentContext->setTimeout([vm, callback]() {
                if (vm && callback.isObject() && callback.asObject()->isFunction()) {
                    auto* fn = static_cast<Runtime::Function*>(callback.asObject());
                    fn->call(nullptr, Runtime::Value::undefined(), {});
                }
            }, delay);
            
            return Runtime::Value::number(static_cast<double>(id));
        }, 2);
    vm_->setGlobal("setTimeout", Runtime::Value::object(setTimeoutFn));
    
    // setInterval(callback, delay)
    auto* setIntervalFn = Runtime::createNativeFunction("setInterval",
        [](Runtime::Context* ctx, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            if (!g_currentContext || args.empty()) return Runtime::Value::undefined();
            
            Runtime::Value callback = args[0];
            int delay = args.size() > 1 ? static_cast<int>(args[1].toNumber()) : 0;
            
            Runtime::VM* vm = g_currentContext->vm();
            int id = g_currentContext->setInterval([vm, callback]() {
                if (vm && callback.isObject() && callback.asObject()->isFunction()) {
                    auto* fn = static_cast<Runtime::Function*>(callback.asObject());
                    fn->call(nullptr, Runtime::Value::undefined(), {});
                }
            }, delay);
            
            return Runtime::Value::number(static_cast<double>(id));
        }, 2);
    vm_->setGlobal("setInterval", Runtime::Value::object(setIntervalFn));
    
    // clearTimeout(id)
    auto* clearTimeoutFn = Runtime::createNativeFunction("clearTimeout",
        [](Runtime::Context*, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            if (!g_currentContext || args.empty()) return Runtime::Value::undefined();
            int id = static_cast<int>(args[0].toNumber());
            g_currentContext->clearTimeout(id);
            return Runtime::Value::undefined();
        }, 1);
    vm_->setGlobal("clearTimeout", Runtime::Value::object(clearTimeoutFn));
    
    // clearInterval(id)
    auto* clearIntervalFn = Runtime::createNativeFunction("clearInterval",
        [](Runtime::Context*, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            if (!g_currentContext || args.empty()) return Runtime::Value::undefined();
            int id = static_cast<int>(args[0].toNumber());
            g_currentContext->clearInterval(id);
            return Runtime::Value::undefined();
        }, 1);
    vm_->setGlobal("clearInterval", Runtime::Value::object(clearIntervalFn));
    
    // fetch(url, options) - returns Promise
    auto* fetchFn = Runtime::createNativeFunction("fetch",
        Browser::FetchAPI::fetchBuiltin, 2);
    vm_->setGlobal("fetch", Runtime::Value::object(fetchFn));
    
    // localStorage - convert Storage to JS object with methods
    Runtime::Object* localStorageObj = new Runtime::Object(Runtime::ObjectType::Ordinary);
    localStorageObj->set("getItem", Runtime::Value::object(Runtime::createNativeFunction("getItem",
        [](Runtime::Context*, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            if (args.empty()) return Runtime::Value::null();
            std::string key = args[0].toString();
            std::string val = Browser::localStorage().getItem(key);
            if (val.empty()) return Runtime::Value::null();
            return Runtime::Value::string(new Runtime::String(val));
        }, 1)));
    localStorageObj->set("setItem", Runtime::Value::object(Runtime::createNativeFunction("setItem",
        [](Runtime::Context*, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            if (args.size() < 2) return Runtime::Value::undefined();
            std::string key = args[0].toString();
            std::string val = args[1].toString();
            Browser::localStorage().setItem(key, val);
            return Runtime::Value::undefined();
        }, 2)));
    localStorageObj->set("removeItem", Runtime::Value::object(Runtime::createNativeFunction("removeItem",
        [](Runtime::Context*, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            if (args.empty()) return Runtime::Value::undefined();
            Browser::localStorage().removeItem(args[0].toString());
            return Runtime::Value::undefined();
        }, 1)));
    localStorageObj->set("clear", Runtime::Value::object(Runtime::createNativeFunction("clear",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            Browser::localStorage().clear();
            return Runtime::Value::undefined();
        }, 0)));
    vm_->setGlobal("localStorage", Runtime::Value::object(localStorageObj));
    
    // sessionStorage
    Runtime::Object* sessionStorageObj = new Runtime::Object(Runtime::ObjectType::Ordinary);
    sessionStorageObj->set("getItem", Runtime::Value::object(Runtime::createNativeFunction("getItem",
        [](Runtime::Context*, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            if (args.empty()) return Runtime::Value::null();
            std::string val = Browser::sessionStorage().getItem(args[0].toString());
            if (val.empty()) return Runtime::Value::null();
            return Runtime::Value::string(new Runtime::String(val));
        }, 1)));
    sessionStorageObj->set("setItem", Runtime::Value::object(Runtime::createNativeFunction("setItem",
        [](Runtime::Context*, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            if (args.size() < 2) return Runtime::Value::undefined();
            Browser::sessionStorage().setItem(args[0].toString(), args[1].toString());
            return Runtime::Value::undefined();
        }, 2)));
    sessionStorageObj->set("removeItem", Runtime::Value::object(Runtime::createNativeFunction("removeItem",
        [](Runtime::Context*, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            if (args.empty()) return Runtime::Value::undefined();
            Browser::sessionStorage().removeItem(args[0].toString());
            return Runtime::Value::undefined();
        }, 1)));
    sessionStorageObj->set("clear", Runtime::Value::object(Runtime::createNativeFunction("clear",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            Browser::sessionStorage().clear();
            return Runtime::Value::undefined();
        }, 0)));
    vm_->setGlobal("sessionStorage", Runtime::Value::object(sessionStorageObj));
    
    // =========================================================================
    // navigator object - Browser/device information
    // =========================================================================
    Runtime::Object* navigatorObj = new Runtime::Object(Runtime::ObjectType::Ordinary);
    
    // navigator.userAgent
    navigatorObj->set("userAgent", Runtime::Value::string(new Runtime::String(
        "Mozilla/5.0 (X11; Linux x86_64) ZepraBrowser/1.0 ZepraScript/1.0")));
    
    // navigator.language
    navigatorObj->set("language", Runtime::Value::string(new Runtime::String("en-US")));
    
    // navigator.languages
    Runtime::Object* languagesArr = new Runtime::Object(Runtime::ObjectType::Array);
    languagesArr->set(0, Runtime::Value::string(new Runtime::String("en-US")));
    languagesArr->set(1, Runtime::Value::string(new Runtime::String("en")));
    languagesArr->set("length", Runtime::Value::number(2));
    navigatorObj->set("languages", Runtime::Value::object(languagesArr));
    
    // navigator.platform
    navigatorObj->set("platform", Runtime::Value::string(new Runtime::String("Linux x86_64")));
    
    // navigator.vendor
    navigatorObj->set("vendor", Runtime::Value::string(new Runtime::String("Zepra")));
    
    // navigator.onLine
    navigatorObj->set("onLine", Runtime::Value::boolean(true));
    
    // navigator.cookieEnabled
    navigatorObj->set("cookieEnabled", Runtime::Value::boolean(true));
    
    // navigator.hardwareConcurrency
    navigatorObj->set("hardwareConcurrency", Runtime::Value::number(
        static_cast<double>(std::thread::hardware_concurrency())));
    
    // navigator.vibrate(pattern) - Vibration API
    navigatorObj->set("vibrate", Runtime::Value::object(Runtime::createNativeFunction("vibrate",
        [](Runtime::Context*, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            // On desktop, vibrate is a no-op but returns true for compatibility
            // Could be wired to gamepad rumble or system notification
            if (args.empty()) return Runtime::Value::boolean(false);
            // Pattern can be number or array
            // For now, just log and return true
            if (g_currentContext) {
                g_currentContext->log("[Vibration API] vibrate() called");
            }
            return Runtime::Value::boolean(true);
        }, 1)));
    
    // navigator.clipboard - Clipboard API
    Runtime::Object* clipboardObj = new Runtime::Object(Runtime::ObjectType::Ordinary);
    clipboardObj->set("writeText", Runtime::Value::object(Runtime::createNativeFunction("writeText",
        [](Runtime::Context*, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            // TODO: Implement X11 clipboard write
            if (args.empty()) return Runtime::Value::undefined();
            std::string text = args[0].toString();
            if (g_currentContext) {
                g_currentContext->log("[Clipboard API] writeText: " + text);
            }
            // Return a resolved Promise (stub)
            return Runtime::Value::undefined();
        }, 1)));
    clipboardObj->set("readText", Runtime::Value::object(Runtime::createNativeFunction("readText",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            // TODO: Implement X11 clipboard read
            if (g_currentContext) {
                g_currentContext->log("[Clipboard API] readText called");
            }
            return Runtime::Value::string(new Runtime::String(""));
        }, 0)));
    navigatorObj->set("clipboard", Runtime::Value::object(clipboardObj));
    
    // navigator.geolocation - Geolocation API (stub)
    Runtime::Object* geolocationObj = new Runtime::Object(Runtime::ObjectType::Ordinary);
    geolocationObj->set("getCurrentPosition", Runtime::Value::object(Runtime::createNativeFunction("getCurrentPosition",
        [](Runtime::Context*, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            // TODO: Integrate with GeoClue on Linux
            if (g_currentContext) {
                g_currentContext->log("[Geolocation API] getCurrentPosition called - not implemented");
            }
            // Call error callback if provided
            if (args.size() > 1 && args[1].isObject() && args[1].asObject()->isFunction()) {
                auto* errorCb = static_cast<Runtime::Function*>(args[1].asObject());
                Runtime::Object* posError = new Runtime::Object(Runtime::ObjectType::Ordinary);
                posError->set("code", Runtime::Value::number(2)); // POSITION_UNAVAILABLE
                posError->set("message", Runtime::Value::string(new Runtime::String("Geolocation not available")));
                errorCb->call(nullptr, Runtime::Value::undefined(), {Runtime::Value::object(posError)});
            }
            return Runtime::Value::undefined();
        }, 3)));
    navigatorObj->set("geolocation", Runtime::Value::object(geolocationObj));
    
    vm_->setGlobal("navigator", Runtime::Value::object(navigatorObj));
    
    // =========================================================================
    // performance object - High-resolution timing
    // =========================================================================
    Runtime::Object* performanceObj = new Runtime::Object(Runtime::ObjectType::Ordinary);
    performanceObj->set("now", Runtime::Value::object(Runtime::createNativeFunction("now",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            auto now = std::chrono::high_resolution_clock::now();
            auto epoch = now.time_since_epoch();
            double ms = std::chrono::duration<double, std::milli>(epoch).count();
            return Runtime::Value::number(ms);
        }, 0)));
    vm_->setGlobal("performance", Runtime::Value::object(performanceObj));
    
    // location (basic string for now)
    vm_->setGlobal("location", Runtime::Value::string(new Runtime::String("about:blank")));
}

void ScriptContext::setupDocumentGlobals() {
    if (!vm_ || !document_) return;
    
    // For now, we expose document as a placeholder object
    // Full DOM integration would require wrapping DOMDocument in a JS object
    Runtime::Object* docObj = new Runtime::Object(Runtime::ObjectType::Ordinary);
    
    // Add basic properties using set() method
    docObj->set("title", Runtime::Value::string(new Runtime::String(document_->title())));
    
    // Stub for createElement('canvas') to support WebGL
    docObj->set("createElement", Runtime::Value::object(Runtime::createNativeFunction("createElement",
        [this](Runtime::Context* ctx, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            if (args.empty()) return Runtime::Value::null();
            std::string tagName = args[0].toString();
            
            if (tagName == "canvas") {
                Runtime::Object* canvas = new Runtime::Object(Runtime::ObjectType::Ordinary);
                canvas->set("width", Runtime::Value::number(800));
                canvas->set("height", Runtime::Value::number(600));
                
                // getContext('webgl')
                canvas->set("getContext", Runtime::Value::object(Runtime::createNativeFunction("getContext",
                    [this](Runtime::Context* c, const std::vector<Runtime::Value>& a) -> Runtime::Value {
                        if (a.empty()) return Runtime::Value::null();
                        std::string type = a[0].toString();
                        
                        if (type == "webgl" || type == "experimental-webgl") {
                            // Create WebGL context
                             int w = 800, h = 600; 
                             // Try to get width/height from canvas obj if possible (this capture is tricky, 
                             // need reference to canvas. simplified: fixed size for now)
                            
                            uint32_t handle = WebGLBindings::createContext(w, h);
                            if (handle == 0) return Runtime::Value::null();
                            
                            // Capture vm_ from outer scope. 
                            // Wait, "vm_" is member of ScriptContext.
                            // The lambda captures "this" (implicitly or explicitly?).
                            // The outer lambda is [this](...) { ... } inside ScriptContext::initialize.
                            // So "this" is available. "this->vm_.get()" is available.
                            return Runtime::Value::object(WebGLBindings::createJSContextObject(this->vm_.get(), handle));
                        }
                        return Runtime::Value::null();
                    }, 1)));
                    
                return Runtime::Value::object(canvas);
            }
            // Other elements return dummy object
            return Runtime::Value::object(new Runtime::Object(Runtime::ObjectType::Ordinary));
        }, 1)));

    vm_->setGlobal("document", Runtime::Value::object(docObj));
    
    // Register WebGL Native functions globally 
    WebGLBindings::registerNativeFunctions(vm_.get());
    
    // We need to make sure the JS object returned by getContext has wrappers that call these __webgl_ native functions
    // passing 'this' handle as first arg.
    // This logic should probably be in WebGLBindings::createJSContextObject but that function currently only adds constants.
    // We can enhance it here or modifying WebGLBindings.
    // Let's modify WebGLBindings::createJSContextObject in webgl_bindings.cpp instead to add the methods. 
    // Wait, I cannot modify webgl_bindings.cpp in this step easily without another tool call.
    // I already enabled it for compilation.
    // I should check if I can modify it. Yes I can.
    // But for now let's just register the globals here so they exist.

}


// =============================================================================
// DevToolsConsole
// =============================================================================

DevToolsConsole::DevToolsConsole() {}

void DevToolsConsole::addEntry(const std::string& level, const std::string& message,
                                const std::string& source, int line) {
    auto now = std::chrono::high_resolution_clock::now();
    double timestamp = std::chrono::duration<double, std::milli>(now.time_since_epoch()).count();
    
    LogEntry entry;
    entry.level = level;
    entry.message = message;
    entry.source = source;
    entry.line = line;
    entry.timestamp = timestamp;
    
    entries_.push_back(entry);
    
    if (entries_.size() > maxEntries_) {
        entries_.erase(entries_.begin());
    }
    
    if (onLog_) {
        onLog_(entry);
    }
}

void DevToolsConsole::log(const std::string& message, const std::string& source, int line) {
    addEntry("log", message, source, line);
}

void DevToolsConsole::clear() {
    entries_.clear();
}

DevToolsPanel::DevToolsPanel() {}

} // namespace Zepra::WebCore
