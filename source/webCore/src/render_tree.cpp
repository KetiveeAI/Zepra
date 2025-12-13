/**
 * @file render_tree.cpp
 * @brief Render tree implementation
 */

#include "webcore/render_tree.hpp"
#include "webcore/paint_context.hpp"
#include <sstream>
#include <iostream>
#include <algorithm>

namespace Zepra::WebCore {

// =============================================================================
// Color
// =============================================================================

Color Color::fromHex(const std::string& hex) {
    Color c;
    if (hex.empty()) return c;
    
    std::string h = hex[0] == '#' ? hex.substr(1) : hex;
    
    if (h.length() == 3) {
        c.r = static_cast<uint8_t>(std::stoi(h.substr(0, 1) + h.substr(0, 1), nullptr, 16));
        c.g = static_cast<uint8_t>(std::stoi(h.substr(1, 1) + h.substr(1, 1), nullptr, 16));
        c.b = static_cast<uint8_t>(std::stoi(h.substr(2, 1) + h.substr(2, 1), nullptr, 16));
    } else if (h.length() >= 6) {
        c.r = static_cast<uint8_t>(std::stoi(h.substr(0, 2), nullptr, 16));
        c.g = static_cast<uint8_t>(std::stoi(h.substr(2, 2), nullptr, 16));
        c.b = static_cast<uint8_t>(std::stoi(h.substr(4, 2), nullptr, 16));
        if (h.length() >= 8) {
            c.a = static_cast<uint8_t>(std::stoi(h.substr(6, 2), nullptr, 16));
        }
    }
    return c;
}

Color Color::fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return {r, g, b, a};
}

// =============================================================================
// Rect
// =============================================================================

bool Rect::contains(float px, float py) const {
    return px >= x && px < x + width && py >= y && py < y + height;
}

bool Rect::intersects(const Rect& other) const {
    return !(x + width <= other.x || other.x + other.width <= x ||
             y + height <= other.y || other.y + other.height <= y);
}

Rect Rect::intersected(const Rect& other) const {
    float nx = std::max(x, other.x);
    float ny = std::max(y, other.y);
    float nx2 = std::min(x + width, other.x + other.width);
    float ny2 = std::min(y + height, other.y + other.height);
    
    if (nx2 <= nx || ny2 <= ny) return {0, 0, 0, 0};
    return {nx, ny, nx2 - nx, ny2 - ny};
}

// =============================================================================
// BoxModel
// =============================================================================

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

// =============================================================================
// RenderNode
// =============================================================================

RenderNode::RenderNode() = default;
RenderNode::~RenderNode() = default;

void RenderNode::appendChild(std::unique_ptr<RenderNode> child) {
    child->parent_ = this;
    children_.push_back(std::move(child));
}

void RenderNode::removeChild(RenderNode* child) {
    auto it = std::find_if(children_.begin(), children_.end(),
        [child](const std::unique_ptr<RenderNode>& c) { return c.get() == child; });
    if (it != children_.end()) {
        (*it)->parent_ = nullptr;
        children_.erase(it);
    }
}

void RenderNode::layout(float containerWidth) {
    // Default layout - just use container width
    boxModel_.contentBox.width = containerWidth;
    boxModel_.contentBox.height = 0;
    
    float y = 0;
    for (auto& child : children_) {
        child->layout(containerWidth);
        child->boxModel_.contentBox.y = y;
        y += child->boxModel_.marginBox().height;
        boxModel_.contentBox.height = y;
    }
}

void RenderNode::paint(PaintContext& ctx) {
    // Paint background
    if (style_.backgroundColor.a > 0) {
        ctx.fillRect(boxModel_.borderBox(), style_.backgroundColor);
    }
    
    // Paint all 4 borders individually
    Rect bb = boxModel_.borderBox();
    const auto& bw = style_.borderWidth;
    const Color& bc = style_.borderColor;
    
    // Top border
    if (bw.top > 0) {
        Rect topBorder = {bb.x, bb.y, bb.width, bw.top};
        ctx.fillRect(topBorder, bc);
    }
    
    // Right border
    if (bw.right > 0) {
        Rect rightBorder = {bb.x + bb.width - bw.right, bb.y, bw.right, bb.height};
        ctx.fillRect(rightBorder, bc);
    }
    
    // Bottom border
    if (bw.bottom > 0) {
        Rect bottomBorder = {bb.x, bb.y + bb.height - bw.bottom, bb.width, bw.bottom};
        ctx.fillRect(bottomBorder, bc);
    }
    
    // Left border
    if (bw.left > 0) {
        Rect leftBorder = {bb.x, bb.y, bw.left, bb.height};
        ctx.fillRect(leftBorder, bc);
    }
    
    // Paint children
    for (auto& child : children_) {
        child->paint(ctx);
    }
}

