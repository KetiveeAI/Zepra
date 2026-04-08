// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "layer_tree_host.h"
#include "nxgfx/context.h"
#include "widgets/widget.h"
#include <algorithm>

namespace NXRender {

LayerTreeHost::LayerTreeHost() : tileManager_(256) {}

LayerTreeHost::~LayerTreeHost() {}

void LayerTreeHost::setRootLayer(std::shared_ptr<Layer> root) {
    rootLayer_ = root;
    frameDamage_.addDamage(Rect(0, 0, viewportWidth_, viewportHeight_));
}

void LayerTreeHost::setViewportSize(int width, int height) {
    if (viewportWidth_ == width && viewportHeight_ == height) return;
    viewportWidth_ = width;
    viewportHeight_ = height;
    tileManager_.setBounds(width, height);
    frameDamage_.addDamage(Rect(0, 0, width, height));
}

void LayerTreeHost::computeLayerDamageRecursive(Layer* layer, const Rect& parentClip) {
    if (!layer || !layer->isVisible() || layer->opacity() <= 0.0f) return;

    Rect b1 = layer->bounds();
    Rect b2 = parentClip;
    float rx = std::max(b1.x, b2.x);
    float ry = std::max(b1.y, b2.y);
    float rR = std::min(b1.x + b1.width, b2.x + b2.width);
    float rB = std::min(b1.y + b1.height, b2.y + b2.height);
    Rect layerClip;
    if (rR > rx && rB > ry) layerClip = Rect(rx, ry, rR - rx, rB - ry);
    else layerClip = Rect(0,0,0,0);
    
    if (layerClip.width <= 0 || layerClip.height <= 0) return;

    if (layer->isDirty()) {
        for(const auto& r : layer->damageRects()) {
            Rect absoluteDamage = r;
            absoluteDamage.x += layer->bounds().x;
            absoluteDamage.y += layer->bounds().y;
            
            float ax = std::max(absoluteDamage.x, layerClip.x);
            float ay = std::max(absoluteDamage.y, layerClip.y);
            float aR = std::min(absoluteDamage.x + absoluteDamage.width, layerClip.x + layerClip.width);
            float aB = std::min(absoluteDamage.y + absoluteDamage.height, layerClip.y + layerClip.height);
            
            Rect finalDamage;
            if (aR > ax && aB > ay) finalDamage = Rect(ax, ay, aR - ax, aB - ay);
            else finalDamage = Rect(0,0,0,0);
            
            if (finalDamage.width > 0 && finalDamage.height > 0) {
                frameDamage_.addDamage(finalDamage);
            }
        }
        layer->clearDamage();
    }
    
    // Recurse into true children array
    for (Layer* child : layer->children()) {
        computeLayerDamageRecursive(child, layerClip);
    }
}

void LayerTreeHost::sortLayersByZIndex(std::vector<Layer*>& layerList) {
    std::stable_sort(layerList.begin(), layerList.end(), [](Layer* a, Layer* b) {
        return a->zIndex() < b->zIndex();
    });
}

void LayerTreeHost::commit() {
    if (!rootLayer_) return;

    // Traverse root recursively. For a robust architectural tree we intersect regions
    computeLayerDamageRecursive(rootLayer_.get(), Rect(0, 0, viewportWidth_, viewportHeight_));
    frameDamage_.optimize();

    // Map the resolved bounding boxes of damage directly to visual output tiles
    for(const auto& r : frameDamage_.getDamageRects()) {
        tileManager_.invalidateRect(r);
    }
}

void LayerTreeHost::rasterizeLayerRecursive(Layer* layer, GpuContext* ctx, const Rect& clipRect) {
    if (!layer || !layer->isVisible() || layer->opacity() <= 0.0f) return;

    Rect b1 = layer->bounds();
    Rect b2 = clipRect;
    float rx = std::max(b1.x, b2.x);
    float ry = std::max(b1.y, b2.y);
    float rR = std::min(b1.x + b1.width, b2.x + b2.width);
    float rB = std::min(b1.y + b1.height, b2.y + b2.height);
    Rect nodeRenderBounds;
    if (rR > rx && rB > ry) nodeRenderBounds = Rect(rx, ry, rR - rx, rB - ry);
    else nodeRenderBounds = Rect(0,0,0,0);

    if (nodeRenderBounds.width <= 0 || nodeRenderBounds.height <= 0) return;

    ctx->pushTransform();
    ctx->translate(layer->bounds().x, layer->bounds().y);
    
    if (layer->rootWidget()) {
        layer->rootWidget()->render(ctx);
    }
    
    // Recursively sort and render children
    if (!layer->children().empty()) {
        std::vector<Layer*> sortedLayers = layer->children();
        sortLayersByZIndex(sortedLayers);
        
        // Relative clipping for children
        Rect childrenClip(0, 0, b1.width, b1.height);
        for (Layer* child : sortedLayers) {
            rasterizeLayerRecursive(child, ctx, childrenClip);
        }
    }
    
    ctx->popTransform();
}

void LayerTreeHost::render(GpuContext* ctx) {
    if (!rootLayer_) return;

    if (frameDamage_.hasDamage()) {
        auto damageRects = frameDamage_.getDamageRects();
        for(const auto& damageRegion : damageRects) {
            auto tilesToUpdate = tileManager_.getTilesIntersecting(damageRegion);
            
            for(auto* tile : tilesToUpdate) {
                if (!tile->isReady()) {
                    tile->beginRecord(ctx);
                    
                    // Constrain the render to only what is visible within this exact tile
                    Rect tileBounds = tile->bounds();
                    rasterizeLayerRecursive(rootLayer_.get(), ctx, tileBounds);
                    
                    tile->endRecord(ctx);
                }
            }
        }
        frameDamage_.clear();
    }

    tileManager_.composite(ctx);
}

} // namespace NXRender
