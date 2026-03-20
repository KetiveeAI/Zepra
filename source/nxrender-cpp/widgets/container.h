// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file container.h
 * @brief Container widget for grouping children with layout
 */

#pragma once

#include "widget.h"
#include "../layout/flexbox.h"
#include "../layout/block_layout.h"

namespace NXRender {

/**
 * @brief Layout mode for container
 */
enum class LayoutMode {
    Block,      // Normal flow — stack vertically with margin collapse
    Flex,       // Flexbox layout
    Inline,     // Inline flow (future)
    None        // Hidden — display:none
};

/**
 * @brief Container widget that can hold children with layout
 */
class Container : public Widget {
public:
    Container();
    ~Container() override;
    
    // Layout mode
    LayoutMode layoutMode() const { return layoutMode_; }
    void setLayoutMode(LayoutMode mode) { layoutMode_ = mode; }
    
    // Flex layout (only used when layoutMode == Flex)
    FlexLayout& flexLayout() { return flexLayout_; }
    void setFlexLayout(const FlexLayout& layout) { flexLayout_ = layout; }
    
    // Size constraints from CSS
    float cssWidth() const { return cssWidth_; }
    float cssHeight() const { return cssHeight_; }
    void setCSSWidth(float w) { cssWidth_ = w; }
    void setCSSHeight(float h) { cssHeight_ = h; }
    
    // Border
    float borderWidth() const { return borderWidth_; }
    void setBorderWidth(float w) { borderWidth_ = w; }
    Color borderColor() const { return borderColor_; }
    void setBorderColor(const Color& c) { borderColor_ = c; }
    float borderRadius() const { return borderRadius_; }
    void setBorderRadius(float r) { borderRadius_ = r; }
    
    // Opacity
    float opacity() const { return opacity_; }
    void setOpacity(float o) { opacity_ = o; }
    
    // Overflow
    bool clipChildren() const { return clipChildren_; }
    void setClipChildren(bool clip) { clipChildren_ = clip; }
    
    // Child count
    size_t childCount() const { return children_.size(); }
    
    // Rendering
    void render(GpuContext* ctx) override;
    Size measure(const Size& available) override;
    void layout() override;
    
    // Convenience
    void setPadding(float top, float right, float bottom, float left) {
        padding_ = EdgeInsets{top, right, bottom, left};
    }
    
private:
    LayoutMode layoutMode_ = LayoutMode::Block;
    FlexLayout flexLayout_;
    BlockLayout blockLayout_;
    float cssWidth_ = -1;   // -1 = auto
    float cssHeight_ = -1;
    float borderWidth_ = 0;
    Color borderColor_ = Color::transparent();
    float borderRadius_ = 0;
    float opacity_ = 1.0f;
    bool clipChildren_ = false;
};

} // namespace NXRender
