/**
 * @file webgpu_bindings.cpp
 * @brief JavaScript bindings implementation for WebGPU
 */

#include "webgpu/webgpu_bindings.hpp"
#include "zeprascript/runtime/object.hpp"
#include "zeprascript/runtime/value.hpp"
#include "zeprascript/runtime/vm.hpp"
#include "zeprascript/runtime/function.hpp"
#include <memory>
#include <unordered_map>

namespace Zepra::WebGPU {

// Store device references
static std::unordered_map<uint32_t, std::unique_ptr<GPUDevice>> g_devices;
static std::unordered_map<uint32_t, std::unique_ptr<GPUAdapter>> g_adapters;
static std::unordered_map<uint32_t, std::unique_ptr<GPUBuffer>> g_buffers;
static uint32_t g_nextId = 1;

void WebGPUBindings::registerWithVM(Runtime::VM* vm) {
    if (!vm) return;
    
    // Create navigator.gpu
    auto* gpu = createGPUObject(vm);
    
    // Get or create navigator object
    auto navVal = vm->getGlobal("navigator");
    Runtime::Object* navigator = nullptr;
    if (navVal.isObject()) {
        navigator = navVal.toObject();
    }
    
    if (navigator) {
        navigator->set("gpu", Runtime::Value::object(gpu));
    }
    
    // Also expose GPU as global for direct access
    vm->setGlobal("GPU", Runtime::Value::object(gpu));
}

Runtime::Object* WebGPUBindings::createGPUObject(Runtime::VM* vm) {
    auto* gpu = new Runtime::Object();
    
    // requestAdapter() -> Promise<GPUAdapter>
    gpu->set("requestAdapter", Runtime::Value::object(Runtime::createNativeFunction("requestAdapter",
        [vm](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            auto adapter = GPU::instance().requestAdapter();
            if (!adapter) return Runtime::Value::null();
            
            uint32_t id = g_nextId++;
            auto* adapterObj = WebGPUBindings::createAdapterObject(vm, adapter.get());
            g_adapters[id] = std::move(adapter);
            
            return Runtime::Value::object(adapterObj);
        }, 1)));
    
    // getPreferredCanvasFormat()
    gpu->set("getPreferredCanvasFormat", Runtime::Value::object(Runtime::createNativeFunction("getPreferredCanvasFormat",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            return Runtime::Value::string(new Runtime::String("bgra8unorm"));
        }, 0)));
    
    // wgslLanguageFeatures (Set)
    auto* features = new Runtime::Object();
    features->set("has", Runtime::Value::object(Runtime::createNativeFunction("has",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            return Runtime::Value::boolean(true);
        }, 1)));
    gpu->set("wgslLanguageFeatures", Runtime::Value::object(features));
    
    return gpu;
}

