// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file compositor.h
 * @brief Compositor for layer management and rendering
 */

#pragma once

#include "../nxgfx/primitives.h"
#include "../nxgfx/color.h"
#include <vector>
#include <memory>

namespace NXRender {

class Widget;
class GpuContext;
class Surface;

/**
 * @brief Layer in the compositor
 */
class Layer {
public:
    Layer();
    ~Layer();
    
    // Properties
    int zIndex() const { return zIndex_; }
    void setZIndex(int z) { zIndex_ = z; }
    
    float opacity() const { return opacity_; }
    void setOpacity(float o) { opacity_ = o; }
    
    bool isVisible() const { return visible_; }
    void setVisible(bool v) { visible_ = v; }
    
    const Rect& bounds() const { return bounds_; }
    void setBounds(const Rect& b) { bounds_ = b; }
    
    // Content
    Widget* rootWidget() const { return rootWidget_; }
    void setRootWidget(Widget* widget) { rootWidget_ = widget; }
    
    // Damage tracking
    void invalidate(const Rect& region);
    void invalidateAll();
    bool isDirty() const { return !damageRects_.empty(); }
    const std::vector<Rect>& damageRects() const { return damageRects_; }
    void clearDamage() { damageRects_.clear(); }
    
private:
    int zIndex_ = 0;
    float opacity_ = 1.0f;
    bool visible_ = true;
    Rect bounds_;
    Widget* rootWidget_ = nullptr;
    std::vector<Rect> damageRects_;
};

/**
 * @brief Compositor manages layers and orchestrates rendering
 */
class Compositor {
public:
    Compositor();
    ~Compositor();
    
    // Initialize with GPU context
    bool init(GpuContext* ctx);
    void shutdown();
    
    // Layers
    Layer* createLayer();
    void destroyLayer(Layer* layer);
    void moveLayer(Layer* layer, int newZIndex);
    const std::vector<std::unique_ptr<Layer>>& layers() const { return layers_; }
    
    // Rendering
    void beginFrame();
    void render();
    void endFrame();
    
    // Damage tracking
    void invalidate(const Rect& region);
    void invalidateAll();
    
    // Hit testing
    Widget* hitTest(float x, float y);
    
    // VSync
    bool vsyncEnabled() const { return vsyncEnabled_; }
    void setVsyncEnabled(bool enabled) { vsyncEnabled_ = enabled; }
    
    // Background
    Color backgroundColor() const { return backgroundColor_; }
    void setBackgroundColor(const Color& color) { backgroundColor_ = color; }
    
private:
    void sortLayers();
    void renderLayer(Layer* layer);
    
    GpuContext* ctx_ = nullptr;
    std::vector<std::unique_ptr<Layer>> layers_;
    bool vsyncEnabled_ = true;
    Color backgroundColor_ = Color::white();
    bool needsSort_ = false;
};

} // namespace NXRender
