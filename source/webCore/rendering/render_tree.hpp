#pragma once

/**
 * @file render_tree.hpp
 * @brief Render tree for visual representation of DOM
 */

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace Zepra::WebCore {

class DOMNode;

/**
 * @brief Color representation (RGBA)
 */
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;
    
    static Color transparent() { return {0, 0, 0, 0}; }
    static Color black() { return {0, 0, 0, 255}; }
    static Color white() { return {255, 255, 255, 255}; }
    static Color red() { return {255, 0, 0, 255}; }
    static Color green() { return {0, 255, 0, 255}; }
    static Color blue() { return {0, 0, 255, 255}; }
    
    static Color fromHex(const std::string& hex);
    static Color fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
};

/**
 * @brief Rectangle for layout
 */
struct Rect {
    float x = 0, y = 0;
    float width = 0, height = 0;
    
    bool contains(float px, float py) const;
    bool intersects(const Rect& other) const;
    Rect intersected(const Rect& other) const;
};

/**
 * @brief Edge insets (margin, padding, border)
 */
struct EdgeInsets {
    float top = 0, right = 0, bottom = 0, left = 0;
    
    float horizontal() const { return left + right; }
    float vertical() const { return top + bottom; }
};

/**
 * @brief Display type for layout
 */
enum class Display {
    Block,
    Inline,
    InlineBlock,
    Flex,
    Grid,
    None
};

/**
 * @brief Box model for layout
 */
struct BoxModel {
    Rect contentBox;
    EdgeInsets padding;
    EdgeInsets border;
    EdgeInsets margin;
    
    Rect paddingBox() const;
    Rect borderBox() const;
    Rect marginBox() const;
};

/**
 * @brief Computed style for a render node
 */
struct ComputedStyle {
    // Display & Layout
    Display display = Display::Block;
    float width = 0, height = 0;
    bool autoWidth = true, autoHeight = true;
    
    // Spacing
    EdgeInsets margin;
    EdgeInsets padding;
    EdgeInsets borderWidth;
    
    // Colors
    Color color = Color::black();
    Color backgroundColor = Color::transparent();
    Color borderColor = Color::black();
    
    // Text
    std::string fontFamily = "sans-serif";
    float fontSize = 16.0f;
    bool fontBold = false;
    bool fontItalic = false;
    
    // Position
    enum class Position { Static, Relative, Absolute, Fixed } position = Position::Static;
    float top = 0, right = 0, bottom = 0, left = 0;
    
    // Z-order
    int zIndex = 0;
    
    // Overflow
    enum class Overflow { Visible, Hidden, Scroll, Auto } overflow = Overflow::Visible;
};

/**
 * @brief Base class for render tree nodes
 */
class RenderNode {
public:
    RenderNode();
    virtual ~RenderNode();
    
    // Tree structure
    RenderNode* parent() const { return parent_; }
    const std::vector<std::unique_ptr<RenderNode>>& children() const { return children_; }
    void appendChild(std::unique_ptr<RenderNode> child);
    void removeChild(RenderNode* child);
    
    // Layout
    virtual void layout(float containerWidth);
    const BoxModel& boxModel() const { return boxModel_; }
    
    // Style
    ComputedStyle& style() { return style_; }
    const ComputedStyle& style() const { return style_; }
    
    // Painting
    virtual void paint(class PaintContext& ctx);
    
    // Hit testing
    virtual RenderNode* hitTest(float x, float y);
    
    // Type info
    virtual bool isText() const { return false; }
    virtual bool isBox() const { return true; }

    // Debug
    virtual void dump(int indent = 0) const;
    
    // DOM Link
    void setDomNode(DOMNode* node) { domNode_ = node; }
    DOMNode* domNode() const { return domNode_; }
    
    // Layout invalidation (dirty flags for incremental reflow)
    bool needsLayout() const { return needsLayout_; }
    bool needsPaint() const { return needsPaint_; }
    void setNeedsLayout() { needsLayout_ = true; markAncestorsNeedLayout(); }
    void setNeedsPaint() { needsPaint_ = true; }
    void clearNeedsLayout() { needsLayout_ = false; }
    void clearNeedsPaint() { needsPaint_ = false; }
    void markAncestorsNeedLayout() { if (parent_) parent_->setNeedsLayout(); }
    
protected:
    RenderNode* parent_ = nullptr;
    DOMNode* domNode_ = nullptr;
    std::vector<std::unique_ptr<RenderNode>> children_;
    BoxModel boxModel_;
    ComputedStyle style_;
    