RenderNode* RenderNode::hitTest(float x, float y) {
    Rect bb = boxModel_.borderBox();
    if (!bb.contains(x, y)) return nullptr;
    
    // Check children in reverse order (top is last)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if (RenderNode* hit = (*it)->hitTest(x, y)) {
            return hit;
        }
    }
    
    return this;
}

// =============================================================================
// RenderBlock
// =============================================================================

void RenderBlock::layout(float containerWidth) {
    // Apply CSS width
    float width = style_.autoWidth ? containerWidth : style_.width;
    width -= style_.margin.horizontal() + style_.padding.horizontal() + style_.borderWidth.horizontal();
    
    boxModel_.margin = style_.margin;
    boxModel_.padding = style_.padding;
    boxModel_.border = style_.borderWidth;
    boxModel_.contentBox.width = width;
    
    // Layout children
    float y = 0;
    for (auto& child : children_) {
        child->layout(width);
        child->boxModel_.contentBox.x = boxModel_.contentBox.x + style_.padding.left;
        child->boxModel_.contentBox.y = boxModel_.contentBox.y + style_.padding.top + y;
        y += child->boxModel_.marginBox().height;
    }
    
    boxModel_.contentBox.height = style_.autoHeight ? y : style_.height;
}

void RenderBlock::paint(PaintContext& ctx) {
    RenderNode::paint(ctx);
}

// =============================================================================
// RenderInline
// =============================================================================

void RenderInline::layout(float containerWidth) {
    // Inline elements don't force line breaks
    RenderNode::layout(containerWidth);
}

void RenderInline::paint(PaintContext& ctx) {
    RenderNode::paint(ctx);
}

// =============================================================================
// RenderText
// =============================================================================

RenderText::RenderText(const std::string& text) : text_(text) {}

void RenderText::layout(float containerWidth) {
    // Simple layout - each line is a rect
    lineBoxes_.clear();
    
    float x = 0, y = 0;
    float lineHeight = style_.fontSize * 1.2f;
    float charWidth = style_.fontSize * 0.6f; // Approximate
    
    size_t wordStart = 0;
    for (size_t i = 0; i <= text_.length(); ++i) {
        if (i == text_.length() || text_[i] == ' ' || text_[i] == '\n') {
            float wordWidth = (i - wordStart) * charWidth;
            
            if (x + wordWidth > containerWidth && x > 0) {
                // Wrap to next line
                x = 0;
                y += lineHeight;
            }
            
            if (i > wordStart) {
                lineBoxes_.push_back({x, y, wordWidth, lineHeight});
            }
            
            x += wordWidth + charWidth; // Space after word
            
            if (i < text_.length() && text_[i] == '\n') {
                x = 0;
                y += lineHeight;
            }
            
            wordStart = i + 1;
        }
    }
    
    boxModel_.contentBox.height = y + lineHeight;
    boxModel_.contentBox.width = containerWidth;
}

void RenderText::paint(PaintContext& ctx) {
    ctx.drawText(text_, boxModel_.contentBox.x, boxModel_.contentBox.y,
                 style_.color, style_.fontSize);
}

void RenderNode::dump(int indent) const {
    std::string prefix(indent * 2, ' ');
    std::cout << prefix << (isText() ? "Text" : "Box");
    if (!isText()) {
        std::cout << " Display: " << (int)style_.display;
    }
    std::cout << " Color: (" << (int)style_.color.r << "," << (int)style_.color.g << "," << (int)style_.color.b << ")";
    std::cout << " Font: " << style_.fontSize << "px " << style_.fontFamily;
    if (style_.fontBold) std::cout << " Bold";
    if (style_.fontItalic) std::cout << " Italic";
    
    if (isText()) {
        const auto* textNode = static_cast<const RenderText*>(this);
        std::string content = textNode->text();
        // Escape newlines for display
        std::string display;
        for (char c : content) {
            if (c == '\n') display += "\\n";
            else display += c;
        }
        std::cout << " '" << display.substr(0, 40) << "'";
    }
    std::cout << "\n";
    
    for (const auto& child : children_) {
        child->dump(indent + 1);
    }
}

