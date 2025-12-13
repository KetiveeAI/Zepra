/**
 * @file webgpu_context.hpp
 * @brief WebGPU API implementation for Zepra Browser
 * 
 * Implements the WebGPU spec for modern GPU compute and graphics.
 * This is the next-generation API replacing WebGL.
 */

#ifndef WEBGPU_CONTEXT_HPP
#define WEBGPU_CONTEXT_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

namespace Zepra::WebGPU {

// Forward declarations
class GPUAdapter;
class GPUDevice;
class GPUBuffer;
class GPUTexture;
class GPUShaderModule;
class GPUBindGroup;
class GPUBindGroupLayout;
class GPUPipelineLayout;
class GPURenderPipeline;
class GPUComputePipeline;
class GPUCommandEncoder;
class GPUCommandBuffer;
class GPUQueue;

// =============================================================================
// GPU Object Handles
// =============================================================================

struct GPUObjectBase {
    uint32_t id = 0;
    std::string label;
    bool valid = true;
};

// =============================================================================
// GPUBuffer
// =============================================================================

enum class GPUBufferUsage : uint32_t {
    MAP_READ       = 0x0001,
    MAP_WRITE      = 0x0002,
    COPY_SRC       = 0x0004,
    COPY_DST       = 0x0008,
    INDEX          = 0x0010,
    VERTEX         = 0x0020,
    UNIFORM        = 0x0040,
    STORAGE        = 0x0080,
    INDIRECT       = 0x0100,
    QUERY_RESOLVE  = 0x0200
};

struct GPUBufferDescriptor {
    std::string label;
    uint64_t size = 0;
    uint32_t usage = 0;
    bool mappedAtCreation = false;
};

class GPUBuffer : public GPUObjectBase {
public:
    GPUBuffer(const GPUBufferDescriptor& desc);
    ~GPUBuffer();
    
    void mapAsync(uint32_t mode, uint64_t offset = 0, uint64_t size = 0);
    void* getMappedRange(uint64_t offset = 0, uint64_t size = 0);
    void unmap();
    void destroy();
    
    uint64_t size() const { return size_; }
    uint32_t usage() const { return usage_; }
    
private:
    uint64_t size_ = 0;
    uint32_t usage_ = 0;
    std::vector<uint8_t> data_;
    bool mapped_ = false;
};

// =============================================================================
// GPUTexture
// =============================================================================

enum class GPUTextureFormat {
    RGBA8Unorm,
    RGBA8UnormSRGB,
    BGRA8Unorm,
    BGRA8UnormSRGB,
    Depth24Plus,
    Depth24PlusStencil8,
    Depth32Float
};

enum class GPUTextureDimension {
    D1 = 1,
    D2 = 2,
    D3 = 3
};

struct GPUTextureDescriptor {
    std::string label;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depthOrArrayLayers = 1;
    uint32_t mipLevelCount = 1;
    uint32_t sampleCount = 1;
    GPUTextureDimension dimension = GPUTextureDimension::D2;
    GPUTextureFormat format = GPUTextureFormat::RGBA8Unorm;
    uint32_t usage = 0;
};

class GPUTexture : public GPUObjectBase {
public:
    GPUTexture(const GPUTextureDescriptor& desc);
    ~GPUTexture();
    
    void destroy();
    
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    GPUTextureFormat format() const { return format_; }
    
private:
    uint32_t width_ = 1;
    uint32_t height_ = 1;
    GPUTextureFormat format_;
};

// =============================================================================
// GPUShaderModule
// =============================================================================

struct GPUShaderModuleDescriptor {
    std::string label;
    std::string code;  // WGSL shader code
};

class GPUShaderModule : public GPUObjectBase {
public:
    GPUShaderModule(const GPUShaderModuleDescriptor& desc);
    ~GPUShaderModule() = default;
    
    const std::string& code() const { return code_; }
    bool hasCompilationErrors() const { return !errors_.empty(); }
    const std::vector<std::string>& errors() const { return errors_; }
    
private:
    std::string code_;
    std::vector<std::string> errors_;
};

// =============================================================================
// GPUCommandEncoder
// =============================================================================

class GPUCommandEncoder : public GPUObjectBase {
public:
    GPUCommandEncoder();
    ~GPUCommandEncoder() = default;
    
    void beginRenderPass(/* GPURenderPassDescriptor */);
    void beginComputePass(/* GPUComputePassDescriptor */);
    void copyBufferToBuffer(GPUBuffer* src, uint64_t srcOffset,
                            GPUBuffer* dst, uint64_t dstOffset, uint64_t size);
    void copyBufferToTexture(/* ... */);
    void copyTextureToBuffer(/* ... */);
    void copyTextureToTexture(/* ... */);
    
    std::unique_ptr<GPUCommandBuffer> finish();
};

// =============================================================================
// GPUCommandBuffer
// =============================================================================

class GPUCommandBuffer : public GPUObjectBase {
public:
    GPUCommandBuffer() = default;
    ~GPUCommandBuffer() = default;
};

// =============================================================================
// GPUQueue
// =============================================================================

class GPUQueue : public GPUObjectBase {
public:
    GPUQueue() = default;
    ~GPUQueue() = default;
    
