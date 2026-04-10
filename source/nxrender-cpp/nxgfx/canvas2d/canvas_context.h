// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "nxgfx/color.h"
#include "nxgfx/math/transform2d.h"
#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <stack>

namespace NXRender {

// ==================================================================
// Canvas 2D Rendering Context
// HTML Canvas 2D Context specification
// ==================================================================

enum class LineCap : uint8_t { Butt, Round, Square };
enum class LineJoin : uint8_t { Miter, Round, Bevel };
enum class TextBaseline : uint8_t { Top, Hanging, Middle, Alphabetic, Ideographic, Bottom };
enum class TextAlign2D : uint8_t { Start, End, Left, Right, Center };
enum class FillRule : uint8_t { NonZero, EvenOdd };
enum class CompositeOp : uint8_t {
    SourceOver, SourceIn, SourceOut, SourceAtop,
    DestinationOver, DestinationIn, DestinationOut, DestinationAtop,
    Lighter, Copy, Xor, Multiply, Screen, Overlay,
    Darken, Lighten, ColorDodge, ColorBurn,
    HardLight, SoftLight, Difference, Exclusion,
    Hue, Saturation, ColorOp, Luminosity,
};

enum class RepeatMode : uint8_t { Repeat, RepeatX, RepeatY, NoRepeat };

struct CanvasGradient {
    enum class Type { Linear, Radial, Conic } type;
    float x0 = 0, y0 = 0, r0 = 0;
    float x1 = 0, y1 = 0, r1 = 0;
    float angle = 0; // Conic

    struct Stop {
        float offset;
        Color color;
    };
    std::vector<Stop> stops;

    void addColorStop(float offset, const Color& color);
};

struct CanvasPattern {
    uint32_t textureId = 0;
    int width = 0, height = 0;
    RepeatMode repeat = RepeatMode::Repeat;
    Math::Transform2D transform;
};

struct CanvasFillStyle {
    enum class Type { Color, Gradient, Pattern } type = Type::Color;
    Color color;
    CanvasGradient gradient;
    CanvasPattern pattern;
};

struct ImageData {
    int width = 0, height = 0;
    std::vector<uint8_t> data; // RGBA, row-major

    ImageData() = default;
    ImageData(int w, int h);

    uint8_t getR(int x, int y) const;
    uint8_t getG(int x, int y) const;
    uint8_t getB(int x, int y) const;
    uint8_t getA(int x, int y) const;

    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
};

struct TextMetrics {
    float width = 0;
    float actualBoundingBoxLeft = 0;
    float actualBoundingBoxRight = 0;
    float actualBoundingBoxAscent = 0;
    float actualBoundingBoxDescent = 0;
    float fontBoundingBoxAscent = 0;
    float fontBoundingBoxDescent = 0;
    float emHeightAscent = 0;
    float emHeightDescent = 0;
    float hangingBaseline = 0;
    float alphabeticBaseline = 0;
    float ideographicBaseline = 0;
};

// ==================================================================
// Path2D — reusable subpaths
// ==================================================================

class Path2D {
public:
    Path2D();
    Path2D(const std::string& svgPathData);

    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void quadraticCurveTo(float cpx, float cpy, float x, float y);
    void bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y);
    void arcTo(float x1, float y1, float x2, float y2, float radius);
    void arc(float x, float y, float radius, float startAngle, float endAngle, bool ccw = false);
    void ellipse(float x, float y, float rx, float ry, float rotation,
                 float startAngle, float endAngle, bool ccw = false);
    void rect(float x, float y, float w, float h);
    void roundRect(float x, float y, float w, float h,
                   float tl, float tr, float br, float bl);
    void closePath();

    void addPath(const Path2D& other);

    struct Command {
        enum Type {
            MoveTo, LineTo, QuadTo, CubicTo, ArcTo, Arc, Ellipse, Rect, Close
        } type;
        float args[8];
        int argCount = 0;
    };

    const std::vector<Command>& commands() const { return commands_; }
    bool empty() const { return commands_.empty(); }

private:
    std::vector<Command> commands_;
    void parseSVGPathData(const std::string& d);
};

// ==================================================================
// CanvasRenderingContext2D
// ==================================================================

class CanvasRenderingContext2D {
public:
    CanvasRenderingContext2D(int width, int height);
    ~CanvasRenderingContext2D();

    int width() const { return width_; }
    int height() const { return height_; }
    void resize(int w, int h);

    // State
    void save();
    void restore();
    void reset();

    // Transform
    void scale(float x, float y);
    void rotate(float angle);
    void translate(float x, float y);
    void transform(float a, float b, float c, float d, float e, float f);
    void setTransform(float a, float b, float c, float d, float e, float f);
    void resetTransform();
    Math::Transform2D getTransform() const;

    // Compositing
    void setGlobalAlpha(float alpha);
    float globalAlpha() const;
    void setGlobalCompositeOperation(CompositeOp op);
    CompositeOp globalCompositeOperation() const;

