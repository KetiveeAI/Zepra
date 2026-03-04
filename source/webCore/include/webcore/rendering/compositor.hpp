/**
 * @file compositor.hpp
 * @brief Layer-based compositing system for hardware-accelerated rendering
 */

#pragma once

#include "render_tree.hpp"
#include "paint_context.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

namespace Zepra::WebCore {

/**
 * @brief Reasons why an element might need its own composited layer
 */
enum class LayerReason {
    None = 0,
    Root = 1 << 0,              // Document root
    Transform3D = 1 << 1,       // Has 3D transform
    FixedPosition = 1 << 2,     // position: fixed
    WillChange = 1 << 3,        // will-change property
    Video = 1 << 4,             // Video element
    Canvas = 1 << 5,            // Canvas element
    Opacity = 1 << 6,           // opacity < 1
    Filter = 1 << 7,            // Has CSS filter
    BackdropFilter = 1 << 8,    // Has backdrop-filter
    Overflow = 1 << 9,          // overflow: scroll/auto
    ZIndex = 1 << 10,           // Explicit z-index
};

/**
 * @brief A compositing layer backed by GPU texture
 */
class CompositorLayer {
public:
    CompositorLayer(RenderNode* owner = nullptr);
    ~CompositorLayer();
    
    // Layer tree
    CompositorLayer* parent() const { return parent_; }
    const std::vector<std::unique_ptr<CompositorLayer>>& children() const { return children_; }
    void appendChild(std::unique_ptr<CompositorLayer> child);
    void removeChild(CompositorLayer* child);
    
    // Backing store
    uint32_t textureId() const { return textureId_; }
    void setTextureId(uint32_t id) { textureId_ = id; }
    bool needsRepaint() const { return needsRepaint_; }
    void setNeedsRepaint() { needsRepaint_ = true; }
    void clearNeedsRepaint() { needsRepaint_ = false; }
    
    // Bounds
    Rect bounds() const { return bounds_; }
    void setBounds(const Rect& r) { bounds_ = r; }
    
    // Transform (for GPU composition)
    const float* transform() const { return transform_; }
    void setTransform(const float m[6]);
    void setTranslation(float x, float y);
    
    // Opacity
    float opacity() const { return opacity_; }
    void setOpacity(float o) { opacity_ = o; }
    
    // Layer reason
    uint32_t layerReasons() const { return reasons_; }
    void addLayerReason(LayerReason r) { reasons_ |= static_cast<uint32_t>(r); }
    bool hasLayerReason(LayerReason r) const { return reasons_ & static_cast<uint32_t>(r); }
    
    // Scroll offset (for overflow layers)
    float scrollX() const { return scrollX_; }
    float scrollY() const { return scrollY_; }
    void setScroll(float x, float y) { scrollX_ = x; scrollY_ = y; }
    
    // Owner render node
    RenderNode* owner() const { return owner_; }
    
    // Display list for this layer
    DisplayList& displayList() { return displayList_; }
    const DisplayList& displayList() const { return displayList_; }
    
private:
    CompositorLayer* parent_ = nullptr;
    std::vector<std::unique_ptr<CompositorLayer>> children_;
    
    RenderNode* owner_ = nullptr;
    uint32_t textureId_ = 0;
    Rect bounds_;
    float transform_[6] = {1, 0, 0, 1, 0, 0};
    float opacity_ = 1.0f;
    uint32_t reasons_ = 0;
    float scrollX_ = 0, scrollY_ = 0;
    bool needsRepaint_ = true;
    DisplayList displayList_;
};

/**
 * @brief Manages the layer tree and compositing pipeline
 */
class Compositor {
public:
    Compositor();
    ~Compositor();
    
    /**
     * @brief Set the render backend for GPU operations
     */
    void setBackend(RenderBackend* backend) { backend_ = backend; }
    
    /**
     * @brief Build layer tree from render tree
     */
    void buildLayerTree(RenderNode* root);
    
    /**
     * @brief Update layer positions from layout
     */
    void updateLayerPositions();
    
    /**
     * @brief Paint dirty layers
     */
    void paintDirtyLayers();
    
    /**
     * @brief Composite all layers to screen
     */
    void composite();
    
    /**
     * @brief Full frame: paint + composite
     */
    void frame();
    
    /**
     * @brief Get root layer
     */
    CompositorLayer* rootLayer() const { return rootLayer_.get(); }
    
    /**
     * @brief Handle scroll on layer
     */
    void scrollLayer(CompositorLayer* layer, float dx, float dy);
    
    /**
     * @brief Invalidate layer for repaint
     */
    void invalidateLayer(CompositorLayer* layer);
    
    /**
     * @brief Find layer at point
     */
    CompositorLayer* layerAtPoint(float x, float y);
    
    // Stats
    size_t layerCount() const { return layerCount_; }
    size_t textureMemory() const { return textureMemory_; }
    
private:
    // Build layer recursively
    void buildLayerRecursive(RenderNode* node, CompositorLayer* parent);
    
    // Check if node needs own layer
    bool needsCompositedLayer(RenderNode* node) const;
    
    // Paint a single layer
    void paintLayer(CompositorLayer* layer);
    
    // Composite layer tree
    void compositeLayer(CompositorLayer* layer, float parentOpacity = 1.0f);
    
    // Allocate GPU resources
    uint32_t allocateTexture(int width, int height);
    void freeTexture(uint32_t id);
    
    RenderBackend* backend_ = nullptr;
    std::unique_ptr<CompositorLayer> rootLayer_;
    size_t layerCount_ = 0;
    size_t textureMemory_ = 0;
};

/**
 * @brief Stacking context for z-index ordering
 */
class StackingContext {
public:
    struct Entry {
        RenderNode* node;
        int zIndex;
        CompositorLayer* layer;
    };
    
    void add(RenderNode* node, int zIndex, CompositorLayer* layer = nullptr);
    void sort();
    const std::vector<Entry>& entries() const { return entries_; }
    void clear() { entries_.clear(); }
    
private:
    std::vector<Entry> entries_;
};

} // namespace Zepra::WebCore