Runtime::Object* WebGPUBindings::createAdapterObject(Runtime::VM* vm, GPUAdapter* adapter) {
    auto* obj = new Runtime::Object();
    
    // isFallbackAdapter
    obj->set("isFallbackAdapter", Runtime::Value::boolean(adapter->isFallbackAdapter()));
    
    // features (Set)
    auto* features = new Runtime::Object();
    features->set("has", Runtime::Value::object(Runtime::createNativeFunction("has",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            return Runtime::Value::boolean(false);
        }, 1)));
    obj->set("features", Runtime::Value::object(features));
    
    // limits
    auto* limits = new Runtime::Object();
    limits->set("maxTextureDimension2D", Runtime::Value::number(8192));
    limits->set("maxBindGroups", Runtime::Value::number(4));
    limits->set("maxBufferSize", Runtime::Value::number(256 * 1024 * 1024));
    limits->set("maxComputeWorkgroupSizeX", Runtime::Value::number(256));
    limits->set("maxComputeWorkgroupSizeY", Runtime::Value::number(256));
    limits->set("maxComputeWorkgroupSizeZ", Runtime::Value::number(64));
    obj->set("limits", Runtime::Value::object(limits));
    
    // requestDevice() -> Promise<GPUDevice>
    obj->set("requestDevice", Runtime::Value::object(Runtime::createNativeFunction("requestDevice",
        [vm, adapter](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            auto device = adapter->requestDevice();
            if (!device) return Runtime::Value::null();
            
            uint32_t id = g_nextId++;
            auto* deviceObj = WebGPUBindings::createDeviceObject(vm, device.get());
            g_devices[id] = std::move(device);
            
            return Runtime::Value::object(deviceObj);
        }, 1)));
    
    // requestAdapterInfo()
    obj->set("requestAdapterInfo", Runtime::Value::object(Runtime::createNativeFunction("requestAdapterInfo",
        [adapter](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            auto* info = new Runtime::Object();
            info->set("vendor", Runtime::Value::string(new Runtime::String(adapter->info().vendor)));
            info->set("architecture", Runtime::Value::string(new Runtime::String(adapter->info().architecture)));
            info->set("device", Runtime::Value::string(new Runtime::String(adapter->info().device)));
            info->set("description", Runtime::Value::string(new Runtime::String(adapter->info().description)));
            return Runtime::Value::object(info);
        }, 0)));
    
    return obj;
}