// =============================================================================
// RenderImage
// =============================================================================

RenderImage::RenderImage(const std::string& src) : src_(src) {}

void RenderImage::layout(float containerWidth) {
    // Use natural size or CSS dimensions
    float width = style_.autoWidth ? 
        (naturalWidth_ > 0 ? naturalWidth_ : 300.0f) : style_.width;
    float height = style_.autoHeight ? 
        (naturalHeight_ > 0 ? naturalHeight_ : 150.0f) : style_.height;
    
    // Constrain to container
    if (width > containerWidth) {
        float scale = containerWidth / width;
        width = containerWidth;
        height *= scale;
    }
    
    boxModel_.margin = style_.margin;
    boxModel_.padding = style_.padding;
    boxModel_.border = style_.borderWidth;
    boxModel_.contentBox.width = width;
    boxModel_.contentBox.height = height;
}

void RenderImage::paint(PaintContext& ctx) {
    // Paint background
    RenderNode::paint(ctx);
    
    // Paint the image texture
    if (textureId_ > 0) {
        ctx.drawTexture(textureId_, boxModel_.contentBox);
    } else {
        // Placeholder for missing image
        Color placeholder = Color::fromRGBA(200, 200, 200, 255);
        ctx.fillRect(boxModel_.contentBox, placeholder);
        ctx.drawText("[Image]", 
                     boxModel_.contentBox.x + 10, 
                     boxModel_.contentBox.y + boxModel_.contentBox.height / 2,
                     Color::fromRGBA(128, 128, 128, 255), 12.0f);
    }
}

// =============================================================================
// RenderVideo
// =============================================================================

RenderVideo::RenderVideo() = default;
RenderVideo::~RenderVideo() = default;

void RenderVideo::play() {
    playing_ = true;
    paused_ = false;
    std::cout << "[RenderVideo] Play: " << src_ << std::endl;
}

void RenderVideo::pause() {
    playing_ = false;
    paused_ = true;
    std::cout << "[RenderVideo] Pause" << std::endl;
}

void RenderVideo::stop() {
    playing_ = false;
    paused_ = true;
    currentTime_ = 0.0;
    std::cout << "[RenderVideo] Stop" << std::endl;
}

void RenderVideo::seek(double seconds) {
    currentTime_ = std::max(0.0, std::min(seconds, duration_));
    std::cout << "[RenderVideo] Seek to " << seconds << "s" << std::endl;
}

double RenderVideo::currentTime() const {
    return currentTime_;
}

double RenderVideo::duration() const {
    return duration_;
}

void RenderVideo::update() {
    if (!playing_) return;
    
    // Simulate playback advancement (in real implementation, 
    // this comes from MediaPipeline)
    currentTime_ += 1.0 / 60.0;  // Assume 60 FPS
    
    if (currentTime_ >= duration_) {
        if (loop_) {
            currentTime_ = 0.0;
        } else {
            playing_ = false;
            paused_ = true;
        }
    }
}

void RenderVideo::layout(float containerWidth) {
    // Use video dimensions or CSS dimensions
    float width = style_.autoWidth ? 
        (videoWidth_ > 0 ? videoWidth_ : 640.0f) : style_.width;
    float height = style_.autoHeight ? 
        (videoHeight_ > 0 ? videoHeight_ : 360.0f) : style_.height;
    
    // Constrain to container while maintaining aspect ratio
    if (width > containerWidth) {
        float scale = containerWidth / width;
        width = containerWidth;
        height *= scale;
    }
    
    boxModel_.margin = style_.margin;
    boxModel_.padding = style_.padding;
    boxModel_.border = style_.borderWidth;
    boxModel_.contentBox.width = width;
    boxModel_.contentBox.height = height;
}

