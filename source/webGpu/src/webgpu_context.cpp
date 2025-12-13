/**
 * @file webgpu_context.cpp
 * @brief WebGPU context implementation
 */

#include "webgpu/webgpu_context.hpp"
#include <iostream>

namespace Zepra::WebGPU {

// =============================================================================
// GPUBuffer Implementation
// =============================================================================

GPUBuffer::GPUBuffer(const GPUBufferDescriptor& desc) 
    : size_(desc.size), usage_(desc.usage) {
    label = desc.label;
    data_.resize(size_);
    if (desc.mappedAtCreation) {
        mapped_ = true;
    }
}

GPUBuffer::~GPUBuffer() {
    destroy();
}

void GPUBuffer::mapAsync(uint32_t mode, uint64_t offset, uint64_t size) {
    // In a real implementation, this would be async
    mapped_ = true;
}

void* GPUBuffer::getMappedRange(uint64_t offset, uint64_t size) {
    if (!mapped_) return nullptr;
    if (offset >= size_) return nullptr;
    return data_.data() + offset;
}

void GPUBuffer::unmap() {
    mapped_ = false;
}

void GPUBuffer::destroy() {
    valid = false;
    data_.clear();
}

// =============================================================================
// GPUTexture Implementation
// =============================================================================

GPUTexture::GPUTexture(const GPUTextureDescriptor& desc)
    : width_(desc.width), height_(desc.height), format_(desc.format) {
    label = desc.label;
}

GPUTexture::~GPUTexture() {
    destroy();
}

void GPUTexture::destroy() {
    valid = false;
}

// =============================================================================
// GPUShaderModule Implementation
// =============================================================================

GPUShaderModule::GPUShaderModule(const GPUShaderModuleDescriptor& desc)
    : code_(desc.code) {
    label = desc.label;
    
    // Basic WGSL validation (placeholder)
    if (code_.empty()) {
        errors_.push_back("Shader code is empty");
    }
    // In real implementation: parse and validate WGSL
}

// =============================================================================
// GPUCommandEncoder Implementation
// =============================================================================

GPUCommandEncoder::GPUCommandEncoder() {
}

void GPUCommandEncoder::beginRenderPass() {
    // Record render pass begin
}

void GPUCommandEncoder::beginComputePass() {
    // Record compute pass begin
}

void GPUCommandEncoder::copyBufferToBuffer(GPUBuffer* src, uint64_t srcOffset,
                                           GPUBuffer* dst, uint64_t dstOffset, 
                                           uint64_t size) {
    if (!src || !dst) return;
    if (srcOffset + size > src->size()) return;
    if (dstOffset + size > dst->size()) return;
    
    // Record copy command
}

std::unique_ptr<GPUCommandBuffer> GPUCommandEncoder::finish() {
    return std::make_unique<GPUCommandBuffer>();
}

// =============================================================================
// GPUQueue Implementation
// =============================================================================

void GPUQueue::submit(const std::vector<GPUCommandBuffer*>& commandBuffers) {
    // Execute command buffers
    for (auto* cb : commandBuffers) {
        if (cb) {
            // Process commands
        }
    }
}

void GPUQueue::writeBuffer(GPUBuffer* buffer, uint64_t offset, 
                           const void* data, size_t size) {
    if (!buffer) return;
    
    void* mapped = buffer->getMappedRange(offset, size);
    if (mapped && data) {
        std::memcpy(mapped, data, size);
    }
}

void GPUQueue::onSubmittedWorkDone(std::function<void()> callback) {
    // In real implementation: track GPU work completion
    if (callback) {
        callback();  // Immediate in software implementation
    }
}

// =============================================================================
// GPURenderPipeline Implementation
// =============================================================================

GPURenderPipeline::GPURenderPipeline(const GPURenderPipelineDescriptor& desc) {
    label = desc.label;
    // Compile and link shaders
}

// =============================================================================
// GPUComputePipeline Implementation
// =============================================================================

GPUComputePipeline::GPUComputePipeline(const GPUComputePipelineDescriptor& desc) {
    label = desc.label;
    // Compile compute shader
}

// =============================================================================
// GPUDevice Implementation
// =============================================================================

GPUDevice::GPUDevice() {
    queue_ = std::make_unique<GPUQueue>();
}

GPUDevice::~GPUDevice() {
    destroy();
}

std::unique_ptr<GPUBuffer> GPUDevice::createBuffer(const GPUBufferDescriptor& desc) {
    return std::make_unique<GPUBuffer>(desc);
}

std::unique_ptr<GPUTexture> GPUDevice::createTexture(const GPUTextureDescriptor& desc) {
    return std::make_unique<GPUTexture>(desc);
}

std::unique_ptr<GPUShaderModule> GPUDevice::createShaderModule(const GPUShaderModuleDescriptor& desc) {
    return std::make_unique<GPUShaderModule>(desc);
}

std::unique_ptr<GPUCommandEncoder> GPUDevice::createCommandEncoder() {
    return std::make_unique<GPUCommandEncoder>();
}

std::unique_ptr<GPURenderPipeline> GPUDevice::createRenderPipeline(const GPURenderPipelineDescriptor& desc) {
    return std::make_unique<GPURenderPipeline>(desc);
}

std::unique_ptr<GPUComputePipeline> GPUDevice::createComputePipeline(const GPUComputePipelineDescriptor& desc) {
    return std::make_unique<GPUComputePipeline>(desc);
}

void GPUDevice::destroy() {
    valid = false;
}

// =============================================================================
// GPUAdapter Implementation
// =============================================================================

GPUAdapter::GPUAdapter() {
    info_.vendor = "Zepra";
    info_.architecture = "Software";
    info_.device = "Zepra WebGPU";
    info_.description = "Zepra Browser WebGPU Implementation";
    isFallback_ = true;  // Software fallback
}

std::unique_ptr<GPUDevice> GPUAdapter::requestDevice() {
    return std::make_unique<GPUDevice>();
}

// =============================================================================
// GPU Implementation
// =============================================================================

GPU& GPU::instance() {
    static GPU instance;
    return instance;
}

std::unique_ptr<GPUAdapter> GPU::requestAdapter() {
    return std::make_unique<GPUAdapter>();
}

bool GPU::isSupported() const {
    return true;  // Software implementation always available
}

} // namespace Zepra::WebGPU
