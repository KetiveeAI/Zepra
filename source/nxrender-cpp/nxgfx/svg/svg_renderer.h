// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "nxgfx/primitives.h"
#include "nxgfx/path/path.h"
#include "nxgfx/color.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <functional>

namespace NXRender {
namespace SVG {

// ==================================================================
// SVG Transform
// ==================================================================

struct SVGTransform {
    float matrix[6] = {1, 0, 0, 1, 0, 0}; // a b c d e f

    static SVGTransform identity();
    static SVGTransform translate(float tx, float ty);
    static SVGTransform scale(float sx, float sy);
    static SVGTransform rotate(float angleDeg, float cx = 0, float cy = 0);
    static SVGTransform skewX(float angleDeg);
    static SVGTransform skewY(float angleDeg);
    static SVGTransform fromMatrix(float a, float b, float c, float d, float e, float f);

    SVGTransform operator*(const SVGTransform& other) const;
    Point apply(const Point& p) const;

    static SVGTransform parse(const std::string& str);
};

// ==================================================================
// SVG Paint — fill/stroke specification
// ==================================================================

struct SVGPaint {
    enum class Type : uint8_t { None, Color, CurrentColor, URL, Inherit } type = Type::None;
    Color color;
    std::string url; // url(#gradient) or url(#pattern)

    static SVGPaint parse(const std::string& str);
    static SVGPaint none() { return {Type::None}; }
    static SVGPaint fromColor(const Color& c) { return {Type::Color, c}; }
};

// ==================================================================
// SVG Gradient
// ==================================================================

struct SVGGradientStop {
    float offset = 0;     // 0..1
    Color color;
    float opacity = 1.0f;
};

enum class SVGSpreadMethod : uint8_t {
    Pad, Reflect, Repeat
};

enum class SVGGradientUnits : uint8_t {
    UserSpaceOnUse, ObjectBoundingBox
};

struct SVGLinearGradient {
    float x1 = 0, y1 = 0, x2 = 1, y2 = 0;
    SVGGradientUnits units = SVGGradientUnits::ObjectBoundingBox;
    SVGSpreadMethod spread = SVGSpreadMethod::Pad;
    SVGTransform gradientTransform;
    std::vector<SVGGradientStop> stops;
    std::string href; // xlink:href for inheritance
};

struct SVGRadialGradient {
    float cx = 0.5f, cy = 0.5f, r = 0.5f;
    float fx = 0.5f, fy = 0.5f, fr = 0;
    SVGGradientUnits units = SVGGradientUnits::ObjectBoundingBox;
    SVGSpreadMethod spread = SVGSpreadMethod::Pad;
    SVGTransform gradientTransform;
    std::vector<SVGGradientStop> stops;
    std::string href;
};

// ==================================================================
// SVG Filter primitives
// ==================================================================

struct SVGFilterPrimitive {
    enum class Type : uint8_t {
        GaussianBlur, ColorMatrix, Offset, Composite, Merge,
        Flood, Turbulence, DisplacementMap, Morphology,
        ConvolveMatrix, DiffuseLighting, SpecularLighting,
        Tile, Image, ComponentTransfer, Blend, DropShadow
    } type;

    std::string in, in2; // Input references
    std::string result;
    Rect subregion;

    // GaussianBlur
    float stdDevX = 0, stdDevY = 0;

    // ColorMatrix
    enum class MatrixType : uint8_t { Matrix, Saturate, HueRotate, LuminanceToAlpha } matrixType;
    std::vector<float> matrixValues; // 20 values for full matrix, 1 for saturate/hueRotate

    // Offset
    float dx = 0, dy = 0;

    // Composite
    enum class CompositeOp : uint8_t { Over, In, Out, Atop, Xor, Arithmetic } compositeOp;
    float k1 = 0, k2 = 0, k3 = 0, k4 = 0;

    // Flood
    Color floodColor;
    float floodOpacity = 1;

    // DropShadow
    Color shadowColor;
    float shadowOffsetX = 0, shadowOffsetY = 0;
    float shadowBlur = 0;

    // Turbulence
    float baseFrequencyX = 0, baseFrequencyY = 0;
    int numOctaves = 1;
    float seed = 0;
    bool stitchTiles = false;
    enum class TurbulenceType { FractalNoise, Turbulence } turbulenceType;

