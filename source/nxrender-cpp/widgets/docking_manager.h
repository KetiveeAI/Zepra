// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "widgets/widget.h"
#include <memory>
#include <vector>

namespace NXRender {
namespace Widgets {

enum class DockSplitType {
    None,
    Horizontal, // Left | Right
    Vertical    // Top / Bottom
};

class DockNode {
public:
    DockNode(Widget* managedWidget = nullptr);
    ~DockNode();

    DockSplitType splitType = DockSplitType::None;
    
    // Valid if splitType != None
    std::unique_ptr<DockNode> childA;
    std::unique_ptr<DockNode> childB;
    float splitRatio = 0.5f; // 0.0 to 1.0 (proportion for childA)

    // Valid if splitType == None
    Widget* managedWidget = nullptr; 
    
    // Recalculated during layout
    Rect cachedBounds;
    
    // Flattens tree searching for hit nodes
    DockNode* hitTest(float x, float y);
};

class DockingManager : public Widget {
public:
    DockingManager();
    ~DockingManager() override;

    void setRoot(std::unique_ptr<DockNode> root);
    
    void render(GpuContext* ctx) override;
    EventResult handleRoutedEvent(const Input::Event& event) override;
    
    void layout() override;

private:
    std::unique_ptr<DockNode> root_;
    
    float splitterThickness_ = 6.0f;
    float splitterHitMargin_ = 4.0f; // Extra margin for mouse grabbing
    
    // Drag state
    DockNode* draggingSplitter_ = nullptr;
    bool draggingHorizontal_ = false;

    void layoutNode(DockNode* node, const Rect& available);
    void renderSplitters(GpuContext* ctx, DockNode* node);
    DockNode* hitTestSplitter(DockNode* node, float x, float y, bool& isHorizontal);
};

} // namespace Widgets
} // namespace NXRender
