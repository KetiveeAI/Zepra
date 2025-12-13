/**
 * @file webgpu_bindings.hpp
 * @brief JavaScript bindings for WebGPU API
 */

#ifndef WEBGPU_BINDINGS_HPP
#define WEBGPU_BINDINGS_HPP

#include "webgpu/webgpu_context.hpp"

namespace Zepra::Runtime {
    class VM;
    class Object;
}

namespace Zepra::WebGPU {

/**
 * WebGPU JavaScript Bindings
 * Exposes navigator.gpu and all WebGPU objects to JavaScript
 */
class WebGPUBindings {
public:
    /**
     * Register WebGPU APIs with the JavaScript VM
     */
    static void registerWithVM(Runtime::VM* vm);
    
    /**
     * Create navigator.gpu object
     */
    static Runtime::Object* createGPUObject(Runtime::VM* vm);
    
private:
    static Runtime::Object* createAdapterObject(Runtime::VM* vm, GPUAdapter* adapter);
    static Runtime::Object* createDeviceObject(Runtime::VM* vm, GPUDevice* device);
    static Runtime::Object* createBufferObject(Runtime::VM* vm, GPUBuffer* buffer);
    static Runtime::Object* createTextureObject(Runtime::VM* vm, GPUTexture* texture);
    static Runtime::Object* createQueueObject(Runtime::VM* vm, GPUQueue* queue);
};

} // namespace Zepra::WebGPU

#endif // WEBGPU_BINDINGS_HPP
