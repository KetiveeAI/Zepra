// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "widget.h"
#include "../layout/flexbox.h"
#include "../layout/block_layout.h"

namespace NXRender {

enum class LayoutMode {
    Block,
    Flex,
    Inline,
    None
};

class Container : public Widget {
public:
    Container();
    ~Container() override;

    // Layout mode
    LayoutMode layoutMode() const { return layoutMode_; }
    void setLayoutMode(LayoutMode mode) { layoutMode_ = mode; }

    // Flex layout
    FlexLayout& flexLayout() { return flexLayout_; }
    void setFlexLayout(const FlexLayout& layout) { flexLayout_ = layout; }

    // CSS size constraints
    float cssWidth() const { return cssWidth_; }
    float cssHeight() const { return cssHeight_; }
    void setCSSWidth(float w) { cssWidth_ = w; }
    void setCSSHeight(float h) { cssHeight_ = h; }

    // Min/Max
    void setMinWidth(float w) { minWidth_ = w; }
    void setMaxWidth(float w) { maxWidth_ = w; }
    void setMinHeight(float h) { minHeight_ = h; }
    void setMaxHeight(float h) { maxHeight_ = h; }

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

    // Scrolling
    float scrollOffsetX() const { return scrollOffsetX_; }
    float scrollOffsetY() const { return scrollOffsetY_; }
    void scrollTo(float x, float y);
    void scrollBy(float dx, float dy);
    bool canScrollVertically() const;
    bool canScrollHorizontally() const;
    float scrollProgress() const;

    // Gradient
    void setGradient(const Color& start, const Color& end, bool horizontal = false);
    void clearGradient();

    // Shadow
    void setShadow(const Color& color, float blur, float offsetX = 0, float offsetY = 0);
    void clearShadow();

    // Debug
    void setDebugBorders(bool enabled) { debugBorders_ = enabled; }

    // Child count
    size_t childCount() const { return children_.size(); }

    // Rendering
    void render(GpuContext* ctx) override;
    Size measure(const Size& available) override;
    void layout() override;

    void setPadding(float top, float right, float bottom, float left) {
        padding_ = EdgeInsets{top, right, bottom, left};
    }

private:
    void clampScroll();

    LayoutMode layoutMode_ = LayoutMode::Block;
    FlexLayout flexLayout_;
    BlockLayout blockLayout_;

    float cssWidth_ = -1;
    float cssHeight_ = -1;
    float minWidth_ = 0;
    float maxWidth_ = 0;
    float minHeight_ = 0;
    float maxHeight_ = 0;

    float borderWidth_ = 0;
    Color borderColor_ = Color::transparent();
    float borderRadius_ = 0;
    float opacity_ = 1.0f;
    bool clipChildren_ = false;

    // Scroll
    float scrollOffsetX_ = 0;
    float scrollOffsetY_ = 0;
    float contentWidth_ = 0;
    float contentHeight_ = 0;

    // Gradient
    bool gradientEnabled_ = false;
    Color gradientStart_ = Color::transparent();
    Color gradientEnd_ = Color::transparent();
    bool gradientHorizontal_ = true;

    // Shadow
    Color shadowColor_ = Color::transparent();
    float shadowBlur_ = 0;
    float shadowOffsetX_ = 0;
    float shadowOffsetY_ = 0;

    bool debugBorders_ = false;
};

} // namespace NXRender
