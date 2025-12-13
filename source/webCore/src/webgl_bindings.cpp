/**
 * @file webgl_bindings.cpp
 * @brief JavaScript bindings implementation for WebGL API
 */

#include "webcore/webgl_bindings.hpp"
#include <unordered_map>
#include <vector>

// Include full definitions for Types used in implementation
#include "zeprascript/zepra_api.hpp" // Defines Runtime::Context
#include "zeprascript/runtime/object.hpp" // Defines Runtime::Object, Runtime::String, Runtime::Function
#include "zeprascript/runtime/function.hpp" // Wraps up Function typedefs if needed
#include "zeprascript/runtime/value.hpp"
#include "zeprascript/runtime/vm.hpp"
#include <iostream>

namespace Zepra::WebCore {

// Static members
std::unordered_map<uint32_t, std::unique_ptr<WebGLRenderingContext>> WebGLBindings::contexts_;
uint32_t WebGLBindings::nextHandle_ = 1;

// =============================================================================
// WebGLBindings Implementation
// =============================================================================

uint32_t WebGLBindings::createContext(int width, int height) {
    auto context = std::make_unique<WebGLRenderingContext>();
    if (!context->initialize(width, height)) {
        std::cerr << "Failed to initialize WebGL context" << std::endl;
        return 0;
    }
    
    uint32_t handle = nextHandle_++;
    contexts_[handle] = std::move(context);
    return handle;
}

WebGLRenderingContext* WebGLBindings::getContext(uint32_t handle) {
    auto it = contexts_.find(handle);
    return it != contexts_.end() ? it->second.get() : nullptr;
}

void WebGLBindings::destroyContext(uint32_t handle) {
    contexts_.erase(handle);
}

Runtime::Object* WebGLBindings::createJSContextObject(Runtime::VM* vm, uint32_t handle) {
    if (!vm) return nullptr;
    
    auto* gl = new Runtime::Object();
    
    // Store the handle as internal property
    gl->set("_handle", Runtime::Value::number(static_cast<double>(handle)));
    
    // Add all WebGL constants
    using namespace WebGLConstants;
    
    // Clear bits
    gl->set("DEPTH_BUFFER_BIT", Runtime::Value::number(DEPTH_BUFFER_BIT));
    gl->set("STENCIL_BUFFER_BIT", Runtime::Value::number(STENCIL_BUFFER_BIT));
    gl->set("COLOR_BUFFER_BIT", Runtime::Value::number(COLOR_BUFFER_BIT));
    
    // Status
    gl->set("COMPILE_STATUS", Runtime::Value::number(COMPILE_STATUS));
    gl->set("LINK_STATUS", Runtime::Value::number(LINK_STATUS));
    
    // Errors
    gl->set("NO_ERROR", Runtime::Value::number(NO_ERROR));
    gl->set("INVALID_ENUM", Runtime::Value::number(INVALID_ENUM));
    gl->set("INVALID_VALUE", Runtime::Value::number(INVALID_VALUE));
    gl->set("INVALID_OPERATION", Runtime::Value::number(INVALID_OPERATION));
    
    // Shader Types
    gl->set("VERTEX_SHADER", Runtime::Value::number(VERTEX_SHADER));
    gl->set("FRAGMENT_SHADER", Runtime::Value::number(FRAGMENT_SHADER));
    
    // Datatypes
    gl->set("FLOAT", Runtime::Value::number(FLOAT));
    gl->set("UNSIGNED_BYTE", Runtime::Value::number(UNSIGNED_BYTE));
    gl->set("UNSIGNED_SHORT", Runtime::Value::number(UNSIGNED_SHORT));
    gl->set("UNSIGNED_INT", Runtime::Value::number(UNSIGNED_INT));
    gl->set("INT", Runtime::Value::number(INT));
    
    // Buffer Objects
    gl->set("ARRAY_BUFFER", Runtime::Value::number(ARRAY_BUFFER));
    gl->set("ELEMENT_ARRAY_BUFFER", Runtime::Value::number(ELEMENT_ARRAY_BUFFER));
    gl->set("STATIC_DRAW", Runtime::Value::number(STATIC_DRAW));
    gl->set("DYNAMIC_DRAW", Runtime::Value::number(DYNAMIC_DRAW));
    
    // Textures
    gl->set("TEXTURE_2D", Runtime::Value::number(TEXTURE_2D));
    gl->set("RGBA", Runtime::Value::number(RGBA));
    gl->set("RGB", Runtime::Value::number(RGB));
    gl->set("TEXTURE0", Runtime::Value::number(TEXTURE0));
    gl->set("TEXTURE1", Runtime::Value::number(TEXTURE1));
    gl->set("TEXTURE_MIN_FILTER", Runtime::Value::number(TEXTURE_MIN_FILTER));
    gl->set("TEXTURE_MAG_FILTER", Runtime::Value::number(TEXTURE_MAG_FILTER));
    gl->set("LINEAR", Runtime::Value::number(LINEAR));
    gl->set("NEAREST", Runtime::Value::number(NEAREST));
    gl->set("CLAMP_TO_EDGE", Runtime::Value::number(CLAMP_TO_EDGE));
    
    // Methods
    // We wrap native calls to prepend the handle
    // Capture 'vm' to access global functions since internal Context is opaque
    
    // Viewport and Clear
    gl->set("viewport", Runtime::Value::object(Runtime::createNativeFunction("viewport",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_viewport");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                // Call needs context, but we can pass nullptr if native function doesn't use it?
                // Or pass 'c' (opaque pointer). The call method signature requires Context*.
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 4)));

    gl->set("clearColor", Runtime::Value::object(Runtime::createNativeFunction("clearColor",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_clearColor");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 4)));

    gl->set("clear", Runtime::Value::object(Runtime::createNativeFunction("clear",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_clear");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 1)));
        
    // Capability enable/disable
    gl->set("enable", Runtime::Value::object(Runtime::createNativeFunction("enable",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_enable");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 1)));
        
    gl->set("disable", Runtime::Value::object(Runtime::createNativeFunction("disable",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_disable");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 1)));
        
    // Shaders
    gl->set("createShader", Runtime::Value::object(Runtime::createNativeFunction("createShader",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_createShader");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::null();
        }, 1)));
        
    gl->set("shaderSource", Runtime::Value::object(Runtime::createNativeFunction("shaderSource",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_shaderSource");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 2)));
        
    gl->set("compileShader", Runtime::Value::object(Runtime::createNativeFunction("compileShader",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_compileShader");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 1)));
        
    gl->set("getShaderParameter", Runtime::Value::object(Runtime::createNativeFunction("getShaderParameter",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_getShaderParameter");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 2)));
        
    gl->set("getShaderInfoLog", Runtime::Value::object(Runtime::createNativeFunction("getShaderInfoLog",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_getShaderInfoLog");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::string(new Runtime::String(""));
        }, 1)));
        
    // Programs
    gl->set("createProgram", Runtime::Value::object(Runtime::createNativeFunction("createProgram",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_createProgram");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::null();
        }, 0)));
        
    gl->set("attachShader", Runtime::Value::object(Runtime::createNativeFunction("attachShader",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_attachShader");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 2)));
        
    gl->set("linkProgram", Runtime::Value::object(Runtime::createNativeFunction("linkProgram",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_linkProgram");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 1)));
        
    gl->set("getProgramParameter", Runtime::Value::object(Runtime::createNativeFunction("getProgramParameter",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_getProgramParameter");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 2)));
        
    gl->set("useProgram", Runtime::Value::object(Runtime::createNativeFunction("useProgram",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_useProgram");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 1)));
        
    gl->set("getAttribLocation", Runtime::Value::object(Runtime::createNativeFunction("getAttribLocation",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_getAttribLocation");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::number(-1);
        }, 2)));
        
    gl->set("vertexAttribPointer", Runtime::Value::object(Runtime::createNativeFunction("vertexAttribPointer",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_vertexAttribPointer");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 6)));
        
    gl->set("enableVertexAttribArray", Runtime::Value::object(Runtime::createNativeFunction("enableVertexAttribArray",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_enableVertexAttribArray");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 1)));
        
    gl->set("getUniformLocation", Runtime::Value::object(Runtime::createNativeFunction("getUniformLocation",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_getUniformLocation");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::null();
        }, 2)));
        
    gl->set("uniform1f", Runtime::Value::object(Runtime::createNativeFunction("uniform1f",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_uniform1f");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 2)));
        
    gl->set("uniformMatrix4fv", Runtime::Value::object(Runtime::createNativeFunction("uniformMatrix4fv",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_uniformMatrix4fv");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 3)));
        
    gl->set("createBuffer", Runtime::Value::object(Runtime::createNativeFunction("createBuffer",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_createBuffer");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::null();
        }, 0)));
        
    gl->set("bindBuffer", Runtime::Value::object(Runtime::createNativeFunction("bindBuffer",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_bindBuffer");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 2)));
        
    gl->set("bufferData", Runtime::Value::object(Runtime::createNativeFunction("bufferData",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_bufferData");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 3)));
        
    gl->set("drawArrays", Runtime::Value::object(Runtime::createNativeFunction("drawArrays",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_drawArrays");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 3)));
        
    gl->set("drawElements", Runtime::Value::object(Runtime::createNativeFunction("drawElements",
        [handle, vm](Runtime::Context* c, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            auto native = vm->getGlobal("__webgl_drawElements");
            if (native.isObject() && native.asObject()->isFunction()) {
                std::vector<Runtime::Value> newArgs = {Runtime::Value::number(handle)};
                newArgs.insert(newArgs.end(), args.begin(), args.end());
                return static_cast<Runtime::Function*>(native.asObject())->call(c, Runtime::Value::undefined(), newArgs);
            }
            return Runtime::Value::undefined();
        }, 4)));

    return gl;
}

void WebGLBindings::registerNativeFunctions(Runtime::VM* vm) {
    if (!vm) return;
    
    using Runtime::Value;
    using Runtime::createNativeFunction;
    
    // Register all native WebGL functions
    vm->setGlobal("__webgl_viewport", Value::object(createNativeFunction("viewport", WebGLNative::viewport, 5)));
    vm->setGlobal("__webgl_clearColor", Value::object(createNativeFunction("clearColor", WebGLNative::clearColor, 5)));
    vm->setGlobal("__webgl_clear", Value::object(createNativeFunction("clear", WebGLNative::clear, 2)));
    vm->setGlobal("__webgl_enable", Value::object(createNativeFunction("enable", WebGLNative::enable, 2)));
    vm->setGlobal("__webgl_disable", Value::object(createNativeFunction("disable", WebGLNative::disable, 2)));
    vm->setGlobal("__webgl_blendFunc", Value::object(createNativeFunction("blendFunc", WebGLNative::blendFunc, 3)));
    vm->setGlobal("__webgl_depthFunc", Value::object(createNativeFunction("depthFunc", WebGLNative::depthFunc, 2)));
    
    vm->setGlobal("__webgl_createShader", Value::object(createNativeFunction("createShader", WebGLNative::createShader, 2)));
    vm->setGlobal("__webgl_shaderSource", Value::object(createNativeFunction("shaderSource", WebGLNative::shaderSource, 3)));
    vm->setGlobal("__webgl_compileShader", Value::object(createNativeFunction("compileShader", WebGLNative::compileShader, 2)));
    vm->setGlobal("__webgl_getShaderParameter", Value::object(createNativeFunction("getShaderParameter", WebGLNative::getShaderParameter, 2)));
    vm->setGlobal("__webgl_getShaderInfoLog", Value::object(createNativeFunction("getShaderInfoLog", WebGLNative::getShaderInfoLog, 2)));
    vm->setGlobal("__webgl_deleteShader", Value::object(createNativeFunction("deleteShader", WebGLNative::deleteShader, 2)));
    
    vm->setGlobal("__webgl_createProgram", Value::object(createNativeFunction("createProgram", WebGLNative::createProgram, 1)));
    vm->setGlobal("__webgl_attachShader", Value::object(createNativeFunction("attachShader", WebGLNative::attachShader, 3)));
    vm->setGlobal("__webgl_linkProgram", Value::object(createNativeFunction("linkProgram", WebGLNative::linkProgram, 2)));
    vm->setGlobal("__webgl_getProgramParameter", Value::object(createNativeFunction("getProgramParameter", WebGLNative::getProgramParameter, 2)));
    vm->setGlobal("__webgl_useProgram", Value::object(createNativeFunction("useProgram", WebGLNative::useProgram, 2)));
    vm->setGlobal("__webgl_deleteProgram", Value::object(createNativeFunction("deleteProgram", WebGLNative::deleteProgram, 2)));
    
    vm->setGlobal("__webgl_getAttribLocation", Value::object(createNativeFunction("getAttribLocation", WebGLNative::getAttribLocation, 3)));
    vm->setGlobal("__webgl_vertexAttribPointer", Value::object(createNativeFunction("vertexAttribPointer", WebGLNative::vertexAttribPointer, 7)));
    vm->setGlobal("__webgl_enableVertexAttribArray", Value::object(createNativeFunction("enableVertexAttribArray", WebGLNative::enableVertexAttribArray, 2)));
    vm->setGlobal("__webgl_disableVertexAttribArray", Value::object(createNativeFunction("disableVertexAttribArray", WebGLNative::disableVertexAttribArray, 2)));
    
    vm->setGlobal("__webgl_getUniformLocation", Value::object(createNativeFunction("getUniformLocation", WebGLNative::getUniformLocation, 3)));
    vm->setGlobal("__webgl_uniform1i", Value::object(createNativeFunction("uniform1i", WebGLNative::uniform1i, 3)));
    vm->setGlobal("__webgl_uniform1f", Value::object(createNativeFunction("uniform1f", WebGLNative::uniform1f, 3)));
    vm->setGlobal("__webgl_uniform2f", Value::object(createNativeFunction("uniform2f", WebGLNative::uniform2f, 4)));
    vm->setGlobal("__webgl_uniform3f", Value::object(createNativeFunction("uniform3f", WebGLNative::uniform3f, 5)));
    vm->setGlobal("__webgl_uniform4f", Value::object(createNativeFunction("uniform4f", WebGLNative::uniform4f, 6)));
    vm->setGlobal("__webgl_uniformMatrix4fv", Value::object(createNativeFunction("uniformMatrix4fv", WebGLNative::uniformMatrix4fv, 4)));
    
    vm->setGlobal("__webgl_createBuffer", Value::object(createNativeFunction("createBuffer", WebGLNative::createBuffer, 1)));
    vm->setGlobal("__webgl_bindBuffer", Value::object(createNativeFunction("bindBuffer", WebGLNative::bindBuffer, 3)));
    vm->setGlobal("__webgl_bufferData", Value::object(createNativeFunction("bufferData", WebGLNative::bufferData, 4)));
    vm->setGlobal("__webgl_deleteBuffer", Value::object(createNativeFunction("deleteBuffer", WebGLNative::deleteBuffer, 2)));
    
    vm->setGlobal("__webgl_createTexture", Value::object(createNativeFunction("createTexture", WebGLNative::createTexture, 1)));
    vm->setGlobal("__webgl_bindTexture", Value::object(createNativeFunction("bindTexture", WebGLNative::bindTexture, 3)));
    vm->setGlobal("__webgl_activeTexture", Value::object(createNativeFunction("activeTexture", WebGLNative::activeTexture, 2)));
    vm->setGlobal("__webgl_texParameteri", Value::object(createNativeFunction("texParameteri", WebGLNative::texParameteri, 4)));
    vm->setGlobal("__webgl_texImage2D", Value::object(createNativeFunction("texImage2D", WebGLNative::texImage2D, 10)));
    vm->setGlobal("__webgl_deleteTexture", Value::object(createNativeFunction("deleteTexture", WebGLNative::deleteTexture, 2)));
    
    vm->setGlobal("__webgl_drawArrays", Value::object(createNativeFunction("drawArrays", WebGLNative::drawArrays, 4)));
    vm->setGlobal("__webgl_drawElements", Value::object(createNativeFunction("drawElements", WebGLNative::drawElements, 5)));
    
    vm->setGlobal("__webgl_getError", Value::object(createNativeFunction("getError", WebGLNative::getError, 1)));
    
    // Helper
    vm->setGlobal("__webgl_getContextWidth", Value::object(createNativeFunction("getContextWidth", WebGLNative::getContextWidth, 1)));
    vm->setGlobal("__webgl_getContextHeight", Value::object(createNativeFunction("getContextHeight", WebGLNative::getContextHeight, 1)));
}

// =============================================================================
// Native Function Implementations
// =============================================================================

namespace WebGLNative {

// Helper to extract context handle from first argument
static WebGLRenderingContext* getGL(const std::vector<Runtime::Value>& args) {
    if (args.empty() || !args[0].isNumber()) return nullptr;
    uint32_t handle = static_cast<uint32_t>(args[0].toNumber());
    return WebGLBindings::getContext(handle);
}

Runtime::Value getContextWidth(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    return gl ? Runtime::Value::number(gl->drawingBufferWidth()) : Runtime::Value::undefined();
}

Runtime::Value getContextHeight(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    return gl ? Runtime::Value::number(gl->drawingBufferHeight()) : Runtime::Value::undefined();
}

Runtime::Value viewport(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 5) {
        gl->viewport(
            static_cast<GLint>(args[1].toNumber()),
            static_cast<GLint>(args[2].toNumber()),
            static_cast<GLsizei>(args[3].toNumber()),
            static_cast<GLsizei>(args[4].toNumber())
        );
    }
    return Runtime::Value::undefined();
}

Runtime::Value clearColor(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 5) {
        gl->clearColor(
            static_cast<GLclampf>(args[1].toNumber()),
            static_cast<GLclampf>(args[2].toNumber()),
            static_cast<GLclampf>(args[3].toNumber()),
            static_cast<GLclampf>(args[4].toNumber())
        );
    }
    return Runtime::Value::undefined();
}

Runtime::Value clear(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        gl->clear(static_cast<GLbitfield>(args[1].toNumber()));
    }
    return Runtime::Value::undefined();
}

Runtime::Value enable(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        gl->enable(static_cast<GLenum>(args[1].toNumber()));
    }
    return Runtime::Value::undefined();
}

Runtime::Value disable(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        gl->disable(static_cast<GLenum>(args[1].toNumber()));
    }
    return Runtime::Value::undefined();
}

Runtime::Value blendFunc(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 3) {
        gl->blendFunc(
            static_cast<GLenum>(args[1].toNumber()),
            static_cast<GLenum>(args[2].toNumber())
        );
    }
    return Runtime::Value::undefined();
}

Runtime::Value depthFunc(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        gl->depthFunc(static_cast<GLenum>(args[1].toNumber()));
    }
    return Runtime::Value::undefined();
}

Runtime::Value createShader(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        auto shader = gl->createShader(static_cast<GLenum>(args[1].toNumber()));
        return Runtime::Value::number(shader.id);
    }
    return Runtime::Value::null();
}

Runtime::Value shaderSource(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 3 && args[2].isString()) {
        WebGLShader shader{static_cast<GLuint>(args[1].toNumber())};
        gl->shaderSource(shader, args[2].toString());
    }
    return Runtime::Value::undefined();
}

Runtime::Value compileShader(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        WebGLShader shader{static_cast<GLuint>(args[1].toNumber())};
        gl->compileShader(shader);
    }
    return Runtime::Value::undefined();
}

Runtime::Value getShaderParameter(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 3) {
        WebGLShader shader{static_cast<GLuint>(args[1].toNumber())};
        return Runtime::Value::boolean(
            gl->getShaderParameter(shader, static_cast<GLenum>(args[2].toNumber())));
    }
    return Runtime::Value::null();
}

Runtime::Value getShaderInfoLog(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        WebGLShader shader{static_cast<GLuint>(args[1].toNumber())};
        return Runtime::Value::string(new Runtime::String(gl->getShaderInfoLog(shader)));
    }
    return Runtime::Value::string(new Runtime::String(""));
}

Runtime::Value deleteShader(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        WebGLShader shader{static_cast<GLuint>(args[1].toNumber())};
        gl->deleteShader(shader);
    }
    return Runtime::Value::undefined();
}

Runtime::Value createProgram(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl) {
        auto program = gl->createProgram();
        return Runtime::Value::number(program.id);
    }
    return Runtime::Value::null();
}

Runtime::Value attachShader(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 3) {
        WebGLProgram program{static_cast<GLuint>(args[1].toNumber())};
        WebGLShader shader{static_cast<GLuint>(args[2].toNumber())};
        gl->attachShader(program, shader);
    }
    return Runtime::Value::undefined();
}

Runtime::Value linkProgram(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        WebGLProgram program{static_cast<GLuint>(args[1].toNumber())};
        gl->linkProgram(program);
    }
    return Runtime::Value::undefined();
}

Runtime::Value getProgramParameter(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 3) {
        WebGLProgram program{static_cast<GLuint>(args[1].toNumber())};
        return Runtime::Value::boolean(
            gl->getProgramParameter(program, static_cast<GLenum>(args[2].toNumber())));
    }
    return Runtime::Value::null();
}

Runtime::Value useProgram(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        WebGLProgram program{static_cast<GLuint>(args[1].toNumber())};
        gl->useProgram(program);
    }
    return Runtime::Value::undefined();
}

Runtime::Value deleteProgram(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        WebGLProgram program{static_cast<GLuint>(args[1].toNumber())};
        gl->deleteProgram(program);
    }
    return Runtime::Value::undefined();
}

Runtime::Value getAttribLocation(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 3 && args[2].isString()) {
        WebGLProgram program{static_cast<GLuint>(args[1].toNumber())};
        return Runtime::Value::number(gl->getAttribLocation(program, args[2].toString()));
    }
    return Runtime::Value::number(-1);
}

Runtime::Value vertexAttribPointer(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 7) {
        gl->vertexAttribPointer(
            static_cast<GLuint>(args[1].toNumber()),
            static_cast<GLint>(args[2].toNumber()),
            static_cast<GLenum>(args[3].toNumber()),
            args[4].toBoolean() ? GL_TRUE : GL_FALSE,
            static_cast<GLsizei>(args[5].toNumber()),
            static_cast<GLintptr>(args[6].toNumber())
        );
    }
    return Runtime::Value::undefined();
}

Runtime::Value enableVertexAttribArray(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        gl->enableVertexAttribArray(static_cast<GLuint>(args[1].toNumber()));
    }
    return Runtime::Value::undefined();
}

Runtime::Value disableVertexAttribArray(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        gl->disableVertexAttribArray(static_cast<GLuint>(args[1].toNumber()));
    }
    return Runtime::Value::undefined();
}

Runtime::Value getUniformLocation(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 3 && args[2].isString()) {
        WebGLProgram program{static_cast<GLuint>(args[1].toNumber())};
        auto loc = gl->getUniformLocation(program, args[2].toString());
        return Runtime::Value::number(loc.loc);
    }
    return Runtime::Value::number(-1);
}

Runtime::Value uniform1i(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 3) {
        WebGLUniformLocation loc{static_cast<GLint>(args[1].toNumber())};
        gl->uniform1i(loc, static_cast<GLint>(args[2].toNumber()));
    }
    return Runtime::Value::undefined();
}

Runtime::Value uniform1f(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 3) {
        WebGLUniformLocation loc{static_cast<GLint>(args[1].toNumber())};
        gl->uniform1f(loc, static_cast<GLfloat>(args[2].toNumber()));
    }
    return Runtime::Value::undefined();
}

Runtime::Value uniform2f(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 4) {
        WebGLUniformLocation loc{static_cast<GLint>(args[1].toNumber())};
        gl->uniform2f(loc, 
            static_cast<GLfloat>(args[2].toNumber()),
            static_cast<GLfloat>(args[3].toNumber()));
    }
    return Runtime::Value::undefined();
}

Runtime::Value uniform3f(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 5) {
        WebGLUniformLocation loc{static_cast<GLint>(args[1].toNumber())};
        gl->uniform3f(loc,
            static_cast<GLfloat>(args[2].toNumber()),
            static_cast<GLfloat>(args[3].toNumber()),
            static_cast<GLfloat>(args[4].toNumber()));
    }
    return Runtime::Value::undefined();
}

Runtime::Value uniform4f(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 6) {
        WebGLUniformLocation loc{static_cast<GLint>(args[1].toNumber())};
        gl->uniform4f(loc,
            static_cast<GLfloat>(args[2].toNumber()),
            static_cast<GLfloat>(args[3].toNumber()),
            static_cast<GLfloat>(args[4].toNumber()),
            static_cast<GLfloat>(args[5].toNumber()));
    }
    return Runtime::Value::undefined();
}

Runtime::Value uniformMatrix4fv(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    // args: handle, loc, transpose, array of 16 floats
    if (gl && args.size() >= 4 && args[3].isObject()) {
        WebGLUniformLocation loc{static_cast<GLint>(args[1].toNumber())};
        bool transpose = args[2].toBoolean();
        
        // Extract array values
        GLfloat matrix[16] = {0};
        auto* arr = args[3].toObject();
        if (arr) {
            for (int i = 0; i < 16; i++) {
                auto val = arr->get(std::to_string(i));
                if (val.isNumber()) {
                    matrix[i] = static_cast<GLfloat>(val.toNumber());
                }
            }
        }
        
        gl->uniformMatrix4fv(loc, transpose ? GL_TRUE : GL_FALSE, matrix);
    }
    return Runtime::Value::undefined();
}

Runtime::Value createBuffer(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl) {
        auto buffer = gl->createBuffer();
        return Runtime::Value::number(buffer.id);
    }
    return Runtime::Value::null();
}

Runtime::Value bindBuffer(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 3) {
        WebGLBuffer buffer{static_cast<GLuint>(args[2].toNumber())};
        gl->bindBuffer(static_cast<GLenum>(args[1].toNumber()), buffer);
    }
    return Runtime::Value::undefined();
}

Runtime::Value bufferData(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    // args: handle, target, data (array), usage
    if (gl && args.size() >= 4 && args[2].isObject()) {
        GLenum target = static_cast<GLenum>(args[1].toNumber());
        GLenum usage = static_cast<GLenum>(args[3].toNumber());
        
        // Convert JS array to float buffer
        auto* arr = args[2].toObject();
        if (arr) {
            auto lenVal = arr->get("length");
            int len = lenVal.isNumber() ? static_cast<int>(lenVal.toNumber()) : 0;
            
            std::vector<float> data(len);
            for (int i = 0; i < len; i++) {
                auto val = arr->get(std::to_string(i));
                data[i] = val.isNumber() ? static_cast<float>(val.toNumber()) : 0;
            }
            
            gl->bufferData(target, data.data(), data.size() * sizeof(float), usage);
        }
    }
    return Runtime::Value::undefined();
}

Runtime::Value deleteBuffer(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        WebGLBuffer buffer{static_cast<GLuint>(args[1].toNumber())};
        gl->deleteBuffer(buffer);
    }
    return Runtime::Value::undefined();
}

Runtime::Value createTexture(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl) {
        auto texture = gl->createTexture();
        return Runtime::Value::number(texture.id);
    }
    return Runtime::Value::null();
}

Runtime::Value bindTexture(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 3) {
        WebGLTexture texture{static_cast<GLuint>(args[2].toNumber())};
        gl->bindTexture(static_cast<GLenum>(args[1].toNumber()), texture);
    }
    return Runtime::Value::undefined();
}

Runtime::Value activeTexture(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        gl->activeTexture(static_cast<GLenum>(args[1].toNumber()));
    }
    return Runtime::Value::undefined();
}

Runtime::Value texImage2D(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    // Simplified - full implementation would handle ImageData, HTMLImageElement etc.
    auto* gl = getGL(args);
    if (gl && args.size() >= 9) {
        gl->texImage2D(
            static_cast<GLenum>(args[1].toNumber()),   // target
            static_cast<GLint>(args[2].toNumber()),    // level
            static_cast<GLenum>(args[3].toNumber()),   // internalformat
            static_cast<GLsizei>(args[4].toNumber()),  // width
            static_cast<GLsizei>(args[5].toNumber()),  // height
            static_cast<GLint>(args[6].toNumber()),    // border
            static_cast<GLenum>(args[7].toNumber()),   // format
            static_cast<GLenum>(args[8].toNumber()),   // type
            nullptr  // pixels - would need array buffer support
        );
    }
    return Runtime::Value::undefined();
}

Runtime::Value texParameteri(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 4) {
        gl->texParameteri(
            static_cast<GLenum>(args[1].toNumber()),
            static_cast<GLenum>(args[2].toNumber()),
            static_cast<GLint>(args[3].toNumber())
        );
    }
    return Runtime::Value::undefined();
}

Runtime::Value deleteTexture(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 2) {
        WebGLTexture texture{static_cast<GLuint>(args[1].toNumber())};
        gl->deleteTexture(texture);
    }
    return Runtime::Value::undefined();
}

Runtime::Value drawArrays(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 4) {
        gl->drawArrays(
            static_cast<GLenum>(args[1].toNumber()),
            static_cast<GLint>(args[2].toNumber()),
            static_cast<GLsizei>(args[3].toNumber())
        );
    }
    return Runtime::Value::undefined();
}

Runtime::Value drawElements(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl && args.size() >= 5) {
        gl->drawElements(
            static_cast<GLenum>(args[1].toNumber()),
            static_cast<GLsizei>(args[2].toNumber()),
            static_cast<GLenum>(args[3].toNumber()),
            static_cast<GLintptr>(args[4].toNumber())
        );
    }
    return Runtime::Value::undefined();
}

Runtime::Value getError(Runtime::Context*, const std::vector<Runtime::Value>& args) {
    auto* gl = getGL(args);
    if (gl) {
        return Runtime::Value::number(gl->getError());
    }
    return Runtime::Value::number(WebGLConstants::NO_ERROR);
}

} // namespace WebGLNative

} // namespace Zepra::WebCore