    void submit(const std::vector<GPUCommandBuffer*>& commandBuffers);
    void writeBuffer(GPUBuffer* buffer, uint64_t offset, const void* data, size_t size);
    void writeTexture(/* ... */);
    void onSubmittedWorkDone(std::function<void()> callback);
};

// =============================================================================
// GPURenderPipeline
// =============================================================================

struct GPUVertexState {
    GPUShaderModule* module = nullptr;
    std::string entryPoint = "main";
};

struct GPUFragmentState {
    GPUShaderModule* module = nullptr;
    std::string entryPoint = "main";
};

struct GPURenderPipelineDescriptor {
    std::string label;
    GPUPipelineLayout* layout = nullptr;
    GPUVertexState vertex;
    GPUFragmentState fragment;
};

class GPURenderPipeline : public GPUObjectBase {
public:
    GPURenderPipeline(const GPURenderPipelineDescriptor& desc);
    ~GPURenderPipeline() = default;
};

// =============================================================================
// GPUComputePipeline
// =============================================================================

struct GPUProgrammableStage {
    GPUShaderModule* module = nullptr;
    std::string entryPoint = "main";
};

struct GPUComputePipelineDescriptor {
    std::string label;
    GPUPipelineLayout* layout = nullptr;
    GPUProgrammableStage compute;
};

class GPUComputePipeline : public GPUObjectBase {
public:
    GPUComputePipeline(const GPUComputePipelineDescriptor& desc);
    ~GPUComputePipeline() = default;
};

// =============================================================================
// GPUDevice
// =============================================================================

struct GPUDeviceLimits {
    uint32_t maxTextureDimension1D = 8192;
    uint32_t maxTextureDimension2D = 8192;
    uint32_t maxTextureDimension3D = 2048;
    uint32_t maxTextureArrayLayers = 256;
    uint32_t maxBindGroups = 4;
    uint32_t maxBufferSize = 1 << 28;  // 256MB
    uint32_t maxUniformBufferBindingSize = 65536;
    uint32_t maxStorageBufferBindingSize = 1 << 28;
    uint32_t maxComputeWorkgroupSizeX = 256;
    uint32_t maxComputeWorkgroupSizeY = 256;
    uint32_t maxComputeWorkgroupSizeZ = 64;
    uint32_t maxComputeInvocationsPerWorkgroup = 256;
    uint32_t maxComputeWorkgroupsPerDimension = 65535;
};

class GPUDevice : public GPUObjectBase {
public:
    GPUDevice();
    ~GPUDevice();
    
    // Device info
    const GPUDeviceLimits& limits() const { return limits_; }
    GPUQueue* queue() const { return queue_.get(); }
    
    // Resource creation
    std::unique_ptr<GPUBuffer> createBuffer(const GPUBufferDescriptor& desc);
    std::unique_ptr<GPUTexture> createTexture(const GPUTextureDescriptor& desc);
    std::unique_ptr<GPUShaderModule> createShaderModule(const GPUShaderModuleDescriptor& desc);
    std::unique_ptr<GPUCommandEncoder> createCommandEncoder();
    std::unique_ptr<GPURenderPipeline> createRenderPipeline(const GPURenderPipelineDescriptor& desc);
    std::unique_ptr<GPUComputePipeline> createComputePipeline(const GPUComputePipelineDescriptor& desc);
    
    // Lifecycle
    void destroy();
    
    // Error handling
    using ErrorCallback = std::function<void(const std::string&)>;
    void onUncapturedError(ErrorCallback callback) { errorCallback_ = callback; }
    
private:
    GPUDeviceLimits limits_;
    std::unique_ptr<GPUQueue> queue_;
    ErrorCallback errorCallback_;
};

// =============================================================================
// GPUAdapter
// =============================================================================

struct GPUAdapterInfo {
    std::string vendor;
    std::string architecture;
    std::string device;
    std::string description;
};

class GPUAdapter : public GPUObjectBase {
public:
    GPUAdapter();
    ~GPUAdapter() = default;
    
    // Adapter info
    const GPUAdapterInfo& info() const { return info_; }
    bool isFallbackAdapter() const { return isFallback_; }
    
    // Device request
    std::unique_ptr<GPUDevice> requestDevice();
    
private:
    GPUAdapterInfo info_;
    bool isFallback_ = false;
};

// =============================================================================
// GPU (navigator.gpu)
// =============================================================================

class GPU {
public:
    static GPU& instance();
    
    // Adapter request (async in spec, sync here for simplicity)
    std::unique_ptr<GPUAdapter> requestAdapter();
    
    // Feature detection
    bool isSupported() const;
    
    // Preferred canvas format
    GPUTextureFormat getPreferredCanvasFormat() const {
        return GPUTextureFormat::BGRA8Unorm;
    }
    
private:
    GPU() = default;
};

} // namespace Zepra::WebGPU

#endif // WEBGPU_CONTEXT_HPP
