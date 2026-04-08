// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "core/resource_cache.h"
#include "nxgfx/context.h"
#include <algorithm>
#include <mutex>
#include <iostream>

namespace NXRender {

ResourceCache& ResourceCache::instance() {
    static ResourceCache cache;
    return cache;
}

void ResourceCache::init(GpuContext* ctx, size_t budgetBytes) {
    ctx_ = ctx;
    budgetBytes_ = budgetBytes;
    currentFrame_ = 0;
    totalMemoryUsed_ = 0;
    nextId_ = 1;
    evictedThisFrame_ = 0;
}

void ResourceCache::shutdown() {
    std::unique_lock lock(mutex_);

    // Destroy all GPU resources
    for (auto& [id, entry] : resources_) {
        if (ctx_) {
            switch (entry.type) {
                case ResourceType::Texture:
                    ctx_->destroyTexture(entry.id);
                    break;
                case ResourceType::RenderTarget:
                    ctx_->destroyRenderTarget(entry.id);
                    break;
                default:
                    break;
            }
        }
    }

    resources_.clear();
    pathCache_.clear();
    deferredDestroyQueue_.clear();
    totalMemoryUsed_ = 0;
    ctx_ = nullptr;
}

size_t ResourceCache::computeTextureSize(int width, int height, PixelFormat format) const {
    size_t bpp = 4; // bytes per pixel
    switch (format) {
        case PixelFormat::RGBA8:           bpp = 4; break;
        case PixelFormat::RGB8:            bpp = 3; break;
        case PixelFormat::R8:              bpp = 1; break;
        case PixelFormat::RG8:             bpp = 2; break;
        case PixelFormat::RGBA16F:         bpp = 8; break;
        case PixelFormat::Depth24Stencil8: bpp = 4; break;
    }
    return static_cast<size_t>(width) * static_cast<size_t>(height) * bpp;
}

uint32_t ResourceCache::allocateId() {
    return nextId_++;
}

uint32_t ResourceCache::createTexture(int width, int height, PixelFormat format,
                                       const uint8_t* pixels, const std::string& name) {
    if (!ctx_ || width <= 0 || height <= 0) return 0;

    uint32_t gpuId = ctx_->createTexture(width, height, pixels);
    if (gpuId == 0) return 0;

    size_t size = computeTextureSize(width, height, format);

    ResourceEntry entry;
    entry.id = gpuId;
    entry.type = ResourceType::Texture;
    entry.format = format;
    entry.width = width;
    entry.height = height;
    entry.sizeBytes = size;
    entry.lastUsedFrame = currentFrame_;
    entry.refCount = 1;
    entry.name = name;

    {
        std::unique_lock lock(mutex_);
        resources_[gpuId] = entry;
        totalMemoryUsed_ += size;
    }

    return gpuId;
}

uint32_t ResourceCache::createRenderTarget(int width, int height, const std::string& name) {
    if (!ctx_ || width <= 0 || height <= 0) return 0;

    uint32_t gpuId = ctx_->createRenderTarget(width, height);
    if (gpuId == 0) return 0;

    size_t size = computeTextureSize(width, height, PixelFormat::RGBA8);

    ResourceEntry entry;
    entry.id = gpuId;
    entry.type = ResourceType::RenderTarget;
    entry.format = PixelFormat::RGBA8;
    entry.width = width;
    entry.height = height;
    entry.sizeBytes = size;
    entry.lastUsedFrame = currentFrame_;
    entry.refCount = 1;
    entry.name = name;

    {
        std::unique_lock lock(mutex_);
        resources_[gpuId] = entry;
        totalMemoryUsed_ += size;
    }

    return gpuId;
}

uint32_t ResourceCache::loadTexture(const std::string& path) {
    {
        std::shared_lock lock(mutex_);
        auto it = pathCache_.find(path);
        if (it != pathCache_.end()) {
            auto resIt = resources_.find(it->second);
            if (resIt != resources_.end()) {
                resIt->second.lastUsedFrame = currentFrame_;
                resIt->second.refCount++;
                return it->second;
            }
            // Stale cache entry — fall through to reload
        }
    }

    if (!ctx_) return 0;
    uint32_t gpuId = ctx_->loadTexture(path);
    if (gpuId == 0) return 0;

    // Approximate size — we don't know format from loadTexture
    ResourceEntry entry;
    entry.id = gpuId;
    entry.type = ResourceType::Texture;
    entry.format = PixelFormat::RGBA8;
    entry.width = 0; // Unknown from this path
    entry.height = 0;
    entry.sizeBytes = 0; // Will be set when dimensions are known
    entry.lastUsedFrame = currentFrame_;
    entry.refCount = 1;
    entry.name = path;

    {
        std::unique_lock lock(mutex_);
        resources_[gpuId] = entry;
        pathCache_[path] = gpuId;
    }

    return gpuId;
}

void ResourceCache::touch(uint32_t id) {
    std::shared_lock lock(mutex_);
    auto it = resources_.find(id);
    if (it != resources_.end()) {
        it->second.lastUsedFrame = currentFrame_;
    }
}

void ResourceCache::addRef(uint32_t id) {
    std::unique_lock lock(mutex_);
    auto it = resources_.find(id);
    if (it != resources_.end()) {
        it->second.refCount++;
        it->second.lastUsedFrame = currentFrame_;
    }
}

void ResourceCache::release(uint32_t id) {
    std::unique_lock lock(mutex_);
    auto it = resources_.find(id);
    if (it != resources_.end()) {
        if (it->second.refCount > 0) {
            it->second.refCount--;
        }
        if (it->second.refCount == 0 && !it->second.persistent) {
            deferredDestroyQueue_.push_back(id);
        }
    }
}

void ResourceCache::setPersistent(uint32_t id, bool persistent) {
    std::unique_lock lock(mutex_);
    auto it = resources_.find(id);
    if (it != resources_.end()) {
        it->second.persistent = persistent;
    }
}

void ResourceCache::destroyImmediate(uint32_t id) {
    std::unique_lock lock(mutex_);
    auto it = resources_.find(id);
    if (it == resources_.end()) return;

    if (ctx_) {
        switch (it->second.type) {
            case ResourceType::Texture:
                ctx_->destroyTexture(it->second.id);
                break;
            case ResourceType::RenderTarget:
                ctx_->destroyRenderTarget(it->second.id);
                break;
            default:
                break;
        }
    }

    totalMemoryUsed_ -= std::min(totalMemoryUsed_, it->second.sizeBytes);

    // Remove from path cache
    for (auto pit = pathCache_.begin(); pit != pathCache_.end(); ) {
        if (pit->second == id) {
            pit = pathCache_.erase(pit);
        } else {
            ++pit;
        }
    }

    resources_.erase(it);
}

void ResourceCache::evictToFitBudget() {
    std::unique_lock lock(mutex_);

    if (totalMemoryUsed_ <= budgetBytes_) return;

    // Collect eviction candidates: non-persistent, refCount == 0
    struct Candidate {
        uint32_t id;
        uint64_t lastUsed;
        size_t size;
    };

    std::vector<Candidate> candidates;
    for (const auto& [id, entry] : resources_) {
        if (!entry.persistent && entry.refCount == 0) {
            candidates.push_back({id, entry.lastUsedFrame, entry.sizeBytes});
        }
    }

    // Sort by last used frame (oldest first = evict first)
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b) {
                  return a.lastUsed < b.lastUsed;
              });

    size_t evicted = 0;
    for (const auto& candidate : candidates) {
        if (totalMemoryUsed_ <= budgetBytes_) break;

        auto it = resources_.find(candidate.id);
        if (it == resources_.end()) continue;

        if (ctx_) {
            switch (it->second.type) {
                case ResourceType::Texture:
                    ctx_->destroyTexture(it->second.id);
                    break;
                case ResourceType::RenderTarget:
                    ctx_->destroyRenderTarget(it->second.id);
                    break;
                default:
                    break;
            }
        }

        totalMemoryUsed_ -= std::min(totalMemoryUsed_, it->second.sizeBytes);

        // Clean path cache
        for (auto pit = pathCache_.begin(); pit != pathCache_.end(); ) {
            if (pit->second == candidate.id) {
                pit = pathCache_.erase(pit);
            } else {
                ++pit;
            }
        }

        resources_.erase(it);
        evicted++;
    }

    evictedThisFrame_ += static_cast<uint32_t>(evicted);
    if (evicted > 0) {
        std::cout << "[ResourceCache] Evicted " << evicted << " resources, memory: "
                  << (totalMemoryUsed_ / (1024 * 1024)) << " MB / "
                  << (budgetBytes_ / (1024 * 1024)) << " MB" << std::endl;
    }
}