Runtime::Object* WebGPUBindings::createDeviceObject(Runtime::VM* vm, GPUDevice* device) {
    auto* obj = new Runtime::Object();
    
    // queue
    obj->set("queue", Runtime::Value::object(createQueueObject(vm, device->queue())));
    
    // features (Set)
    auto* features = new Runtime::Object();
    features->set("has", Runtime::Value::object(Runtime::createNativeFunction("has",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            return Runtime::Value::boolean(false);
        }, 1)));
    obj->set("features", Runtime::Value::object(features));
    
    // limits
    auto* limits = new Runtime::Object();
    const auto& l = device->limits();
    limits->set("maxTextureDimension2D", Runtime::Value::number(l.maxTextureDimension2D));
    limits->set("maxBindGroups", Runtime::Value::number(l.maxBindGroups));
    limits->set("maxBufferSize", Runtime::Value::number(l.maxBufferSize));
    limits->set("maxUniformBufferBindingSize", Runtime::Value::number(l.maxUniformBufferBindingSize));
    limits->set("maxStorageBufferBindingSize", Runtime::Value::number(l.maxStorageBufferBindingSize));
    obj->set("limits", Runtime::Value::object(limits));
    
    // createBuffer(descriptor) -> GPUBuffer
    obj->set("createBuffer", Runtime::Value::object(Runtime::createNativeFunction("createBuffer",
        [vm, device](Runtime::Context*, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            if (args.empty() || !args[0].isObject()) return Runtime::Value::null();
            
            auto* desc = args[0].toObject();
            GPUBufferDescriptor bufDesc;
            
            auto sizeVal = desc->get("size");
            if (sizeVal.isNumber()) bufDesc.size = static_cast<uint64_t>(sizeVal.toNumber());
            
            auto usageVal = desc->get("usage");
            if (usageVal.isNumber()) bufDesc.usage = static_cast<uint32_t>(usageVal.toNumber());
            
            auto mappedVal = desc->get("mappedAtCreation");
            if (mappedVal.isBoolean()) bufDesc.mappedAtCreation = mappedVal.toBoolean();
            
            auto buffer = device->createBuffer(bufDesc);
            if (!buffer) return Runtime::Value::null();
            
            uint32_t id = g_nextId++;
            auto* bufferObj = WebGPUBindings::createBufferObject(vm, buffer.get());
            g_buffers[id] = std::move(buffer);
            
            return Runtime::Value::object(bufferObj);
        }, 1)));
    
    // createShaderModule(descriptor) -> GPUShaderModule
    obj->set("createShaderModule", Runtime::Value::object(Runtime::createNativeFunction("createShaderModule",
        [device](Runtime::Context*, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            if (args.empty() || !args[0].isObject()) return Runtime::Value::null();
            
            auto* desc = args[0].toObject();
            GPUShaderModuleDescriptor shaderDesc;
            
            auto codeVal = desc->get("code");
            if (codeVal.isString()) shaderDesc.code = codeVal.toString();
            
            auto module = device->createShaderModule(shaderDesc);
            if (!module) return Runtime::Value::null();
            
            auto* moduleObj = new Runtime::Object();
            moduleObj->set("getCompilationInfo", Runtime::Value::object(Runtime::createNativeFunction("getCompilationInfo",
                [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
                    auto* info = new Runtime::Object();
                    auto* messages = new Runtime::Object();
                    messages->set("length", Runtime::Value::number(0));
                    info->set("messages", Runtime::Value::object(messages));
                    return Runtime::Value::object(info);
                }, 0)));
            
            return Runtime::Value::object(moduleObj);
        }, 1)));
    
    // createCommandEncoder()
    obj->set("createCommandEncoder", Runtime::Value::object(Runtime::createNativeFunction("createCommandEncoder",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            auto* encoder = new Runtime::Object();
            
            encoder->set("beginRenderPass", Runtime::Value::object(Runtime::createNativeFunction("beginRenderPass",
                [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
                    auto* pass = new Runtime::Object();
                    pass->set("setPipeline", Runtime::Value::object(Runtime::createNativeFunction("setPipeline",
                        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
                            return Runtime::Value::undefined();
                        }, 1)));
                    pass->set("draw", Runtime::Value::object(Runtime::createNativeFunction("draw",
                        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
                            return Runtime::Value::undefined();
                        }, 4)));
                    pass->set("end", Runtime::Value::object(Runtime::createNativeFunction("end",
                        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
                            return Runtime::Value::undefined();
                        }, 0)));
                    return Runtime::Value::object(pass);
                }, 1)));
            
            encoder->set("beginComputePass", Runtime::Value::object(Runtime::createNativeFunction("beginComputePass",
                [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
                    auto* pass = new Runtime::Object();
                    pass->set("setPipeline", Runtime::Value::object(Runtime::createNativeFunction("setPipeline",
                        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
                            return Runtime::Value::undefined();
                        }, 1)));
                    pass->set("setBindGroup", Runtime::Value::object(Runtime::createNativeFunction("setBindGroup",
                        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
                            return Runtime::Value::undefined();
                        }, 2)));
                    pass->set("dispatchWorkgroups", Runtime::Value::object(Runtime::createNativeFunction("dispatchWorkgroups",
                        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
                            return Runtime::Value::undefined();
                        }, 3)));
                    pass->set("end", Runtime::Value::object(Runtime::createNativeFunction("end",
                        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
                            return Runtime::Value::undefined();
                        }, 0)));
                    return Runtime::Value::object(pass);
                }, 1)));
            
            encoder->set("copyBufferToBuffer", Runtime::Value::object(Runtime::createNativeFunction("copyBufferToBuffer",
                [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
                    return Runtime::Value::undefined();
                }, 5)));
            
            encoder->set("finish", Runtime::Value::object(Runtime::createNativeFunction("finish",
                [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
                    auto* commandBuffer = new Runtime::Object();
                    return Runtime::Value::object(commandBuffer);
                }, 0)));
            
            return Runtime::Value::object(encoder);
        }, 1)));
    
    // createRenderPipeline(descriptor)
    obj->set("createRenderPipeline", Runtime::Value::object(Runtime::createNativeFunction("createRenderPipeline",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            auto* pipeline = new Runtime::Object();
            pipeline->set("getBindGroupLayout", Runtime::Value::object(Runtime::createNativeFunction("getBindGroupLayout",
                [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
                    return Runtime::Value::object(new Runtime::Object());
                }, 1)));
            return Runtime::Value::object(pipeline);
        }, 1)));
    
    // createComputePipeline(descriptor)
    obj->set("createComputePipeline", Runtime::Value::object(Runtime::createNativeFunction("createComputePipeline",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            auto* pipeline = new Runtime::Object();
            pipeline->set("getBindGroupLayout", Runtime::Value::object(Runtime::createNativeFunction("getBindGroupLayout",
                [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
                    return Runtime::Value::object(new Runtime::Object());
                }, 1)));
            return Runtime::Value::object(pipeline);
        }, 1)));
    
    // createBindGroup(descriptor)
    obj->set("createBindGroup", Runtime::Value::object(Runtime::createNativeFunction("createBindGroup",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            return Runtime::Value::object(new Runtime::Object());
        }, 1)));
    
    // destroy()
    obj->set("destroy", Runtime::Value::object(Runtime::createNativeFunction("destroy",
        [device](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            device->destroy();
            return Runtime::Value::undefined();
        }, 0)));
    
    return obj;
}

