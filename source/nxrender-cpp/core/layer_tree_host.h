// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "compositor.h"
#include "damage_tracker.h"
#include "tile_manager.h"
#include <memory>
#include <vector>

namespace NXRender {

class LayerTreeHost {
public:
    LayerTreeHost();
    ~LayerTreeHost();

    void setRootLayer(std::shared_ptr<Layer> root);
    std::shared_ptr<Layer> rootLayer() const { return rootLayer_; }

    void setViewportSize(int width, int height);

    // Forces a full commit of layer tree changes to the compositor thread structures
    void commit();

    // Renders the committed tree using damage tracking and tile caching
    void render(GpuContext* ctx);

private:
    std::shared_ptr<Layer> rootLayer_;
    
    DamageTracker frameDamage_;
    TileManager tileManager_;

    int viewportWidth_ = 0;
    int viewportHeight_ = 0;

    // Recursive resolution
    void computeLayerDamageRecursive(Layer* layer, const Rect& parentClip);
    void rasterizeLayerRecursive(Layer* layer, GpuContext* ctx, const Rect& clipRect);
    void sortLayersByZIndex(std::vector<Layer*>& layerList);
};

} // namespace NXRender
