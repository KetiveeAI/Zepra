// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "docking_manager.h"
#include "nxgfx/context.h"

namespace NXRender {
namespace Widgets {

DockNode::DockNode(Widget* widget) : managedWidget(widget) {}

DockNode::~DockNode() {}

DockNode* DockNode::hitTest(float x, float y) {
    if (!cachedBounds.contains(x, y)) return nullptr;
    
    if (splitType == DockSplitType::None) {
        return this;
    }
    
    if (childA) {
        if (DockNode* hit = childA->hitTest(x, y)) return hit;
    }
    if (childB) {
        if (DockNode* hit = childB->hitTest(x, y)) return hit;
    }
    return this;
}

DockingManager::DockingManager() {
    backgroundColor_ = Color(28, 28, 28, 255);
}

DockingManager::~DockingManager() {}

void DockingManager::setRoot(std::unique_ptr<DockNode> root) {
    root_ = std::move(root);
    layout();
}

void DockingManager::layout() {
    if (root_) {
        layoutNode(root_.get(), bounds_);
    }
    Widget::layout();
}

void DockingManager::layoutNode(DockNode* node, const Rect& available) {
    node->cachedBounds = available;
    
    if (node->splitType == DockSplitType::None) {
        if (node->managedWidget) {
            node->managedWidget->setBounds(available);
            node->managedWidget->layout();
        }
        return;
    }
    
    float halfSplit = splitterThickness_ / 2.0f;
    Rect rectA, rectB;
    
    if (node->splitType == DockSplitType::Horizontal) {
        float midX = available.x + (available.width * node->splitRatio);
        rectA = Rect(available.x, available.y, midX - available.x - halfSplit, available.height);
        rectB = Rect(midX + halfSplit, available.y, available.x + available.width - (midX + halfSplit), available.height);
    } else { // Vertical
        float midY = available.y + (available.height * node->splitRatio);
        rectA = Rect(available.x, available.y, available.width, midY - available.y - halfSplit);
        rectB = Rect(available.x, midY + halfSplit, available.width, available.y + available.height - (midY + halfSplit));
    }
    
    if (node->childA) layoutNode(node->childA.get(), rectA);
    if (node->childB) layoutNode(node->childB.get(), rectB);
}

DockNode* DockingManager::hitTestSplitter(DockNode* node, float x, float y, bool& isHorizontal) {
    if (!node || node->splitType == DockSplitType::None) return nullptr;
    
    float halfSplit = splitterThickness_ / 2.0f;
    float grabMargin = splitterHitMargin_;
    
    if (node->splitType == DockSplitType::Horizontal) {
        float midX = node->cachedBounds.x + (node->cachedBounds.width * node->splitRatio);
        if (x >= midX - halfSplit - grabMargin && x <= midX + halfSplit + grabMargin &&
            y >= node->cachedBounds.y && y <= node->cachedBounds.y + node->cachedBounds.height) {
            isHorizontal = true;
            return node;
        }
    } else { // Vertical
        float midY = node->cachedBounds.y + (node->cachedBounds.height * node->splitRatio);
        if (y >= midY - halfSplit - grabMargin && y <= midY + halfSplit + grabMargin &&
            x >= node->cachedBounds.x && x <= node->cachedBounds.x + node->cachedBounds.width) {
            isHorizontal = false;
            return node;
        }
    }
    
    if (DockNode* hit = hitTestSplitter(node->childA.get(), x, y, isHorizontal)) return hit;
    if (DockNode* hit = hitTestSplitter(node->childB.get(), x, y, isHorizontal)) return hit;
    
    return nullptr;
}

EventResult DockingManager::handleRoutedEvent(const Input::Event& event) {
    if (!state_.enabled) return EventResult::Ignored;

    if (auto mouseEv = dynamic_cast<const Input::MouseEvent*>(&event)) {
        float ly = mouseEv->y();
        float lx = mouseEv->x();

        if (mouseEv->type() == Input::EventType::MouseDown && mouseEv->button() == 0) {
            bool isHorizontal = false;
            if (DockNode* hit = hitTestSplitter(root_.get(), lx, ly, isHorizontal)) {
                draggingSplitter_ = hit;
                draggingHorizontal_ = isHorizontal;
                return EventResult::Handled;
            }
        } 
        else if (mouseEv->type() == Input::EventType::MouseUp && mouseEv->button() == 0) {
            if (draggingSplitter_) {
                draggingSplitter_ = nullptr;
                return EventResult::Handled;
            }
        } 
        else if (mouseEv->type() == Input::EventType::MouseMove) {
            if (draggingSplitter_) {
                float ratio = 0.5f;
                if (draggingHorizontal_) {
                    float relX = lx - draggingSplitter_->cachedBounds.x;
                    ratio = relX / draggingSplitter_->cachedBounds.width;
                } else {
                    float relY = ly - draggingSplitter_->cachedBounds.y;
                    ratio = relY / draggingSplitter_->cachedBounds.height;
                }
                
                // Clamp ratio to prevent panes collapsing entirely (10% padding)
                if (ratio < 0.1f) ratio = 0.1f;
                if (ratio > 0.9f) ratio = 0.9f;
                
                draggingSplitter_->splitRatio = ratio;
                layout(); // Recursively reflow
                return EventResult::NeedsRedraw;
            } else {
                // Future cursor update: MouseHover hitting splitter changes cursor to ResizeH/V
                bool isHorizontal = false;
                if (hitTestSplitter(root_.get(), lx, ly, isHorizontal)) {
                    // emit cursor change request up route
                }
            }
        }
    }

    return Widget::handleRoutedEvent(event);
}

void DockingManager::renderSplitters(GpuContext* ctx, DockNode* node) {
    if (!node || node->splitType == DockSplitType::None) return;
    
    float halfSplit = splitterThickness_ / 2.0f;
    Rect splitterRect;
    
    if (node->splitType == DockSplitType::Horizontal) {
        float midX = node->cachedBounds.x + (node->cachedBounds.width * node->splitRatio);
        splitterRect = Rect(midX - halfSplit, node->cachedBounds.y, splitterThickness_, node->cachedBounds.height);
    } else { // Vertical
        float midY = node->cachedBounds.y + (node->cachedBounds.height * node->splitRatio);
        splitterRect = Rect(node->cachedBounds.x, midY - halfSplit, node->cachedBounds.width, splitterThickness_);
    }
    
    // Render splitter body
    ctx->fillRect(splitterRect, Color(45, 45, 45, 255));
    
    renderSplitters(ctx, node->childA.get());
    renderSplitters(ctx, node->childB.get());
}

void DockingManager::render(GpuContext* ctx) {
    if (!state_.visible) return;

    ctx->fillRect(bounds_, backgroundColor_);
    
    // Managed Widgets will be rendered implicitly via the Widget tree rendering pass
    // if DockingManager ensures managed widgets are formally managed via `addChild()`.
    // Instead of overriding everything, the best architecture ensures DockingManager
    // adds views as direct `children_` but manages layout uniquely.
    Widget::renderChildren(ctx);
    
    // Draw splitters exactly on top
    if (root_) {
        renderSplitters(ctx, root_.get());
    }
}

} // namespace Widgets
} // namespace NXRender
