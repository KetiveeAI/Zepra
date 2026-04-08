// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "tree_view.h"
#include "nxgfx/context.h"
#include <cmath>
#include <algorithm>

namespace NXRender {
namespace Widgets {

TreeNode::TreeNode(const std::string& labelText) : label(labelText) {}

TreeViewWidget::TreeViewWidget() {
    backgroundColor_ = Color::white();
}

TreeViewWidget::~TreeViewWidget() {}

void TreeViewWidget::setRoot(std::unique_ptr<TreeNode> node) {
    root_ = std::move(node);
    scrollY_ = 0.0f;
}

void TreeViewWidget::toggleNodeSelection(TreeNode* node) {
    if (selectedNode_ && selectedNode_ != node) {
        selectedNode_->selected = false;
    }
    selectedNode_ = node;
    if (selectedNode_) {
        selectedNode_->selected = true;
    }
}

TreeNode* TreeViewWidget::hitTestNode(TreeNode* node, int depth, float localX, float localY, float& currentY) {
    if (!node) return nullptr;

    float nodeY = currentY - scrollY_;
    
    if (nodeY >= 0 && nodeY < bounds_.height) {
        if (localY >= nodeY && localY < nodeY + itemHeight_) {
            // Clicked this exact node
            return node;
        }
    }
    
    currentY += itemHeight_;
    
    if (node->expanded) {
        for (auto& child : node->children()) {
            TreeNode* hit = hitTestNode(child.get(), depth + 1, localX, localY, currentY);
            if (hit) return hit;
        }
    }
    
    return nullptr;
}

EventResult TreeViewWidget::handleRoutedEvent(const Input::Event& event) {
    if (!state_.enabled || !root_) return EventResult::Ignored;

    if (auto scrollEv = dynamic_cast<const Input::ScrollEvent*>(&event)) {
        scrollY_ += scrollEv->deltaY() * 20.0f;
        if (scrollY_ < 0) scrollY_ = 0;
        
        // Rough cumulative bounding check can be cached in a real impl
        if (cumulativeY_ > bounds_.height) {
            float maxY = cumulativeY_ - bounds_.height;
            if (scrollY_ > maxY) scrollY_ = maxY;
        } else {
            scrollY_ = 0.0f;
        }
        return EventResult::NeedsRedraw;
    }
    
    if (auto mouseEv = dynamic_cast<const Input::MouseEvent*>(&event)) {
        float ly = mouseEv->y() - bounds_.y;
        float lx = mouseEv->x() - bounds_.x;
        
        if (mouseEv->type() == Input::EventType::MouseMove) {
            float cy = 0.0f;
            TreeNode* newHover = hitTestNode(root_.get(), 0, lx, ly, cy);
            if (newHover != hoveredNode_) {
                hoveredNode_ = newHover;
                return EventResult::NeedsRedraw;
            }
        } else if (mouseEv->type() == Input::EventType::MouseLeave) {
            hoveredNode_ = nullptr;
            return EventResult::NeedsRedraw;
        } else if (mouseEv->type() == Input::EventType::MouseDown && mouseEv->button() == 0) {
            float cy = 0.0f;
            TreeNode* hit = hitTestNode(root_.get(), 0, lx, ly, cy);
            if (hit) {
                // If clicked within the expander box (roughly first 20px)
                float indent = hit == root_.get() ? 0 : 0 * indentWidth_; // Adjust depth manually or pre-calculate
                
                // For simplicity, double click expands, single click selects
                hit->expanded = !hit->expanded;
                toggleNodeSelection(hit);
                return EventResult::NeedsRedraw;
            }
        }
    }

    return Widget::handleRoutedEvent(event);
}

void TreeViewWidget::renderNode(GpuContext* ctx, TreeNode* node, int depth, float& currentY) {
    if (!node) return;

    float nodeY = bounds_.y + currentY - scrollY_;
    
    // Only render if visible
    if (nodeY + itemHeight_ >= bounds_.y && nodeY <= bounds_.y + bounds_.height) {
        Rect nodeRect(bounds_.x, nodeY, bounds_.width, itemHeight_);
        
        if (node == selectedNode_) {
            ctx->fillRect(nodeRect, Color(200, 225, 250, 255));
        } else if (node == hoveredNode_) {
            ctx->fillRect(nodeRect, Color(240, 245, 250, 255));
        }

        float contentX = bounds_.x + (depth * indentWidth_) + 10.0f;
        
        // Draw expander triangle if has children
        if (!node->children().empty()) {
            float triX = contentX;
            float triY = nodeY + (itemHeight_ / 2.0f);
            
            std::vector<Point> tri;
            if (node->expanded) {
                tri = {{triX - 4, triY - 2}, {triX + 4, triY - 2}, {triX, triY + 4}};
            } else {
                tri = {{triX - 2, triY - 4}, {triX - 2, triY + 4}, {triX + 4, triY}};
            }
            ctx->fillPath(tri, Color(100, 100, 100, 255));
        }
        
        // Draw real string text natively aligned
        ctx->drawText(node->label, contentX + 15.0f, nodeY + 16.0f, Color::black(), 14.0f);
    }
    
    currentY += itemHeight_;

    if (node->expanded) {
        for (auto& child : node->children()) {
            renderNode(ctx, child.get(), depth + 1, currentY);
        }
    }
}

void TreeViewWidget::render(GpuContext* ctx) {
    if (!state_.visible) return;

    ctx->fillRect(bounds_, backgroundColor_);
    ctx->strokeRect(bounds_, Color(200, 200, 200, 255), 1.0f);

    ctx->pushClip(bounds_);

    if (root_) {
        float cy = 0.0f;
        renderNode(ctx, root_.get(), 0, cy);
        cumulativeY_ = cy; // Cache total height for scrolling constraints
    }

    ctx->popClip();
    Widget::renderChildren(ctx);
}

} // namespace Widgets
} // namespace NXRender