    // Fill/Stroke style
    void setFillStyle(const Color& color);
    void setFillStyle(const CanvasGradient& gradient);
    void setFillStyle(const CanvasPattern& pattern);
    void setStrokeStyle(const Color& color);
    void setStrokeStyle(const CanvasGradient& gradient);

    // Line style
    void setLineWidth(float width);
    float lineWidth() const;
    void setLineCap(LineCap cap);
    void setLineJoin(LineJoin join);
    void setMiterLimit(float limit);
    void setLineDash(const std::vector<float>& segments);
    std::vector<float> getLineDash() const;
    void setLineDashOffset(float offset);

    // Shadow
    void setShadowColor(const Color& color);
    void setShadowBlur(float blur);
    void setShadowOffsetX(float x);
    void setShadowOffsetY(float y);

    // Text
    void setFont(const std::string& font);
    void setTextAlign(TextAlign2D align);
    void setTextBaseline(TextBaseline baseline);
    void setDirection(const std::string& dir);

    // Path operations
    void beginPath();
    void closePath();
    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void quadraticCurveTo(float cpx, float cpy, float x, float y);
    void bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y);
    void arcTo(float x1, float y1, float x2, float y2, float radius);
    void arc(float x, float y, float radius, float startAngle, float endAngle, bool ccw = false);
    void ellipse(float x, float y, float rx, float ry, float rotation,
                 float startAngle, float endAngle, bool ccw = false);
    void rect(float x, float y, float w, float h);
    void roundRect(float x, float y, float w, float h,
                   float tl, float tr = -1, float br = -1, float bl = -1);

    // Drawing paths
    void fill(FillRule rule = FillRule::NonZero);
    void fill(const Path2D& path, FillRule rule = FillRule::NonZero);
    void stroke();
    void stroke(const Path2D& path);
    void clip(FillRule rule = FillRule::NonZero);
    void clip(const Path2D& path, FillRule rule = FillRule::NonZero);
    bool isPointInPath(float x, float y, FillRule rule = FillRule::NonZero);
    bool isPointInStroke(float x, float y);

    // Rectangles
    void clearRect(float x, float y, float w, float h);
    void fillRect(float x, float y, float w, float h);
    void strokeRect(float x, float y, float w, float h);

    // Text
    void fillText(const std::string& text, float x, float y, float maxWidth = -1);
    void strokeText(const std::string& text, float x, float y, float maxWidth = -1);
    TextMetrics measureText(const std::string& text);

    // Image drawing
    void drawImage(uint32_t textureId, float dx, float dy);
    void drawImage(uint32_t textureId, float dx, float dy, float dw, float dh);
    void drawImage(uint32_t textureId, float sx, float sy, float sw, float sh,
                   float dx, float dy, float dw, float dh);

    // Pixel manipulation
    ImageData createImageData(int width, int height);
    ImageData getImageData(int sx, int sy, int sw, int sh);
    void putImageData(const ImageData& data, int dx, int dy);
    void putImageData(const ImageData& data, int dx, int dy,
                      int dirtyX, int dirtyY, int dirtyW, int dirtyH);

    // Gradient/pattern creation
    CanvasGradient createLinearGradient(float x0, float y0, float x1, float y1);
    CanvasGradient createRadialGradient(float x0, float y0, float r0,
                                         float x1, float y1, float r1);
    CanvasGradient createConicGradient(float startAngle, float x, float y);
    CanvasPattern createPattern(uint32_t textureId, int w, int h, RepeatMode repeat);

    // Filter
    void setFilter(const std::string& filter);

    // Framebuffer access
    uint32_t framebufferId() const { return fbo_; }
    uint32_t textureId() const { return texture_; }

private:
    int width_, height_;
    uint32_t fbo_ = 0;
    uint32_t texture_ = 0;

    // State stack
    struct State {
        Math::Transform2D transform;
        CanvasFillStyle fillStyle;
        CanvasFillStyle strokeStyle;
        float lineWidth = 1.0f;
        LineCap lineCap = LineCap::Butt;
        LineJoin lineJoin = LineJoin::Miter;
        float miterLimit = 10.0f;
        std::vector<float> lineDash;
        float lineDashOffset = 0;
        float globalAlpha = 1.0f;
        CompositeOp compositeOp = CompositeOp::SourceOver;
        Color shadowColor;
        float shadowBlur = 0;
        float shadowOffsetX = 0, shadowOffsetY = 0;
        std::string font = "10px sans-serif";
        TextAlign2D textAlign = TextAlign2D::Start;
        TextBaseline textBaseline = TextBaseline::Alphabetic;
        std::string direction = "ltr";
        std::string filter;
    };

    State state_;
    std::stack<State> stateStack_;
    Path2D currentPath_;

    void setupFBO();
    void teardownFBO();
    void applyCompositing();
    void drawPathFill(const Path2D& path, FillRule rule);
    void drawPathStroke(const Path2D& path);
};

} // namespace NXRender