    // Morphology
    enum class MorphologyOp { Erode, Dilate } morphologyOp;
    float radiusX = 0, radiusY = 0;

    // Blend
    enum class BlendMode : uint8_t {
        Normal, Multiply, Screen, Overlay, Darken, Lighten,
        ColorDodge, ColorBurn, HardLight, SoftLight, Difference, Exclusion
    } blendMode;
};

struct SVGFilter {
    std::string id;
    SVGGradientUnits units = SVGGradientUnits::UserSpaceOnUse;
    Rect filterRegion;
    std::vector<SVGFilterPrimitive> primitives;
};

// ==================================================================
// SVG Clip path
// ==================================================================

struct SVGClipPath {
    std::string id;
    SVGGradientUnits units = SVGGradientUnits::UserSpaceOnUse;
    PathGen::Path path;
    std::vector<std::unique_ptr<struct SVGNode>> children;
};

// ==================================================================
// SVG Mask
// ==================================================================

struct SVGMask {
    std::string id;
    Rect maskRegion;
    SVGGradientUnits units = SVGGradientUnits::ObjectBoundingBox;
    SVGGradientUnits contentUnits = SVGGradientUnits::UserSpaceOnUse;
    std::vector<std::unique_ptr<struct SVGNode>> children;
};

// ==================================================================
// SVG Pattern
// ==================================================================

struct SVGPattern {
    std::string id;
    float x = 0, y = 0, width = 0, height = 0;
    SVGGradientUnits units = SVGGradientUnits::ObjectBoundingBox;
    SVGGradientUnits contentUnits = SVGGradientUnits::UserSpaceOnUse;
    SVGTransform patternTransform;
    std::string href;
    Rect viewBox;
    std::vector<std::unique_ptr<struct SVGNode>> children;
};

// ==================================================================
// SVG Style properties
// ==================================================================

struct SVGStyle {
    SVGPaint fill = SVGPaint::fromColor(Color::black());
    SVGPaint stroke = SVGPaint::none();
    float fillOpacity = 1.0f;
    float strokeOpacity = 1.0f;
    float opacity = 1.0f;

    float strokeWidth = 1.0f;
    enum class LineCap : uint8_t { Butt, Round, Square } lineCap = LineCap::Butt;
    enum class LineJoin : uint8_t { Miter, Round, Bevel } lineJoin = LineJoin::Miter;
    float miterLimit = 4.0f;
    std::vector<float> dashArray;
    float dashOffset = 0;

    enum class FillRule : uint8_t { NonZero, EvenOdd } fillRule = FillRule::NonZero;

    // Text
    float fontSize = 16;
    std::string fontFamily = "sans-serif";
    std::string fontWeight = "normal";
    std::string fontStyle = "normal";
    enum class TextAnchor : uint8_t { Start, Middle, End } textAnchor = TextAnchor::Start;
    enum class DominantBaseline : uint8_t { Auto, Middle, Hanging, Central } baseline = DominantBaseline::Auto;

    // Visibility
    bool visible = true;
    enum class Display : uint8_t { Inline, None, Block } display = Display::Inline;

    // References
    std::string clipPathId;
    std::string maskId;
    std::string filterId;
    std::string markerId, markerStartId, markerEndId;
};

// ==================================================================
// SVG Node (element tree)
// ==================================================================

struct SVGNode {
    enum class Type : uint8_t {
        SVG, Group, Path, Rect, Circle, Ellipse, Line, Polyline, Polygon,
        Text, TSpan, TextPath, Image, Use,
        Defs, Symbol, ClipPathNode, MaskNode, PatternNode,
        LinearGradientNode, RadialGradientNode, FilterNode,
        Marker, ForeignObject, Switch, A,
    } type = Type::Group;

    std::string id;
    std::string className;
    SVGStyle style;
    SVGTransform transform;

    // Geometry (depends on type)
    float x = 0, y = 0, width = 0, height = 0;
    float rx = 0, ry = 0;
    float cx = 0, cy = 0, r = 0;
    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    PathGen::Path path;
    std::vector<Point> points; // polyline/polygon
    std::string textContent;