void ResourceCache::flushDeferred() {
    std::unique_lock lock(mutex_);

    for (uint32_t id : deferredDestroyQueue_) {
        auto it = resources_.find(id);
        if (it == resources_.end()) continue;
        if (it->second.refCount > 0) continue; // Re-acquired since queuing

        if (ctx_) {
            switch (it->second.type) {
                case ResourceType::Texture:
                    ctx_->destroyTexture(it->second.id);
                    break;
                case ResourceType::RenderTarget:
                    ctx_->destroyRenderTarget(it->second.id);
                    break;
                default:
                    break;
            }
        }

        totalMemoryUsed_ -= std::min(totalMemoryUsed_, it->second.sizeBytes);

        for (auto pit = pathCache_.begin(); pit != pathCache_.end(); ) {
            if (pit->second == id) {
                pit = pathCache_.erase(pit);
            } else {
                ++pit;
            }
        }

        resources_.erase(it);
    }

    deferredDestroyQueue_.clear();
}

const ResourceEntry* ResourceCache::getEntry(uint32_t id) const {
    std::shared_lock lock(mutex_);
    auto it = resources_.find(id);
    return it != resources_.end() ? &it->second : nullptr;
}

bool ResourceCache::exists(uint32_t id) const {
    std::shared_lock lock(mutex_);
    return resources_.count(id) > 0;
}

size_t ResourceCache::resourceCount() const {
    std::shared_lock lock(mutex_);
    return resources_.size();
}

void ResourceCache::beginFrame() {
    currentFrame_++;
    evictedThisFrame_ = 0;
}

ResourceCache::MemoryStats ResourceCache::getStats() const {
    std::shared_lock lock(mutex_);

    MemoryStats stats{};
    stats.totalUsed = totalMemoryUsed_;
    stats.budget = budgetBytes_;
    stats.evictedThisFrame = evictedThisFrame_;

    for (const auto& [id, entry] : resources_) {
        switch (entry.type) {
            case ResourceType::Texture:
                stats.textureMemory += entry.sizeBytes;
                stats.textureCount++;
                break;
            case ResourceType::RenderTarget:
                stats.renderTargetMemory += entry.sizeBytes;
                stats.renderTargetCount++;
                break;
            case ResourceType::FontAtlas:
                stats.fontAtlasMemory += entry.sizeBytes;
                stats.fontAtlasCount++;
                break;
            default:
                break;
        }
    }

    return stats;
}

} // namespace NXRender
