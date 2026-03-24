/**
 * @file render_tree.cpp
 * @brief Render tree node implementation — layout and paint
 */

#include "rendering/render_tree.hpp"
#include <algorithm>
#include <iostream>
#include <cstdlib>

namespace Zepra::WebCore {

// ============================================================================
// Color
// ============================================================================

Color Color::fromHex(const std::string& hex) {
    Color c;
    if (hex.empty()) return c;
    const char* s = hex.c_str();
    if (*s == '#') s++;
    unsigned long v = strtoul(s, nullptr, 16);
    size_t len = strlen(s);
    if (len == 6) {
        c.r = (v >> 16) & 0xFF;
        c.g = (v >> 8)  & 0xFF;
        c.b = v & 0xFF;
        c.a = 255;
    } else if (len == 8) {
        c.r = (v >> 24) & 0xFF;
        c.g = (v >> 16) & 0xFF;
        c.b = (v >> 8)  & 0xFF;
        c.a = v & 0xFF;
    } else if (len == 3) {
        c.r = ((v >> 8) & 0xF) * 17;
        c.g = ((v >> 4) & 0xF) * 17;
        c.b = (v & 0xF) * 17;
        c.a = 255;
    }
    return c;
}

Color Color::fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return {r, g, b, a};
}

// ============================================================================
// Rect
// ============================================================================

bool Rect::contains(float px, float py) const {
    return px >= x && px <= x + width && py >= y && py <= y + height;
}

bool Rect::intersects(const Rect& other) const {
    return !(x + width < other.x || other.x + other.width < x ||
             y + height < other.y || other.y + other.height < y);
}

Rect Rect::intersected(const Rect& other) const {
    float ix = std::max(x, other.x);
    float iy = std::max(y, other.y);
    float iw = std::min(x + width, other.x + other.width) - ix;
    float ih = std::min(y + height, other.y + other.height) - iy;
    if (iw < 0 || ih < 0) return {0, 0, 0, 0};
    return {ix, iy, iw, ih};
}

// ============================================================================
// BoxModel
// ============================================================================

Rect BoxModel::paddingBox() const {
    return {
        contentBox.x - padding.left,
        contentBox.y - padding.top,
        contentBox.width + padding.horizontal(),
        contentBox.height + padding.vertical()
    };
}

Rect BoxModel::borderBox() const {
    Rect pb = paddingBox();
    return {
        pb.x - border.left,
        pb.y - border.top,
        pb.width + border.horizontal(),
        pb.height + border.vertical()
    };
}

Rect BoxModel::marginBox() const {
    Rect bb = borderBox();
    return {
        bb.x - margin.left,
        bb.y - margin.top,
        bb.width + margin.horizontal(),
        bb.height + margin.vertical()
    };
}

// ============================================================================
// RenderNode
// ============================================================================

RenderNode::RenderNode() {}
RenderNode::~RenderNode() {}

void RenderNode::appendChild(std::unique_ptr<RenderNode> child) {
    child->parent_ = this;
    children_.push_back(std::move(child));
    setNeedsLayout();
}

void RenderNode::removeChild(RenderNode* child) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (it->get() == child) {
            child->parent_ = nullptr;
            children_.erase(it);
            setNeedsLayout();
            return;
        }
    }
}

void RenderNode::layout(float containerWidth) {
    // Base: content box fills container minus margins
    auto& bm = mutableBoxModel();
    float availWidth = containerWidth - style_.margin.horizontal() - 
                       style_.padding.horizontal() - style_.borderWidth.horizontal();
    
    bm.margin = style_.margin;
    bm.padding = style_.padding;
    bm.border = style_.borderWidth;
    bm.contentBox.width = style_.autoWidth ? availWidth : style_.width;
    
    // Layout children vertically (block flow)
    float childY = 0;
    for (auto& child : children_) {
        child->layout(bm.contentBox.width);
        auto& cm = const_cast<BoxModel&>(child->boxModel());
        cm.contentBox.x = child->style().margin.left + child->style().padding.left;
        cm.contentBox.y = childY + child->style().margin.top + child->style().padding.top;
        childY += child->boxModel().marginBox().height;
    }
    
    bm.contentBox.height = style_.autoHeight ? childY : style_.height;
    clearNeedsLayout();
}

void RenderNode::paint(PaintContext& ctx) {
    for (auto& child : children_) {
        child->paint(ctx);
    }
    clearNeedsPaint();
}

RenderNode* RenderNode::hitTest(float x, float y) {
    // Check children in reverse order (front-to-back)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if (auto* hit = (*it)->hitTest(x, y)) return hit;
    }
    if (boxModel_.borderBox().contains(x, y)) return this;
    return nullptr;
}

void RenderNode::dump(int indent) const {
    std::string pad(indent * 2, ' ');
    auto& bb = boxModel_.borderBox();
    std::cerr << pad << (isText() ? "TEXT" : "BOX") 
              << " [" << bb.x << "," << bb.y << " " << bb.width << "x" << bb.height << "]"
              << std::endl;
    for (auto& child : children_) child->dump(indent + 1);
}

// ============================================================================
// RenderBlock
// ============================================================================

void RenderBlock::layout(float containerWidth) {
    RenderNode::layout(containerWidth);
}

void RenderBlock::paint(PaintContext& ctx) {
    RenderNode::paint(ctx);
}

