/**
 * @file compositor.cpp
 * @brief Layer-based compositing — layer tree, dirty paint, GPU composition
 */

#include "rendering/compositor.hpp"
#include <algorithm>
#include <cstring>
#include "../../nxrender-cpp/nxrender_cpp.h" // Hook native backend

namespace Zepra::WebCore {

// ============================================================================
// CompositorLayer
// ============================================================================

CompositorLayer::CompositorLayer(RenderNode* owner) : owner_(owner) {}
CompositorLayer::~CompositorLayer() {}

void CompositorLayer::appendChild(std::unique_ptr<CompositorLayer> child) {
    child->parent_ = this;
    children_.push_back(std::move(child));
}

void CompositorLayer::removeChild(CompositorLayer* child) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (it->get() == child) {
            child->parent_ = nullptr;
            children_.erase(it);
            return;
        }
    }
}

void CompositorLayer::setTransform(const float m[6]) {
    memcpy(transform_, m, sizeof(float) * 6);
}

void CompositorLayer::setTranslation(float x, float y) {
    transform_[0] = 1; transform_[1] = 0;
    transform_[2] = 0; transform_[3] = 1;
    transform_[4] = x; transform_[5] = y;
}

// ============================================================================
// StackingContext
// ============================================================================

void StackingContext::add(RenderNode* node, int zIndex, CompositorLayer* layer) {
    entries_.push_back({node, zIndex, layer});
}

void StackingContext::sort() {
    std::stable_sort(entries_.begin(), entries_.end(),
        [](const Entry& a, const Entry& b) { return a.zIndex < b.zIndex; });
}

// ============================================================================
// Compositor
// ============================================================================

Compositor::Compositor() {}
Compositor::~Compositor() {}

void Compositor::buildLayerTree(RenderNode* root) {
    if (!root) return;
    
    rootLayer_ = std::make_unique<CompositorLayer>(root);
    rootLayer_->addLayerReason(LayerReason::Root);
    rootLayer_->setBounds(root->boxModel().borderBox());
    layerCount_ = 1;
    
    buildLayerRecursive(root, rootLayer_.get());
}

void Compositor::buildLayerRecursive(RenderNode* node, CompositorLayer* parentLayer) {
    for (auto& child : node->children()) {
        if (needsCompositedLayer(child.get())) {
            auto layer = std::make_unique<CompositorLayer>(child.get());
            layer->setBounds(child->boxModel().borderBox());
            layerCount_++;
            
            CompositorLayer* layerPtr = layer.get();
            parentLayer->appendChild(std::move(layer));
            buildLayerRecursive(child.get(), layerPtr);
        } else {
            buildLayerRecursive(child.get(), parentLayer);
        }
    }
}

bool Compositor::needsCompositedLayer(RenderNode* node) const {
    if (!node) return false;
    const auto& style = node->style();
    
    // Fixed position elements
    if (style.position == ComputedStyle::Position::Fixed) return true;
    
    // Explicit z-index on positioned elements
    if (style.position != ComputedStyle::Position::Static && style.zIndex != 0) return true;
    
    // Overflow scroll
    if (style.overflow == ComputedStyle::Overflow::Scroll) return true;
    
    return false;
}

void Compositor::updateLayerPositions() {
    if (!rootLayer_) return;
    
    std::function<void(CompositorLayer*)> update = [&](CompositorLayer* layer) {
        if (layer->owner()) {
            layer->setBounds(layer->owner()->boxModel().borderBox());
            auto bb = layer->bounds();
            layer->setTranslation(bb.x + layer->scrollX(), bb.y + layer->scrollY());
        }
        for (auto& child : layer->children()) {
            update(child.get());
        }
    };
    update(rootLayer_.get());
}

void Compositor::paintDirtyLayers() {
    if (!rootLayer_) return;
    
    std::function<void(CompositorLayer*)> paint = [&](CompositorLayer* layer) {
        if (layer->needsRepaint()) {
            paintLayer(layer);
            layer->clearNeedsRepaint();
        }
        for (auto& child : layer->children()) {
            paint(child.get());
        }
    };
    paint(rootLayer_.get());
}

void Compositor::paintLayer(CompositorLayer* layer) {
    if (!layer || !layer->owner()) return;
    
    layer->displayList().clear();
    PaintContext ctx(layer->displayList());
    layer->owner()->paint(ctx);
}

void Compositor::composite() {
    if (!backend_ || !rootLayer_) return;
    
    compositeLayer(rootLayer_.get(), 1.0f);
    backend_->present();
}

void Compositor::compositeLayer(CompositorLayer* layer, float parentOpacity) {
    if (!backend_ || !layer) return;
    
    float effectiveOpacity = parentOpacity * layer->opacity();
    
    // Execute this layer's display list
    if (!layer->displayList().commands().empty()) {
        backend_->executeDisplayList(layer->displayList());
    }
    
    // Composite children
    for (auto& child : layer->children()) {
        compositeLayer(child.get(), effectiveOpacity);
    }
}

void Compositor::frame() {
    updateLayerPositions();
    paintDirtyLayers();
    composite();
}

void Compositor::scrollLayer(CompositorLayer* layer, float dx, float dy) {
    if (!layer) return;
    layer->setScroll(layer->scrollX() + dx, layer->scrollY() + dy);
    layer->setNeedsRepaint();
}

void Compositor::invalidateLayer(CompositorLayer* layer) {
    if (layer) layer->setNeedsRepaint();
}

CompositorLayer* Compositor::layerAtPoint(float x, float y) {
    if (!rootLayer_) return nullptr;
    
    CompositorLayer* result = nullptr;
    std::function<void(CompositorLayer*)> search = [&](CompositorLayer* layer) {
        if (layer->bounds().contains(x, y)) {
            result = layer;
        }
        for (auto& child : layer->children()) {
            search(child.get());
        }
    };
    search(rootLayer_.get());
    return result;
}

uint32_t Compositor::allocateTexture(int width, int height) {
    auto* ctx = NXRender::gpuContext();
    if (!ctx) return 0;
    
    textureMemory_ += width * height * 4; // RGBA assumption
    // Create native FBO target 
    return ctx->createRenderTarget(width, height);
}

void Compositor::freeTexture(uint32_t id) {
    auto* ctx = NXRender::gpuContext();
    if (ctx && id > 0) {
        ctx->destroyRenderTarget(id);
    }
}

} // namespace Zepra::WebCore
