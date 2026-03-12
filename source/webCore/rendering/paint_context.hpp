#pragma once

/**
 * @file paint_context.hpp
 * @brief Paint context for rendering operations
 */

#include <stdint.h>
#include "rendering/render_tree.hpp"
#include <functional>
#include <vector>

namespace Zepra::WebCore {

/**
 * @brief Paint command types
 */
enum class PaintCommandType {
    FillRect,
    StrokeRect,
    FillRoundedRect,
    StrokeRoundedRect,
    FillCircle,
    StrokeCircle,
    DrawLine,
    DrawText,
    DrawImage,
    DrawTexture,
    DrawGradient,
    DrawShadow,
    DrawPath,
    PushClip,
    PopClip,
    PushOpacity,
    PopOpacity,
    SetTransform,
    ResetTransform,
    SetBlendMode
};

/**
 * @brief Gradient direction
 */
enum class GradientType {
    Linear,
    Radial
};

/**
 * @brief Color stop for gradients
 */
struct ColorStop {
    float position;  // 0.0 to 1.0
    Color color;
};

/**
 * @brief Blend modes
 */
enum class BlendMode {
    Normal,
    Multiply,
    Screen,
    Overlay,
    Darken,
    Lighten
};

/**
 * @brief Base paint command
 */
struct PaintCommand {
    PaintCommandType type = PaintCommandType::FillRect;
    Rect rect;
    Color color;
    Color color2;      // For gradients (end color)
    std::string text;
    std::string imagePath;
    uint32_t textureId = 0;
    float matrix[6] = {1, 0, 0, 1, 0, 0};
    float number = 0;       // Stroke width, font size, etc.
    float radius = 0;       // Border radius
    float shadowX = 0;      // Shadow offset X
    float shadowY = 0;      // Shadow offset Y  
    float shadowBlur = 0;   // Shadow blur radius
    Color shadowColor;
    GradientType gradientType = GradientType::Linear;
    float gradientAngle = 0;  // For linear gradients
    BlendMode blendMode = BlendMode::Normal;
    std::vector<ColorStop> colorStops;  // For complex gradients
};

/**
 * @brief Paint display list
 */
class DisplayList {
public:
    void addCommand(const PaintCommand& cmd);
    const std::vector<PaintCommand>& commands() const { return commands_; }
    void clear() { commands_.clear(); }
    size_t size() const { return commands_.size(); }
    
private:
    std::vector<PaintCommand> commands_;
};

/**
 * @brief Paint context for drawing operations
 */
class PaintContext {
public:
    explicit PaintContext(DisplayList& displayList);
    
    // Drawing primitives
    void fillRect(const Rect& rect, const Color& color);
    void fillRoundedRect(const Rect& rect, const Color& color, float radius);
    void strokeRect(const Rect& rect, const Color& color, float width = 1.0f);
    void drawText(const std::string& text, float x, float y, 
                  const Color& color, float fontSize);
    void drawImage(const std::string& path, const Rect& dest);
    void drawTexture(uint32_t textureId, const Rect& dest);
    
    // Clipping
    void pushClip(const Rect& rect);
    void popClip();
    
    // Transform
    void translate(float x, float y);
    void scale(float sx, float sy);
    void rotate(float angle);
    void resetTransform();
    
    // State
    void save();
    void restore();
    
    // Current bounds
    Rect clipBounds() const { return clipStack_.empty() ? 
        Rect{0, 0, 10000, 10000} : clipStack_.back(); }
    
private:
    DisplayList& displayList_;
    std::vector<Rect> clipStack_;
    float transform_[6] = {1, 0, 0, 1, 0, 0};
};

/**
 * @brief Backend-agnostic rendering interface
 */
class RenderBackend {
public:
    virtual ~RenderBackend() = default;
    
    // Execute display list
    virtual void executeDisplayList(const DisplayList& list) = 0;
    
    // Present to screen
    virtual void present() = 0;
    
    // Resize
    virtual void resize(int width, int height) = 0;
    
    // Create textures/surfaces
    virtual void* createTexture(int width, int height) = 0;
    virtual void destroyTexture(void* texture) = 0;
};

} // namespace Zepra::WebCore
