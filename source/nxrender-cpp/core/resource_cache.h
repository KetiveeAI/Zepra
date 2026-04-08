// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file resource_cache.h
 * @brief Central GPU resource cache with LRU eviction and memory budget tracking.
 *
 * Manages textures, render targets, and font atlases. Thread-safe via shared_mutex.
 * Resources are reference-counted; destruction is deferred to end-of-frame to
 * prevent mid-draw GPU object deletion.
 */

#pragma once

#include "../nxgfx/primitives.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <shared_mutex>
#include <functional>

namespace NXRender {

class GpuContext;

enum class ResourceType : uint8_t {
    Texture,
    RenderTarget,
    FontAtlas,
    ShaderProgram
};

enum class PixelFormat : uint8_t {
    RGBA8,
    RGB8,
    R8,       // Single channel (e.g. glyph alpha)
    RG8,      // Two channel (e.g. subpixel glyph)
    RGBA16F,  // HDR
    Depth24Stencil8
};

struct ResourceEntry {
    uint32_t id = 0;
    ResourceType type = ResourceType::Texture;
    PixelFormat format = PixelFormat::RGBA8;
    int width = 0;
    int height = 0;
    size_t sizeBytes = 0;
    uint64_t lastUsedFrame = 0;
    uint32_t refCount = 0;
    std::string name; // Debug label
    bool persistent = false; // Exempt from LRU eviction
};

/**
 * @brief GPU resource lifecycle manager.
 *
 * All texture/render-target creation and destruction goes through this cache.
 * Provides:
 * - LRU eviction when memory budget is exceeded
 * - Deferred destruction (queued to end-of-frame)
 * - Memory usage tracking and reporting
 * - Named resources for debugging
 */
class ResourceCache {
public:
    static constexpr size_t kDefaultBudgetBytes = 256 * 1024 * 1024; // 256 MB

    static ResourceCache& instance();

    void init(GpuContext* ctx, size_t budgetBytes = kDefaultBudgetBytes);
    void shutdown();

    // ======================================================================
    // Texture management
    // ======================================================================

    /**
     * @brief Create a texture from pixel data.
     * @return Texture ID (0 on failure).
     */
    uint32_t createTexture(int width, int height, PixelFormat format,
                           const uint8_t* pixels, const std::string& name = "");

    /**
     * @brief Create a render target (FBO + texture).
     */
    uint32_t createRenderTarget(int width, int height, const std::string& name = "");

    /**
     * @brief Load a texture from file path. Cached by path.
     */
    uint32_t loadTexture(const std::string& path);

    /**
     * @brief Mark a resource as used this frame (prevents LRU eviction).
     */
    void touch(uint32_t id);

    /**
     * @brief Increment reference count.
     */
    void addRef(uint32_t id);

    /**
     * @brief Decrement reference count. Resource is destroyed when refCount hits 0
     * at the next end-of-frame flush.
     */
    void release(uint32_t id);

    /**
     * @brief Mark a resource as persistent (never evicted by LRU).
     */
    void setPersistent(uint32_t id, bool persistent);

    /**
     * @brief Destroy a resource immediately. Use with caution — only safe
     * outside of a render pass.
     */
    void destroyImmediate(uint32_t id);

    // ======================================================================
    // Eviction
    // ======================================================================

    /**
     * @brief Evict least-recently-used resources until memory usage is under budget.
     * Called automatically at end of frame.
     */
    void evictToFitBudget();

    /**
     * @brief Process deferred destruction queue. Called at end of frame.
     */
    void flushDeferred();

    /**
     * @brief Set memory budget in bytes.
     */
    void setBudget(size_t bytes) { budgetBytes_ = bytes; }

    // ======================================================================
    // Queries
    // ======================================================================

    const ResourceEntry* getEntry(uint32_t id) const;
    bool exists(uint32_t id) const;

    size_t totalMemoryUsed() const { return totalMemoryUsed_; }
    size_t budget() const { return budgetBytes_; }
    float memoryUsageRatio() const {
        if (budgetBytes_ == 0) return 0.0f;
        return static_cast<float>(totalMemoryUsed_) / static_cast<float>(budgetBytes_);
    }
    size_t resourceCount() const;
    uint64_t currentFrame() const { return currentFrame_; }

    /**
     * @brief Advance frame counter. Called at start of each frame.
     */
    void beginFrame();

    struct MemoryStats {
        size_t totalUsed;
        size_t budget;
        size_t textureMemory;
        size_t renderTargetMemory;
        size_t fontAtlasMemory;
        uint32_t textureCount;
        uint32_t renderTargetCount;
        uint32_t fontAtlasCount;
        uint32_t evictedThisFrame;
    };

    MemoryStats getStats() const;

private:
    ResourceCache() = default;
    ResourceCache(const ResourceCache&) = delete;
    ResourceCache& operator=(const ResourceCache&) = delete;

    size_t computeTextureSize(int width, int height, PixelFormat format) const;
    uint32_t allocateId();

    GpuContext* ctx_ = nullptr;
    size_t budgetBytes_ = kDefaultBudgetBytes;
    size_t totalMemoryUsed_ = 0;
    uint64_t currentFrame_ = 0;
    uint32_t nextId_ = 1;
    uint32_t evictedThisFrame_ = 0;

    mutable std::shared_mutex mutex_;
    std::unordered_map<uint32_t, ResourceEntry> resources_;
    std::unordered_map<std::string, uint32_t> pathCache_; // file path → texture ID
    std::vector<uint32_t> deferredDestroyQueue_;
};

} // namespace NXRender
