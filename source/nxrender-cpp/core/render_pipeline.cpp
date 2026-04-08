// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "core/render_pipeline.h"
#include "nxgfx/context.h"
#include "platform/display_info.h"
#include <algorithm>
#include <chrono>

namespace NXRender {

RenderPipeline::RenderPipeline() : frameArena_(128 * 1024) {
    layers_.reserve(32);
    visibleLayers_.reserve(32);
    damageRects_.reserve(16);
}

RenderPipeline::~RenderPipeline() {
    shutdown();
}

void RenderPipeline::init(GpuContext* ctx) {
    ctx_ = ctx;
    scheduler_.setTargetFrameRate(60.0f);
    ResourceCache::instance().init(ctx);
}

void RenderPipeline::shutdown() {
    layers_.clear();
    visibleLayers_.clear();
    damageRects_.clear();
    ResourceCache::instance().shutdown();
    ctx_ = nullptr;
}

uint32_t RenderPipeline::addLayer(const Rect& bounds, int zIndex) {
    uint32_t id = nextLayerId_++;
    PipelineLayer layer;
    layer.id = id;
    layer.bounds = bounds;
    layer.zIndex = zIndex;
    layer.dirty = true;
    layers_.push_back(std::move(layer));
    return id;
}

void RenderPipeline::removeLayer(uint32_t id) {
    layers_.erase(
        std::remove_if(layers_.begin(), layers_.end(),
                       [id](const PipelineLayer& l) { return l.id == id; }),
        layers_.end());
}

void RenderPipeline::invalidateLayer(uint32_t id) {
    for (auto& layer : layers_) {
        if (layer.id == id) {
            layer.dirty = true;
            damageRects_.push_back(layer.bounds);
            break;
        }
    }
}

void RenderPipeline::invalidateRegion(uint32_t layerId, const Rect& region) {
    for (auto& layer : layers_) {
        if (layer.id == layerId) {
            layer.dirty = true;
            damageRects_.push_back(region);
            break;
        }
    }
}

void RenderPipeline::setLayerBounds(uint32_t id, const Rect& bounds) {
    for (auto& layer : layers_) {
        if (layer.id == id) {
            if (layer.bounds.x != bounds.x || layer.bounds.y != bounds.y ||
                layer.bounds.width != bounds.width || layer.bounds.height != bounds.height) {
                damageRects_.push_back(layer.bounds); // Old position
                layer.bounds = bounds;
                layer.dirty = true;
                damageRects_.push_back(bounds); // New position
            }
            break;
        }
    }
}

void RenderPipeline::setLayerZIndex(uint32_t id, int zIndex) {
    for (auto& layer : layers_) {
        if (layer.id == id) { layer.zIndex = zIndex; break; }
    }
}

void RenderPipeline::setLayerOpacity(uint32_t id, float opacity) {
    for (auto& layer : layers_) {
        if (layer.id == id) { layer.opacity = opacity; layer.dirty = true; break; }
    }
}

void RenderPipeline::setLayerOpaque(uint32_t id, bool opaque) {
    for (auto& layer : layers_) {
        if (layer.id == id) { layer.opaque = opaque; break; }
    }
}

void RenderPipeline::setLayerVisible(uint32_t id, bool visible) {
    for (auto& layer : layers_) {
        if (layer.id == id) { layer.visible = visible; layer.dirty = true; break; }
    }
}

DisplayList* RenderPipeline::layerDisplayList(uint32_t id) {
    for (auto& layer : layers_) {
        if (layer.id == id) return &layer.displayList;
    }
    return nullptr;
}

void RenderPipeline::invalidateAll() {
    for (auto& layer : layers_) {
        layer.dirty = true;
    }
}

void RenderPipeline::renderFrame() {
    if (!ctx_) return;

    auto frameStart = std::chrono::steady_clock::now();
    scheduler_.beginFrame();
    ResourceCache::instance().beginFrame();
    frameArena_.reset();

    PipelineStats stats{};
    stats.layerCount = static_cast<uint32_t>(layers_.size());

    // 1. Collect damage
    auto t0 = std::chrono::steady_clock::now();
    collectDamage();
    auto t1 = std::chrono::steady_clock::now();
    stats.collectDamageMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // 2. Cull invisible/off-screen layers
    scheduler_.beginPaint();
    cullLayers();
    stats.visibleLayers = static_cast<uint32_t>(visibleLayers_.size());
    stats.culledLayers = stats.layerCount - stats.visibleLayers;

    // 3. Sort by z-index
    sortLayers();

    // 4. Occlusion culling
    occlusionCull();

    auto t2 = std::chrono::steady_clock::now();
    stats.paintMs = std::chrono::duration<double, std::milli>(t2 - t1).count();

    // 5. Composite — replay display lists
    scheduler_.beginComposite();
    composite();
    scheduler_.endComposite();

    auto t3 = std::chrono::steady_clock::now();
    stats.compositeMs = std::chrono::duration<double, std::milli>(t3 - t2).count();

    // Count display list entries
    for (const auto* layer : visibleLayers_) {
        stats.displayListEntries += static_cast<uint32_t>(layer->displayList.entryCount());
    }

    // End-of-frame cleanup
    ResourceCache::instance().flushDeferred();
    ResourceCache::instance().evictToFitBudget();

    damageRects_.clear();

    scheduler_.endFrame();

    auto frameEnd = std::chrono::steady_clock::now();
    stats.totalMs = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();

    lastStats_ = stats;
}

void RenderPipeline::collectDamage() {
    // Coalesce overlapping damage rects
    if (damageRects_.size() <= 1) return;

    // Simple merge: if union area < 1.5x sum of individual areas, merge
    bool merged = true;
    while (merged) {
        merged = false;
        for (size_t i = 0; i < damageRects_.size() && !merged; i++) {
            for (size_t j = i + 1; j < damageRects_.size() && !merged; j++) {
                Rect& a = damageRects_[i];
                Rect& b = damageRects_[j];

                // Check overlap
                bool overlaps = a.x < b.x + b.width && a.x + a.width > b.x &&
                                a.y < b.y + b.height && a.y + a.height > b.y;

                if (overlaps) {
                    float sumArea = a.width * a.height + b.width * b.height;
                    float minX = std::min(a.x, b.x);
                    float minY = std::min(a.y, b.y);
                    float maxX = std::max(a.x + a.width, b.x + b.width);
                    float maxY = std::max(a.y + a.height, b.y + b.height);
                    float unionArea = (maxX - minX) * (maxY - minY);

                    if (unionArea < sumArea * 1.5f) {
                        a = Rect(minX, minY, maxX - minX, maxY - minY);
                        damageRects_.erase(damageRects_.begin() + static_cast<ptrdiff_t>(j));
                        merged = true;
                    }
                }
            }
        }
    }
}

void RenderPipeline::cullLayers() {
    visibleLayers_.clear();

    const auto& di = DisplayInfo::instance().metrics();
    Rect viewport(0, 0, static_cast<float>(di.windowWidth), static_cast<float>(di.windowHeight));

    for (auto& layer : layers_) {
        if (!layer.visible) continue;
        if (layer.opacity <= 0.001f) continue;

        // Viewport frustum cull
        bool inViewport = layer.bounds.x < viewport.x + viewport.width &&
                          layer.bounds.x + layer.bounds.width > viewport.x &&
                          layer.bounds.y < viewport.y + viewport.height &&
                          layer.bounds.y + layer.bounds.height > viewport.y;

        if (!inViewport) continue;

        visibleLayers_.push_back(&layer);
    }
}

void RenderPipeline::sortLayers() {
    std::sort(visibleLayers_.begin(), visibleLayers_.end(),
              [](const PipelineLayer* a, const PipelineLayer* b) {
                  return a->zIndex < b->zIndex;
              });
}

void RenderPipeline::occlusionCull() {
    if (visibleLayers_.size() <= 1) return;

    // Walk back-to-front. For each opaque layer, check if it fully covers layers below.
    // Build a coverage set as we go backwards.
    for (int i = static_cast<int>(visibleLayers_.size()) - 1; i >= 0; i--) {
        PipelineLayer* top = visibleLayers_[static_cast<size_t>(i)];
        if (!top->opaque || top->opacity < 1.0f) continue;

        // Check if this opaque layer fully covers any layer below it
        for (int j = i - 1; j >= 0; j--) {
            PipelineLayer* bottom = visibleLayers_[static_cast<size_t>(j)];
            if (!bottom->visible) continue;

            // Simple containment check
            if (top->bounds.x <= bottom->bounds.x &&
                top->bounds.y <= bottom->bounds.y &&
                top->bounds.x + top->bounds.width >= bottom->bounds.x + bottom->bounds.width &&
                top->bounds.y + top->bounds.height >= bottom->bounds.y + bottom->bounds.height) {
                bottom->visible = false; // Fully occluded
            }
        }
    }

    // Remove occluded layers
    visibleLayers_.erase(
        std::remove_if(visibleLayers_.begin(), visibleLayers_.end(),
                       [](const PipelineLayer* l) { return !l->visible; }),
        visibleLayers_.end());
}

void RenderPipeline::composite() {
    if (!ctx_ || visibleLayers_.empty()) return;

    for (const auto* layer : visibleLayers_) {
        if (layer->displayList.empty()) continue;

        if (layer->opacity < 1.0f) {
            // TODO: render to offscreen target, blend with opacity
            // For now, just replay directly
        }

        ctx_->pushTransform();
        layer->displayList.replay(ctx_);
        ctx_->popTransform();
    }
}

} // namespace NXRender
