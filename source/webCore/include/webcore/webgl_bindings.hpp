/**
 * @file webgl_bindings.hpp
 * @brief JavaScript bindings for WebGL API
 * 
 * Provides the bridge between JavaScript WebGLRenderingContext calls
 * and the C++ WebGLRenderingContext implementation.
 */

#pragma once

#include "webgl_context.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

// Forward declarations for ZepraScript runtime types
// We need full definition of Value for vector instantiation usually
#include "zeprascript/runtime/value.hpp"
// Check if header is available or forward declare if possible
// The error was "invalid use of incomplete type 'class Zepra::Runtime::Value'" in vector
// So we need #include "zeprascript/runtime/value.hpp"

namespace Zepra::Runtime {
    class Object;
    class Context;
    class VM;
}

namespace Zepra::WebCore {

/**
 * @brief Manages WebGL contexts and exposes them to JavaScript
 */
class WebGLBindings {
public:
    /**
     * @brief Create WebGL bindings for a canvas element
     * @param width Canvas width
     * @param height Canvas height
     * @return Handle to the WebGL context
     */
    static uint32_t createContext(int width, int height);
    
    /**
     * @brief Get a context by handle
     */
    static WebGLRenderingContext* getContext(uint32_t handle);
    
    /**
     * @brief Destroy a context
     */
    static void destroyContext(uint32_t handle);
    
    /**
     * @brief Create a JavaScript WebGLRenderingContext object
     * for use in the VM
     */
    static Runtime::Object* createJSContextObject(Runtime::VM* vm, uint32_t handle);
    
    /**
     * @brief Register all WebGL native functions
     */
    static void registerNativeFunctions(Runtime::VM* vm);
    
private:
    static std::unordered_map<uint32_t, std::unique_ptr<WebGLRenderingContext>> contexts_;
    static uint32_t nextHandle_;
};

// =============================================================================
// Native Function Callbacks for WebGL API
// =============================================================================

namespace WebGLNative {
    // Context
    Runtime::Value getContextWidth(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value getContextHeight(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    
    // Viewport/Clear
    Runtime::Value viewport(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value clearColor(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value clear(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    
    // State
    Runtime::Value enable(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value disable(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value blendFunc(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value depthFunc(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    
    // Shaders
    Runtime::Value createShader(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value shaderSource(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value compileShader(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value getShaderParameter(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value getShaderInfoLog(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value deleteShader(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    
    // Programs
    Runtime::Value createProgram(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value attachShader(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value linkProgram(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value getProgramParameter(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value useProgram(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value deleteProgram(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    
    // Attributes
    Runtime::Value getAttribLocation(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value vertexAttribPointer(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value enableVertexAttribArray(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value disableVertexAttribArray(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    
    // Uniforms
    Runtime::Value getUniformLocation(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value uniform1i(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value uniform1f(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value uniform2f(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value uniform3f(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value uniform4f(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value uniformMatrix4fv(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    
    // Buffers
    Runtime::Value createBuffer(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value bindBuffer(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value bufferData(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value deleteBuffer(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    
    // Textures
    Runtime::Value createTexture(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value bindTexture(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value activeTexture(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value texImage2D(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value texParameteri(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value deleteTexture(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    
    // Drawing
    Runtime::Value drawArrays(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    Runtime::Value drawElements(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
    
    // Error
    Runtime::Value getError(Runtime::Context* ctx, const std::vector<Runtime::Value>& args);
}

} // namespace Zepra::WebCore
