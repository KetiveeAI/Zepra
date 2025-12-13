/**
 * @file compositor.cpp
 * @brief Compositor implementation
 */

#include "webcore/compositor.hpp"
#include <algorithm>

namespace Zepra::WebCore {

// =============================================================================
// CompositorLayer
// =============================================================================

CompositorLayer::CompositorLayer(RenderNode* owner) : owner_(owner) {}
CompositorLayer::~CompositorLayer() = default;

void CompositorLayer::appendChild(std::unique_ptr<CompositorLayer> child) {
    child->parent_ = this;
    children_.push_back(std::move(child));
}

void CompositorLayer::removeChild(CompositorLayer* child) {
    children_.erase(
        std::remove_if(children_.begin(), children_.end(),
            [child](const auto& c) { return c.get() == child; }),
        children_.end());
}

void CompositorLayer::setTransform(const float m[6]) {
    for (int i = 0; i < 6; ++i) transform_[i] = m[i];
}

void CompositorLayer::setTranslation(float x, float y) {
    transform_[4] = x;
    transform_[5] = y;
}

// =============================================================================
// Compositor
// =============================================================================

Compositor::Compositor() = default;
Compositor::~Compositor() = default;

void Compositor::buildLayerTree(RenderNode* root) {
    if (!root) return;
    
    rootLayer_ = std::make_unique<CompositorLayer>(root);
    rootLayer_->addLayerReason(LayerReason::Root);
    rootLayer_->setBounds({0, 0, root->boxModel().contentBox.width, 
                           root->boxModel().contentBox.height});
    
    layerCount_ = 1;
    
    buildLayerRecursive(root, rootLayer_.get());
}

void Compositor::buildLayerRecursive(RenderNode* node, CompositorLayer* parent) {
    for (auto& child : node->children()) {
        if (needsCompositedLayer(child.get())) {
            // Create new layer
            auto layer = std::make_unique<CompositorLayer>(child.get());
            
            auto& style = child->style();
            auto& box = child->boxModel();
            
            // Set layer properties
            layer->setBounds(box.borderBox());
            layer->setOpacity(1.0f);  // Would read from CSS
            
            if (style.position == ComputedStyle::Position::Fixed) {
                layer->addLayerReason(LayerReason::FixedPosition);
            }
            if (style.overflow == ComputedStyle::Overflow::Scroll ||
                style.overflow == ComputedStyle::Overflow::Auto) {
                layer->addLayerReason(LayerReason::Overflow);
            }
            if (style.zIndex != 0) {
                layer->addLayerReason(LayerReason::ZIndex);
            }
            
            layerCount_++;
            
            CompositorLayer* newLayer = layer.get();
            parent->appendChild(std::move(layer));
            
            // Recurse into new layer
            buildLayerRecursive(child.get(), newLayer);
        } else {
            // Stay in same layer
            buildLayerRecursive(child.get(), parent);
        }
    }
}

bool Compositor::needsCompositedLayer(RenderNode* node) const {
    auto& style = node->style();
    
    // Fixed positioning always composites
    if (style.position == ComputedStyle::Position::Fixed) {
        return true;
    }
    
    // Scrollable elements
    if (style.overflow == ComputedStyle::Overflow::Scroll ||
        style.overflow == ComputedStyle::Overflow::Auto) {
        return true;
    }
    
    // Explicit stacking context
    if (style.zIndex != 0 && 
        style.position != ComputedStyle::Position::Static) {
        return true;
    }
    
    return false;
}

void Compositor::updateLayerPositions() {
    std::function<void(CompositorLayer*)> update = [&](CompositorLayer* layer) {
        if (layer->owner()) {
            auto& box = layer->owner()->boxModel();
            layer->setBounds(box.borderBox());
            layer->setTranslation(box.contentBox.x, box.contentBox.y);
        }
        
        for (auto& child : layer->children()) {
            update(child.get());
        }
    };
    
    if (rootLayer_) {
        update(rootLayer_.get());
    }
}

void Compositor::paintDirtyLayers() {
    std::function<void(CompositorLayer*)> paint = [&](CompositorLayer* layer) {
        if (layer->needsRepaint()) {
            paintLayer(layer);
            layer->clearNeedsRepaint();
        }
        
        for (auto& child : layer->children()) {
            paint(child.get());
        }
    };
    
    if (rootLayer_) {
        paint(rootLayer_.get());
    }
}

void Compositor::paintLayer(CompositorLayer* layer) {
    layer->displayList().clear();
    
    PaintContext ctx(layer->displayList());
    
    // Paint owner and all non-composited descendants
    std::function<void(RenderNode*, bool)> paintNode = [&](RenderNode* node, bool isOwner) {
        if (!isOwner && needsCompositedLayer(node)) {
            return;  // Skip - has own layer
        }
        
        node->paint(ctx);
        
        for (auto& child : node->children()) {
            paintNode(child.get(), false);
        }
    };
    
    if (layer->owner()) {
        paintNode(layer->owner(), true);
    }
    
    // If we have a backend, upload to GPU
    if (backend_ && layer->displayList().size() > 0) {
        if (layer->textureId() == 0) {
            layer->setTextureId(allocateTexture(
                static_cast<int>(layer->bounds().width),
                static_cast<int>(layer->bounds().height)));
        }
    }
}

void Compositor::composite() {
    if (!backend_ || !rootLayer_) return;
    
    compositeLayer(rootLayer_.get(), 1.0f);
    backend_->present();
}

void Compositor::compositeLayer(CompositorLayer* layer, float parentOpacity) {
    float opacity = layer->opacity() * parentOpacity;
    
    // Execute this layer's display list
    if (layer->displayList().size() > 0) {
        backend_->executeDisplayList(layer->displayList());
    }
    
    // Composite children in order
    for (auto& child : layer->children()) {
        compositeLayer(child.get(), opacity);
    }
}

void Compositor::frame() {
    updateLayerPositions();
    paintDirtyLayers();
    composite();
}

void Compositor::scrollLayer(CompositorLayer* layer, float dx, float dy) {
    layer->setScroll(layer->scrollX() + dx, layer->scrollY() + dy);
    layer->setNeedsRepaint();
}

void Compositor::invalidateLayer(CompositorLayer* layer) {
    if (layer) {
        layer->setNeedsRepaint();
    }
}

CompositorLayer* Compositor::layerAtPoint(float x, float y) {
    CompositorLayer* result = nullptr;
    
    std::function<void(CompositorLayer*)> test = [&](CompositorLayer* layer) {
        if (layer->bounds().contains(x, y)) {
            result = layer;
        }
        for (auto& child : layer->children()) {
            test(child.get());
        }
    };
    
    if (rootLayer_) {
        test(rootLayer_.get());
    }
    
    return result;
}

uint32_t Compositor::allocateTexture(int width, int height) {
    if (backend_) {
        textureMemory_ += width * height * 4;  // RGBA
        // Would call backend_->createTexture
    }
    static uint32_t nextId = 1;
    return nextId++;
}

void Compositor::freeTexture(uint32_t) {
    // Would deallocate GPU resources
}

// =============================================================================
// StackingContext
// =============================================================================

void StackingContext::add(RenderNode* node, int zIndex, CompositorLayer* layer) {
    entries_.push_back({node, zIndex, layer});
}

void StackingContext::sort() {
    std::stable_sort(entries_.begin(), entries_.end(),
        [](const Entry& a, const Entry& b) {
            return a.zIndex < b.zIndex;
        });
}

} // namespace Zepra::WebCore