void RenderVideo::paint(PaintContext& ctx) {
    // Paint background (black for video)
    Color videoBg = Color::fromRGBA(0, 0, 0, 255);
    ctx.fillRect(boxModel_.contentBox, videoBg);
    
    // Paint the video frame texture
    if (textureId_ > 0 && playing_) {
        ctx.drawTexture(textureId_, boxModel_.contentBox);
    } else if (!poster_.empty()) {
        // Draw poster image when not playing
        // ctx.drawImage(poster_, boxModel_.contentBox);
        Color posterBg = Color::fromRGBA(40, 40, 40, 255);
        ctx.fillRect(boxModel_.contentBox, posterBg);
    }
    
    // Draw play button if paused
    if (paused_ && showControls_) {
        float cx = boxModel_.contentBox.x + boxModel_.contentBox.width / 2;
        float cy = boxModel_.contentBox.y + boxModel_.contentBox.height / 2;
        float btnSize = std::min(boxModel_.contentBox.width, boxModel_.contentBox.height) * 0.15f;
        
        // Play button circle
        Rect btnRect = {cx - btnSize, cy - btnSize, btnSize * 2, btnSize * 2};
        Color btnColor = Color::fromRGBA(255, 255, 255, 180);
        ctx.fillRect(btnRect, btnColor);
        
        // Play triangle (simplified as text)
        ctx.drawText("▶", cx - btnSize * 0.5f, cy - btnSize * 0.3f, 
                     Color::fromRGBA(0, 0, 0, 255), btnSize);
    }
    
    // Draw controls bar if enabled
    if (showControls_) {
        float barHeight = 40.0f;
        Rect controlsRect = {
            boxModel_.contentBox.x,
            boxModel_.contentBox.y + boxModel_.contentBox.height - barHeight,
            boxModel_.contentBox.width,
            barHeight
        };
        
        // Semi-transparent background
        Color controlsBg = Color::fromRGBA(0, 0, 0, 150);
        ctx.fillRect(controlsRect, controlsBg);
        
        // Progress bar
        float progress = duration_ > 0 ? currentTime_ / duration_ : 0.0f;
        Rect progressBg = {
            controlsRect.x + 60,
            controlsRect.y + 15,
            controlsRect.width - 120,
            10
        };
        ctx.fillRect(progressBg, Color::fromRGBA(80, 80, 80, 255));
        
        Rect progressFill = progressBg;
        progressFill.width *= progress;
        ctx.fillRect(progressFill, Color::fromRGBA(255, 0, 0, 255));
        
        // Time display
        char timeStr[32];
        int mins = static_cast<int>(currentTime_) / 60;
        int secs = static_cast<int>(currentTime_) % 60;
        snprintf(timeStr, sizeof(timeStr), "%d:%02d", mins, secs);
        ctx.drawText(timeStr, controlsRect.x + 10, controlsRect.y + 12,
                     Color::white(), 12.0f);
    }
}

// =============================================================================
// RenderAudio
// =============================================================================

RenderAudio::RenderAudio() = default;

void RenderAudio::play() {
    playing_ = true;
    std::cout << "[RenderAudio] Play: " << src_ << std::endl;
}

void RenderAudio::pause() {
    playing_ = false;
    std::cout << "[RenderAudio] Pause" << std::endl;
}

void RenderAudio::layout(float containerWidth) {
    // Audio element has fixed height when controls visible
    boxModel_.contentBox.width = std::min(containerWidth, 300.0f);
    boxModel_.contentBox.height = showControls_ ? 54.0f : 0.0f;
    
    boxModel_.margin = style_.margin;
    boxModel_.padding = style_.padding;
}

void RenderAudio::paint(PaintContext& ctx) {
    if (!showControls_) return;
    
    // Audio player UI
    Color bg = Color::fromRGBA(241, 243, 244, 255);
    ctx.fillRect(boxModel_.contentBox, bg);
    
    // Play button
    float btnSize = 36.0f;
    Rect playBtn = {
        boxModel_.contentBox.x + 8,
        boxModel_.contentBox.y + (boxModel_.contentBox.height - btnSize) / 2,
        btnSize, btnSize
    };
    
    Color btnColor = playing_ ? 
        Color::fromRGBA(66, 133, 244, 255) : 
        Color::fromRGBA(95, 99, 104, 255);
    ctx.fillRect(playBtn, btnColor);
    
    // Progress bar
    Rect progressBg = {
        playBtn.x + playBtn.width + 10,
        boxModel_.contentBox.y + boxModel_.contentBox.height / 2 - 2,
        boxModel_.contentBox.width - playBtn.width - 80,
        4
    };
    ctx.fillRect(progressBg, Color::fromRGBA(218, 220, 224, 255));
}

} // namespace Zepra::WebCore