Runtime::Object* WebGPUBindings::createBufferObject(Runtime::VM* vm, GPUBuffer* buffer) {
    auto* obj = new Runtime::Object();
    
    obj->set("size", Runtime::Value::number(static_cast<double>(buffer->size())));
    obj->set("usage", Runtime::Value::number(buffer->usage()));
    
    obj->set("mapAsync", Runtime::Value::object(Runtime::createNativeFunction("mapAsync",
        [buffer](Runtime::Context*, const std::vector<Runtime::Value>& args) -> Runtime::Value {
            uint32_t mode = args.size() > 0 ? static_cast<uint32_t>(args[0].toNumber()) : 0;
            buffer->mapAsync(mode);
            return Runtime::Value::undefined();
        }, 1)));
    
    obj->set("getMappedRange", Runtime::Value::object(Runtime::createNativeFunction("getMappedRange",
        [buffer](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            // Return ArrayBuffer-like object
            auto* arrayBuffer = new Runtime::Object();
            arrayBuffer->set("byteLength", Runtime::Value::number(static_cast<double>(buffer->size())));
            return Runtime::Value::object(arrayBuffer);
        }, 2)));
    
    obj->set("unmap", Runtime::Value::object(Runtime::createNativeFunction("unmap",
        [buffer](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            buffer->unmap();
            return Runtime::Value::undefined();
        }, 0)));
    
    obj->set("destroy", Runtime::Value::object(Runtime::createNativeFunction("destroy",
        [buffer](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            buffer->destroy();
            return Runtime::Value::undefined();
        }, 0)));
    
    return obj;
}

Runtime::Object* WebGPUBindings::createTextureObject(Runtime::VM* vm, GPUTexture* texture) {
    auto* obj = new Runtime::Object();
    
    obj->set("width", Runtime::Value::number(texture->width()));
    obj->set("height", Runtime::Value::number(texture->height()));
    
    obj->set("createView", Runtime::Value::object(Runtime::createNativeFunction("createView",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            return Runtime::Value::object(new Runtime::Object());
        }, 1)));
    
    obj->set("destroy", Runtime::Value::object(Runtime::createNativeFunction("destroy",
        [texture](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            texture->destroy();
            return Runtime::Value::undefined();
        }, 0)));
    
    return obj;
}

Runtime::Object* WebGPUBindings::createQueueObject(Runtime::VM* vm, GPUQueue* queue) {
    auto* obj = new Runtime::Object();
    
    obj->set("submit", Runtime::Value::object(Runtime::createNativeFunction("submit",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            // Process command buffers
            return Runtime::Value::undefined();
        }, 1)));
    
    obj->set("writeBuffer", Runtime::Value::object(Runtime::createNativeFunction("writeBuffer",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            return Runtime::Value::undefined();
        }, 4)));
    
    obj->set("onSubmittedWorkDone", Runtime::Value::object(Runtime::createNativeFunction("onSubmittedWorkDone",
        [](Runtime::Context*, const std::vector<Runtime::Value>&) -> Runtime::Value {
            return Runtime::Value::undefined();
        }, 0)));
    
    return obj;
}

} // namespace Zepra::WebGPU
