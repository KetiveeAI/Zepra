// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file compositor.cpp
 * @brief Compositor implementation with layer management
 */

#include "core/compositor.h"
#include "nxgfx/context.h"
#include "widgets/widget.h"
#include <algorithm>

namespace NXRender {

// ==========================================================================
// Layer
// ==========================================================================

Layer::Layer() {}
Layer::~Layer() {}

void Layer::invalidate(const Rect& region) {
    damageRects_.push_back(region);
}

void Layer::invalidateAll() {
    damageRects_.clear();
    damageRects_.push_back(bounds_);
}

// ==========================================================================
// Compositor
// ==========================================================================

Compositor::Compositor() {}
Compositor::~Compositor() { shutdown(); }

bool Compositor::init(GpuContext* ctx) {
    ctx_ = ctx;
    return ctx != nullptr;
}

void Compositor::shutdown() {
    layers_.clear();
    ctx_ = nullptr;
}

Layer* Compositor::createLayer() {
    auto layer = std::make_unique<Layer>();
    Layer* ptr = layer.get();
    layers_.push_back(std::move(layer));
    needsSort_ = true;
    return ptr;
}

void Compositor::destroyLayer(Layer* layer) {
    auto it = std::find_if(layers_.begin(), layers_.end(),
        [layer](const auto& ptr) { return ptr.get() == layer; });
    if (it != layers_.end()) {
        layers_.erase(it);
    }
}

void Compositor::moveLayer(Layer* layer, int newZIndex) {
    layer->setZIndex(newZIndex);
    needsSort_ = true;
}

void Compositor::beginFrame() {
    if (ctx_) {
        ctx_->beginFrame();
    }
}

void Compositor::render() {
    if (!ctx_) return;
    
    // Sort layers if needed
    if (needsSort_) {
        sortLayers();
        needsSort_ = false;
    }
    
    // Clear background
    ctx_->clear(backgroundColor_);
    
    // Render each layer in z-order
    for (auto& layer : layers_) {
        if (layer->isVisible()) {
            renderLayer(layer.get());
        }
    }
}

void Compositor::endFrame() {
    if (ctx_) {
        ctx_->endFrame();
        ctx_->present();
    }
    
    // Clear damage for next frame
    for (auto& layer : layers_) {
        layer->clearDamage();
    }
}

void Compositor::invalidate(const Rect& region) {
    for (auto& layer : layers_) {
        if (layer->bounds().intersects(region)) {
            layer->invalidate(layer->bounds().intersection(region));
        }
    }
}

void Compositor::invalidateAll() {
    for (auto& layer : layers_) {
        layer->invalidateAll();
    }
}

Widget* Compositor::hitTest(float x, float y) {
    // Test layers in reverse z-order (top to bottom)
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
        Layer* layer = it->get();
        if (!layer->isVisible()) continue;
        if (!layer->bounds().contains(x, y)) continue;
        
        if (Widget* root = layer->rootWidget()) {
            if (Widget* hit = root->hitTest(x, y)) {
                return hit;
            }
        }
    }
    return nullptr;
}

void Compositor::sortLayers() {
    std::stable_sort(layers_.begin(), layers_.end(),
        [](const auto& a, const auto& b) {
            return a->zIndex() < b->zIndex();
        });
}

void Compositor::renderLayer(Layer* layer) {
    if (!layer->rootWidget()) return;
    
    // Apply layer transform and opacity
    ctx_->pushTransform();
    ctx_->translate(layer->bounds().x, layer->bounds().y);
    
    // Clip to layer bounds
    ctx_->pushClip(layer->bounds());
    
    // Render widget tree
    layer->rootWidget()->render(ctx_);
    
    ctx_->popClip();
    ctx_->popTransform();
}

} // namespace NXRender