    // Dirty flags for incremental layout
    bool needsLayout_ = true;
    bool needsPaint_ = true;
    
    // Allow subclasses to access boxModel for layout
    BoxModel& mutableBoxModel() { return boxModel_; }
    friend class RenderBlock;
    friend class RenderInline;
    friend class RenderText;
};

/**
 * @brief Render node for block-level elements
 */
class RenderBlock : public RenderNode {
public:
    void layout(float containerWidth) override;
    void paint(class PaintContext& ctx) override;
};

/**
 * @brief Render node for inline content
 */
class RenderInline : public RenderNode {
public:
    void layout(float containerWidth) override;
    void paint(class PaintContext& ctx) override;
};

/**
 * @brief Render node for text content
 */
class RenderText : public RenderNode {
public:
    explicit RenderText(const std::string& text);
    
    const std::string& text() const { return text_; }
    void setText(const std::string& text) { text_ = text; }
    
    bool isText() const override { return true; }
    bool isBox() const override { return false; }
    
    void layout(float containerWidth) override;
    void paint(class PaintContext& ctx) override;
    
private:
    std::string text_;
    std::vector<Rect> lineBoxes_;
};

/**
 * @brief Render node for images
 */
class RenderImage : public RenderNode {
public:
    explicit RenderImage(const std::string& src);
    
    const std::string& src() const { return src_; }
    void setSrc(const std::string& src) { src_ = src; }
    
    void setTextureId(uint32_t id) { textureId_ = id; }
    uint32_t textureId() const { return textureId_; }
    
    void setNaturalSize(int width, int height) { 
        naturalWidth_ = width; 
        naturalHeight_ = height; 
    }
    
    void layout(float containerWidth) override;
    void paint(class PaintContext& ctx) override;
    
private:
    std::string src_;
    uint32_t textureId_ = 0;
    int naturalWidth_ = 0;
    int naturalHeight_ = 0;
};

/**
 * @brief Render node for video elements
 * 
 * Handles HTML5 <video> rendering with GPU acceleration.
 * Connects to MediaPipeline for frame updates.
 */
class RenderVideo : public RenderNode {
public:
    RenderVideo();
    ~RenderVideo() override;
    
    // Source
    void setSrc(const std::string& src) { src_ = src; }
    const std::string& src() const { return src_; }
    
    // Playback control
    void play();
    void pause();
    void stop();
    void seek(double seconds);
    
    // State
    bool isPlaying() const { return playing_; }
    bool isPaused() const { return paused_; }
    double currentTime() const;
    double duration() const;
    
    // Video properties
    int videoWidth() const { return videoWidth_; }
    int videoHeight() const { return videoHeight_; }
    
    // Display
    void setPoster(const std::string& url) { poster_ = url; }
    void setControls(bool show) { showControls_ = show; }
    void setAutoplay(bool autoplay) { autoplay_ = autoplay; }
    void setLoop(bool loop) { loop_ = loop; }
    void setMuted(bool muted) { muted_ = muted; }
    
    // Volume
    void setVolume(float volume) { volume_ = volume; }
    float volume() const { return volume_; }
    
    // Frame texture (for GPU rendering)
    uint32_t currentFrameTexture() const { return textureId_; }
    
    // Update playback (call per frame)
    void update();
    
    // RenderNode overrides
    void layout(float containerWidth) override;
    void paint(class PaintContext& ctx) override;
    
    // Type
    bool isVideo() const { return true; }
    
private:
    std::string src_;
    std::string poster_;
    
    bool playing_ = false;
    bool paused_ = true;
    bool autoplay_ = false;
    bool loop_ = false;
    bool muted_ = false;
    bool showControls_ = true;
    
    float volume_ = 1.0f;
    
    int videoWidth_ = 0;
    int videoHeight_ = 0;
    
    uint32_t textureId_ = 0;
    
    // Playback state
    double currentTime_ = 0.0;
    double duration_ = 0.0;
};

/**
 * @brief Render node for audio elements
 */
class RenderAudio : public RenderNode {
public:
    RenderAudio();
    
    void setSrc(const std::string& src) { src_ = src; }
    const std::string& src() const { return src_; }
    
    void play();
    void pause();
    
    void layout(float containerWidth) override;
    void paint(class PaintContext& ctx) override;
    
private:
    std::string src_;
    bool playing_ = false;
    bool showControls_ = true;
};

} // namespace Zepra::WebCore
