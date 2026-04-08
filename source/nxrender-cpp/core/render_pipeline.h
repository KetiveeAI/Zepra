// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file render_pipeline.h
 * @brief Full frame rendering pipeline.
 *
 * Coordinates: damage collection → layer culling → display list recording →
 * batch rendering → composition → present.
 */

#pragma once

#include "display_list.h"
#include "frame_scheduler.h"
#include "resource_cache.h"
#include "memory_pool.h"
#include "../nxgfx/primitives.h"
#include "../platform/display_info.h"
#include <vector>
#include <memory>

namespace NXRender {

class GpuContext;
class Compositor;

/**
 * @brief Layer information for the pipeline.
 */
struct PipelineLayer {
    uint32_t id = 0;
    int zIndex = 0;
    Rect bounds;
    float opacity = 1.0f;
    bool opaque = false;        // Fully opaque (can occlude layers below)
    bool visible = true;
    bool dirty = true;          // Needs repaint
    DisplayList displayList;
};

/**
 * @brief Render pipeline stats for performance monitoring.
 */
struct PipelineStats {
    uint32_t layerCount = 0;
    uint32_t visibleLayers = 0;
    uint32_t culledLayers = 0;
    uint32_t occludedLayers = 0;
    uint32_t dirtyLayers = 0;
    uint32_t displayListEntries = 0;
    double collectDamageMs = 0;
    double paintMs = 0;
    double compositeMs = 0;
    double totalMs = 0;
};

/**
 * @brief Central rendering pipeline.
 *
 * Manages the full frame lifecycle from damage collection through GPU submission.
 */
class RenderPipeline {
public:
    RenderPipeline();
    ~RenderPipeline();

    /**
     * @brief Initialize the pipeline with GPU context and scheduler.
     */
    void init(GpuContext* ctx);

    /**
     * @brief Shutdown and release resources.
     */
    void shutdown();

    /**
     * @brief Add a layer to the pipeline.
     * @return Layer ID.
     */
    uint32_t addLayer(const Rect& bounds, int zIndex = 0);

    /**
     * @brief Remove a layer.
     */
    void removeLayer(uint32_t id);

    /**
     * @brief Mark a layer as dirty (needs repaint).
     */
    void invalidateLayer(uint32_t id);

    /**
     * @brief Invalidate a region within a layer.
     */
    void invalidateRegion(uint32_t layerId, const Rect& region);

    /**
     * @brief Set layer properties.
     */
    void setLayerBounds(uint32_t id, const Rect& bounds);
    void setLayerZIndex(uint32_t id, int zIndex);
    void setLayerOpacity(uint32_t id, float opacity);
    void setLayerOpaque(uint32_t id, bool opaque);
    void setLayerVisible(uint32_t id, bool visible);

    /**
     * @brief Get the display list for a layer (for recording paint commands).
     */
    DisplayList* layerDisplayList(uint32_t id);

    /**
     * @brief Execute one frame.
     * 1. Collect damage
     * 2. Cull invisible/off-screen layers
     * 3. Sort by z-index
     * 4. Perform occlusion culling
     * 5. Replay display lists (back to front)
     * 6. Present
     */
    void renderFrame();

    /**
     * @brief Force a full repaint (all layers dirty).
     */
    void invalidateAll();

    // Stats
    const PipelineStats& lastStats() const { return lastStats_; }
    FrameScheduler& scheduler() { return scheduler_; }

private:
    void collectDamage();
    void cullLayers();
    void sortLayers();
    void occlusionCull();
    void composite();

    GpuContext* ctx_ = nullptr;
    FrameScheduler scheduler_;
    ArenaAllocator frameArena_;

    std::vector<PipelineLayer> layers_;
    std::vector<PipelineLayer*> visibleLayers_; // Sorted by z-index, after culling
    uint32_t nextLayerId_ = 1;

    std::vector<Rect> damageRects_;
    PipelineStats lastStats_;
};

} // namespace NXRender