    // Image
    std::string href; // xlink:href for <use>, <image>

    // ViewBox
    Rect viewBox;
    enum class PreserveAspectRatio : uint8_t {
        None, XMinYMin, XMidYMin, XMaxYMin,
        XMinYMid, XMidYMid, XMaxYMid,
        XMinYMax, XMidYMax, XMaxYMax
    } preserveAspectRatio = PreserveAspectRatio::XMidYMid;
    enum class MeetOrSlice : uint8_t { Meet, Slice } meetOrSlice = MeetOrSlice::Meet;

    std::vector<std::unique_ptr<SVGNode>> children;
    SVGNode* parent = nullptr;

    // Computed bounding box
    Rect computeBBox() const;
};

// ==================================================================
// SVG Document — parsed SVG data
// ==================================================================

class SVGDocument {
public:
    SVGDocument();
    ~SVGDocument();

    bool parse(const std::string& svgSource);
    bool parse(const uint8_t* data, size_t size);

    SVGNode* root() const { return root_.get(); }
    float width() const { return width_; }
    float height() const { return height_; }
    const Rect& viewBox() const { return viewBox_; }

    // Lookup by ID
    SVGNode* getElementById(const std::string& id) const;

    // Defs lookups
    SVGLinearGradient* linearGradient(const std::string& id) const;
    SVGRadialGradient* radialGradient(const std::string& id) const;
    SVGFilter* filter(const std::string& id) const;
    SVGClipPath* clipPath(const std::string& id) const;
    SVGMask* mask(const std::string& id) const;
    SVGPattern* pattern(const std::string& id) const;

    // Style resolution (CSS <style> blocks)
    void resolveStyles(SVGNode* node);

private:
    std::unique_ptr<SVGNode> root_;
    float width_ = 0, height_ = 0;
    Rect viewBox_;

    std::unordered_map<std::string, SVGNode*> idMap_;
    std::unordered_map<std::string, SVGLinearGradient> linearGradients_;
    std::unordered_map<std::string, SVGRadialGradient> radialGradients_;
    std::unordered_map<std::string, SVGFilter> filters_;
    std::unordered_map<std::string, SVGClipPath> clipPaths_;
    std::unordered_map<std::string, SVGMask> masks_;
    std::unordered_map<std::string, SVGPattern> patterns_;

    void buildIdMap(SVGNode* node);
    void parseElement(SVGNode* node, const std::string& tag,
                       const std::unordered_map<std::string, std::string>& attrs);
    void parseStyle(SVGNode* node, const std::string& styleStr);
    void parseDefs(SVGNode* node);
};

// ==================================================================
// SVG Renderer — rasterizes SVG to pixels
// ==================================================================

class SVGRenderer {
public:
    struct RenderConfig {
        int width = 0, height = 0;      // Output size (0 = use document size)
        float dpi = 96;
        Color backgroundColor = {0, 0, 0, 0}; // Transparent
        bool antialias = true;
    };

    // Render to RGBA pixel buffer
    std::vector<uint8_t> render(const SVGDocument& doc, const RenderConfig& config);

    // Render to existing buffer
    void renderInto(const SVGDocument& doc, uint8_t* pixels, int width, int height,
                     int stride);

    // Render single node subtree
    void renderNode(const SVGNode& node, uint8_t* pixels, int width, int height,
                     int stride, const SVGTransform& parentTransform = SVGTransform::identity());

private:
    void renderShape(const SVGNode& node, uint8_t* pixels, int w, int h, int stride,
                      const SVGTransform& transform);
    void renderText(const SVGNode& node, uint8_t* pixels, int w, int h, int stride,
                     const SVGTransform& transform);
    void renderImage(const SVGNode& node, uint8_t* pixels, int w, int h, int stride,
                      const SVGTransform& transform);
    void applyFilter(const SVGFilter& filter, uint8_t* pixels, int w, int h, int stride,
                      const Rect& region);
    void rasterizePath(const PathGen::Path& path, uint8_t* pixels, int w, int h, int stride,
                        const SVGStyle& style, const SVGTransform& transform);
};

} // namespace SVG
} // namespace NXRender