// ============================================================================
// RenderInline
// ============================================================================

void RenderInline::layout(float containerWidth) {
    // Inline: size from content, no block flow
    auto& bm = mutableBoxModel();
    bm.margin = style().margin;
    bm.padding = style().padding;
    bm.border = style().borderWidth;
    
    float xOff = 0;
    for (auto& child : children_) {
        child->layout(containerWidth);
        auto& cm = const_cast<BoxModel&>(child->boxModel());
        cm.contentBox.x = xOff;
        xOff += child->boxModel().marginBox().width;
    }
    bm.contentBox.width = xOff;
    bm.contentBox.height = style().autoHeight ? style().fontSize : style().height;
    clearNeedsLayout();
}

void RenderInline::paint(PaintContext& ctx) {
    RenderNode::paint(ctx);
}

// ============================================================================
// RenderText
// ============================================================================

RenderText::RenderText(const std::string& text) : text_(text) {}

void RenderText::layout(float containerWidth) {
    auto& bm = mutableBoxModel();
    // Approximate: 0.6 * fontSize per char
    float charWidth = style().fontSize * 0.6f;
    float textWidth = text_.length() * charWidth;
    
    lineBoxes_.clear();
    
    if (textWidth <= containerWidth) {
        bm.contentBox.width = textWidth;
        bm.contentBox.height = style().fontSize * 1.4f;
        lineBoxes_.push_back({0, 0, textWidth, bm.contentBox.height});
    } else {
        // Word wrap
        float x = 0, y = 0;
        float lineHeight = style().fontSize * 1.4f;
        size_t lineStart = 0;
        
        for (size_t i = 0; i <= text_.size(); i++) {
            bool isBreak = (i == text_.size()) || (text_[i] == ' ');
            if (isBreak) {
                float wordWidth = (i - lineStart) * charWidth;
                if (x + wordWidth > containerWidth && x > 0) {
                    lineBoxes_.push_back({0, y, x, lineHeight});
                    y += lineHeight;
                    x = 0;
                }
                x += wordWidth;
                lineStart = i + 1;
            }
        }
        if (x > 0) lineBoxes_.push_back({0, y, x, lineHeight});
        
        bm.contentBox.width = containerWidth;
        bm.contentBox.height = lineBoxes_.empty() ? lineHeight : 
            lineBoxes_.back().y + lineBoxes_.back().height;
    }
    clearNeedsLayout();
}

void RenderText::paint(PaintContext& ctx) {
    clearNeedsPaint();
}

// ============================================================================
// RenderImage
// ============================================================================

RenderImage::RenderImage(const std::string& src) : src_(src) {}

void RenderImage::layout(float containerWidth) {
    auto& bm = mutableBoxModel();
    bm.margin = style().margin;
    bm.padding = style().padding;
    
    if (!style().autoWidth) {
        bm.contentBox.width = style().width;
    } else if (naturalWidth_ > 0) {
        bm.contentBox.width = std::min(static_cast<float>(naturalWidth_), containerWidth);
    } else {
        bm.contentBox.width = 300; // placeholder
    }
    
    if (!style().autoHeight) {
        bm.contentBox.height = style().height;
    } else if (naturalHeight_ > 0 && naturalWidth_ > 0) {
        float scale = bm.contentBox.width / naturalWidth_;
        bm.contentBox.height = naturalHeight_ * scale;
    } else {
        bm.contentBox.height = 150; // placeholder
    }
    clearNeedsLayout();
}

void RenderImage::paint(PaintContext& ctx) {
    clearNeedsPaint();
}

// ============================================================================
// RenderVideo
// ============================================================================

RenderVideo::RenderVideo() {}
RenderVideo::~RenderVideo() {}

void RenderVideo::play() { playing_ = true; paused_ = false; }
void RenderVideo::pause() { paused_ = true; playing_ = false; }
void RenderVideo::stop() { playing_ = false; paused_ = true; currentTime_ = 0; }
void RenderVideo::seek(double seconds) { currentTime_ = seconds; }
double RenderVideo::currentTime() const { return currentTime_; }
double RenderVideo::duration() const { return duration_; }
void RenderVideo::update() {}

void RenderVideo::layout(float containerWidth) {
    auto& bm = mutableBoxModel();
    bm.contentBox.width = style().autoWidth ? 
        std::min(static_cast<float>(videoWidth_ > 0 ? videoWidth_ : 640), containerWidth) : 
        style().width;
    bm.contentBox.height = style().autoHeight ? 
        (videoHeight_ > 0 ? static_cast<float>(videoHeight_) : bm.contentBox.width * 9.0f / 16.0f) : 
        style().height;
    clearNeedsLayout();
}

void RenderVideo::paint(PaintContext& ctx) {
    clearNeedsPaint();
}

// ============================================================================
// RenderAudio
// ============================================================================

RenderAudio::RenderAudio() {}
void RenderAudio::play() { playing_ = true; }
void RenderAudio::pause() { playing_ = false; }

void RenderAudio::layout(float containerWidth) {
    auto& bm = mutableBoxModel();
    bm.contentBox.width = style().autoWidth ? std::min(300.0f, containerWidth) : style().width;
    bm.contentBox.height = showControls_ ? 54.0f : 0;
    clearNeedsLayout();
}

void RenderAudio::paint(PaintContext& ctx) {
    clearNeedsPaint();
}

} // namespace Zepra::WebCore
