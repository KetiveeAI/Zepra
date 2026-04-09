// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file render_target.h
 * @brief Framebuffer Object (FBO) management for off-screen rendering.
 *
 * Wraps GL framebuffers with color/depth/stencil attachments.
 * Supports MSAA resolve and automatic viewport management.
 */

#pragma once

#include "primitives.h"
#include <cstdint>
#include <vector>

namespace NXRender {

class GpuContext;

enum class RenderTargetFormat : uint8_t {
    RGBA8,
    RGBA16F,
    R8,
    RG8,
    Depth16,
    Depth24Stencil8
};

struct RenderTargetDesc {
    int width = 0;
    int height = 0;
    RenderTargetFormat colorFormat = RenderTargetFormat::RGBA8;
    bool hasDepth = false;
    bool hasStencil = false;
    int msaaSamples = 0; // 0 = no MSAA

    RenderTargetDesc() = default;
    RenderTargetDesc(int w, int h) : width(w), height(h) {}
    RenderTargetDesc(int w, int h, RenderTargetFormat fmt)
        : width(w), height(h), colorFormat(fmt) {}
};

/**
 * @brief GPU framebuffer wrapper.
 */
class RenderTarget {
public:
    RenderTarget();
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    /**
     * @brief Create FBO resources.
     */
    bool create(const RenderTargetDesc& desc);

    /**
     * @brief Destroy FBO resources.
     */
    void destroy();

    /**
     * @brief Bind this FBO as the current render target.
     * Saves the previous FBO and viewport, restores on unbind.
     */
    void bind();

    /**
     * @brief Unbind and restore the previous render target.
     */
    void unbind();

    /**
     * @brief Clear the FBO contents.
     */
    void clear(float r = 0, float g = 0, float b = 0, float a = 0);

    /**
     * @brief Resolve MSAA to a non-MSAA texture for sampling.
     * Only meaningful if msaaSamples > 0.
     */
    void resolve();

    /**
     * @brief Read pixels from the FBO.
     */
    bool readPixels(int x, int y, int width, int height,
                     uint8_t* buffer, int channels = 4) const;

    /**
     * @brief Resize the FBO (destroys and recreates).
     */
    bool resize(int width, int height);

    // Accessors
    int width() const { return desc_.width; }
    int height() const { return desc_.height; }
    uint32_t fboId() const { return fbo_; }
    uint32_t colorTexture() const { return colorTexture_; }
    uint32_t resolvedTexture() const { return resolvedTexture_ ? resolvedTexture_ : colorTexture_; }
    uint32_t depthRenderbuffer() const { return depthRbo_; }
    bool isValid() const { return fbo_ != 0; }
    bool isMultisampled() const { return desc_.msaaSamples > 0; }
    const RenderTargetDesc& desc() const { return desc_; }

    /**
     * @brief Bind the color texture for sampling.
     * @param unit Texture unit (0-15).
     */
    void bindColorTexture(int unit = 0) const;

private:
    bool createAttachments();

    RenderTargetDesc desc_;
    uint32_t fbo_ = 0;
    uint32_t colorTexture_ = 0;
    uint32_t depthRbo_ = 0;
    uint32_t stencilRbo_ = 0;

    // MSAA
    uint32_t msaaFbo_ = 0;
    uint32_t msaaColorRbo_ = 0;
    uint32_t resolvedTexture_ = 0;

    // Previous state for save/restore
    int prevFbo_ = 0;
    int prevViewport_[4] = {};
};

/**
 * @brief Pool of reusable render targets.
 *
 * Avoids allocating/destroying FBOs every frame. Targets are
 * returned to the pool on release and reused if the dimensions match.
 */
class RenderTargetPool {
public:
    static RenderTargetPool& instance();

    /**
     * @brief Acquire a render target with the given dimensions.
     * May return a cached one or create a new one.
     */
    RenderTarget* acquire(int width, int height,
                           RenderTargetFormat format = RenderTargetFormat::RGBA8,
                           int msaa = 0);

    /**
     * @brief Release a render target back to the pool.
     */
    void release(RenderTarget* rt);

    /**
     * @brief Flush all cached targets (call on context loss).
     */
    void flush();

    /**
     * @brief Trim targets that haven't been used for N frames.
     */
    void trim(int maxAge = 60);

    size_t poolSize() const { return pool_.size(); }
    size_t activeCount() const { return activeCount_; }

private:
    RenderTargetPool() = default;

    struct PoolEntry {
        RenderTarget* target = nullptr;
        int lastUsedFrame = 0;
        bool inUse = false;
    };

    std::vector<PoolEntry> pool_;
    int currentFrame_ = 0;
    size_t activeCount_ = 0;
};

} // namespace NXRender
