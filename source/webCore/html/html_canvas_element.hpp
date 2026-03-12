/**
 * @file html_canvas_element.hpp
 * @brief HTMLCanvasElement and CanvasRenderingContext2D interfaces
 *
 * Implements canvas drawing functionality per HTML Living Standard.
 *
 * @see https://developer.mozilla.org/en-US/docs/Web/API/HTMLCanvasElement
 * @see https://developer.mozilla.org/en-US/docs/Web/API/CanvasRenderingContext2D
 */

#pragma once

#include "html/html_element.hpp"
#include <vector>
#include <variant>
#include <functional>

namespace Zepra::WebCore {

// Forward declarations
class CanvasRenderingContext2D;
class ImageData;
class CanvasGradient;
class CanvasPattern;

/**
 * @brief Line cap style
 */
enum class LineCap {
    Butt,
    Round,
    Square
};

/**
 * @brief Line join style
 */
enum class LineJoin {
    Miter,
    Round,
    Bevel
};

/**
 * @brief Text alignment
 */
enum class TextAlign {
    Start,
    End,
    Left,
    Right,
    Center
};

/**
 * @brief Text baseline
 */
enum class TextBaseline {
    Top,
    Hanging,
    Middle,
    Alphabetic,
    Ideographic,
    Bottom
};

/**
 * @brief Compositing operation
 */
enum class CompositeOperation {
    SourceOver,
    SourceIn,
    SourceOut,
    SourceAtop,
    DestinationOver,
    DestinationIn,
    DestinationOut,
    DestinationAtop,
    Lighter,
    Copy,
    Xor,
    Multiply,
    Screen,
    Overlay,
    Darken,
    Lighten,
    ColorDodge,
    ColorBurn,
    HardLight,
    SoftLight,
    Difference,
    Exclusion,
    Hue,
    Saturation,
    Color,
    Luminosity
};

/**
 * @brief Fill rule for paths
 */
enum class FillRule {
    NonZero,
    EvenOdd
};

/**
 * @brief Image smoothing quality
 */
enum class ImageSmoothingQuality {
    Low,
    Medium,
    High
};

/**
 * @brief Canvas gradient
 */
class CanvasGradient {
public:
    CanvasGradient(double x0, double y0, double x1, double y1);
    CanvasGradient(double x0, double y0, double r0, double x1, double y1, double r1);
    
    /// Add color stop
    void addColorStop(double offset, const std::string& color);
    
    /// Get color stops
    struct ColorStop {
        double offset;
        std::string color;
    };
    const std::vector<ColorStop>& colorStops() const { return stops_; }
    
    bool isRadial() const { return isRadial_; }
    double x0() const { return x0_; }
    double y0() const { return y0_; }
    double x1() const { return x1_; }
    double y1() const { return y1_; }
    double r0() const { return r0_; }
    double r1() const { return r1_; }

private:
    bool isRadial_;
    double x0_, y0_, x1_, y1_;
    double r0_ = 0, r1_ = 0;
    std::vector<ColorStop> stops_;
};

/**
 * @brief Canvas pattern
 */
class CanvasPattern {
public:
    enum class Repetition {
        Repeat,
        RepeatX,
        RepeatY,
        NoRepeat
    };
    
    CanvasPattern(const std::vector<uint8_t>& imageData, int width, int height, Repetition rep);
    
    void setTransform(double a, double b, double c, double d, double e, double f);
    
    const std::vector<uint8_t>& imageData() const { return imageData_; }
    int width() const { return width_; }
    int height() const { return height_; }
    Repetition repetition() const { return repetition_; }

private:
    std::vector<uint8_t> imageData_;
    int width_, height_;
    Repetition repetition_;
    double transform_[6] = {1, 0, 0, 1, 0, 0};
};

/**
 * @brief Image data for pixel manipulation
 */
class ImageData {
public:
    ImageData(unsigned int width, unsigned int height);
    ImageData(const std::vector<uint8_t>& data, unsigned int width, unsigned int height);
    
    unsigned int width() const { return width_; }
    unsigned int height() const { return height_; }
    
    /// Access pixel data (RGBA, 4 bytes per pixel)
    std::vector<uint8_t>& data() { return data_; }
    const std::vector<uint8_t>& data() const { return data_; }

private:
    unsigned int width_;
    unsigned int height_;
    std::vector<uint8_t> data_;
};

/**
 * @brief Path2D for reusable paths
 */
class Path2D {
public:
    Path2D();
    Path2D(const std::string& svgPath);
    
    void moveTo(double x, double y);
    void lineTo(double x, double y);
    void bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y);
    void quadraticCurveTo(double cpx, double cpy, double x, double y);
    void arc(double x, double y, double radius, double startAngle, double endAngle, bool counterclockwise = false);
    void arcTo(double x1, double y1, double x2, double y2, double radius);
    void ellipse(double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle, bool counterclockwise = false);
    void rect(double x, double y, double width, double height);
    void roundRect(double x, double y, double width, double height, double radii);
    void closePath();
    
    void addPath(const Path2D& path);
    
    struct Command {
        enum Type { Move, Line, BezierCurve, QuadraticCurve, Arc, ArcTo, Ellipse, Rect, Close };
        Type type;
        std::vector<double> params;
    };
    const std::vector<Command>& commands() const { return commands_; }

private:
    std::vector<Command> commands_;
};

/**
 * @brief Text metrics
 */
struct TextMetrics {
    double width;
    double actualBoundingBoxLeft;
    double actualBoundingBoxRight;
    double fontBoundingBoxAscent;
    double fontBoundingBoxDescent;
    double actualBoundingBoxAscent;
    double actualBoundingBoxDescent;
    double emHeightAscent;
    double emHeightDescent;
    double hangingBaseline;
    double alphabeticBaseline;
    double ideographicBaseline;
};

/// Fill/stroke style variant
using CanvasStyle = std::variant<std::string, CanvasGradient*, CanvasPattern*>;

/**
 * @brief CanvasRenderingContext2D - 2D drawing context
 */
class CanvasRenderingContext2D {
public:
    explicit CanvasRenderingContext2D(class HTMLCanvasElement* canvas);
    ~CanvasRenderingContext2D();

    /// Parent canvas
    HTMLCanvasElement* canvas() const { return canvas_; }

    // =========================================================================
    // State
    // =========================================================================

    void save();
    void restore();
    void reset();

    // =========================================================================
    // Transform
    // =========================================================================

    void scale(double x, double y);
    void rotate(double angle);
    void translate(double x, double y);
    void transform(double a, double b, double c, double d, double e, double f);
    void setTransform(double a, double b, double c, double d, double e, double f);
    void resetTransform();

    // Current transform matrix
    struct DOMMatrix {
        double a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;
    };
    DOMMatrix getTransform() const;

    // =========================================================================
    // Fill/Stroke Style
    // =========================================================================

    std::string fillStyle() const;
    void setFillStyle(const std::string& color);
    void setFillStyle(CanvasGradient* gradient);
    void setFillStyle(CanvasPattern* pattern);

    std::string strokeStyle() const;
    void setStrokeStyle(const std::string& color);
    void setStrokeStyle(CanvasGradient* gradient);
    void setStrokeStyle(CanvasPattern* pattern);

    // =========================================================================
    // Gradients & Patterns
    // =========================================================================

    CanvasGradient* createLinearGradient(double x0, double y0, double x1, double y1);
    CanvasGradient* createRadialGradient(double x0, double y0, double r0, double x1, double y1, double r1);
    CanvasGradient* createConicGradient(double startAngle, double x, double y);
    CanvasPattern* createPattern(const ImageData& image, const std::string& repetition);

    // =========================================================================
    // Line Styles
    // =========================================================================

    double lineWidth() const;
    void setLineWidth(double width);

    std::string lineCap() const;
    void setLineCap(const std::string& cap);

    std::string lineJoin() const;
    void setLineJoin(const std::string& join);

    double miterLimit() const;
    void setMiterLimit(double limit);

    std::vector<double> getLineDash() const;
    void setLineDash(const std::vector<double>& segments);

    double lineDashOffset() const;
    void setLineDashOffset(double offset);

    // =========================================================================
    // Text Styles
    // =========================================================================

    std::string font() const;
    void setFont(const std::string& font);

    std::string textAlign() const;
    void setTextAlign(const std::string& align);

    std::string textBaseline() const;
    void setTextBaseline(const std::string& baseline);

    std::string direction() const;
    void setDirection(const std::string& dir);

    std::string fontKerning() const;
    void setFontKerning(const std::string& kerning);

    std::string letterSpacing() const;
    void setLetterSpacing(const std::string& spacing);

    std::string wordSpacing() const;
    void setWordSpacing(const std::string& spacing);

    // =========================================================================
    // Shadows
    // =========================================================================

    double shadowBlur() const;
    void setShadowBlur(double blur);

    std::string shadowColor() const;
    void setShadowColor(const std::string& color);

    double shadowOffsetX() const;
    void setShadowOffsetX(double offset);

    double shadowOffsetY() const;
    void setShadowOffsetY(double offset);

    // =========================================================================
    // Compositing
    // =========================================================================

    double globalAlpha() const;
    void setGlobalAlpha(double alpha);

    std::string globalCompositeOperation() const;
    void setGlobalCompositeOperation(const std::string& op);

    // =========================================================================
    // Image Smoothing
    // =========================================================================

    bool imageSmoothingEnabled() const;
    void setImageSmoothingEnabled(bool enabled);

    std::string imageSmoothingQuality() const;
    void setImageSmoothingQuality(const std::string& quality);

    // =========================================================================
    // Path Methods
    // =========================================================================

    void beginPath();
    void closePath();
    void moveTo(double x, double y);
    void lineTo(double x, double y);
    void bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y);
    void quadraticCurveTo(double cpx, double cpy, double x, double y);
    void arc(double x, double y, double radius, double startAngle, double endAngle, bool counterclockwise = false);
    void arcTo(double x1, double y1, double x2, double y2, double radius);
    void ellipse(double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle, bool counterclockwise = false);
    void rect(double x, double y, double width, double height);
    void roundRect(double x, double y, double width, double height, double radii);

    // =========================================================================
    // Drawing paths
    // =========================================================================

    void fill(FillRule rule = FillRule::NonZero);
    void fill(const Path2D& path, FillRule rule = FillRule::NonZero);
    void stroke();
    void stroke(const Path2D& path);
    void clip(FillRule rule = FillRule::NonZero);
    void clip(const Path2D& path, FillRule rule = FillRule::NonZero);

    bool isPointInPath(double x, double y, FillRule rule = FillRule::NonZero);
    bool isPointInPath(const Path2D& path, double x, double y, FillRule rule = FillRule::NonZero);
    bool isPointInStroke(double x, double y);
    bool isPointInStroke(const Path2D& path, double x, double y);

    // =========================================================================
    // Rectangles
    // =========================================================================

    void clearRect(double x, double y, double width, double height);
    void fillRect(double x, double y, double width, double height);
    void strokeRect(double x, double y, double width, double height);

    // =========================================================================
    // Text
    // =========================================================================

    void fillText(const std::string& text, double x, double y, double maxWidth = -1);
    void strokeText(const std::string& text, double x, double y, double maxWidth = -1);
    TextMetrics measureText(const std::string& text);

    // =========================================================================
    // Pixel Manipulation
    // =========================================================================

    ImageData createImageData(unsigned int width, unsigned int height);
    ImageData createImageData(const ImageData& imagedata);
    ImageData getImageData(int sx, int sy, int sw, int sh);
    void putImageData(const ImageData& imagedata, int dx, int dy);
    void putImageData(const ImageData& imagedata, int dx, int dy, int dirtyX, int dirtyY, int dirtyWidth, int dirtyHeight);

    // =========================================================================
    // Drawing Images
    // =========================================================================

    void drawImage(const ImageData& image, double dx, double dy);
    void drawImage(const ImageData& image, double dx, double dy, double dWidth, double dHeight);
    void drawImage(const ImageData& image, double sx, double sy, double sWidth, double sHeight, double dx, double dy, double dWidth, double dHeight);

private:
    HTMLCanvasElement* canvas_;
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTMLCanvasElement - canvas for 2D/WebGL drawing
 */
class HTMLCanvasElement : public HTMLElement {
public:
    HTMLCanvasElement();
    ~HTMLCanvasElement() override;

    // =========================================================================
    // Properties
    // =========================================================================

    /// Canvas width in pixels (default 300)
    unsigned int width() const;
    void setWidth(unsigned int width);

    /// Canvas height in pixels (default 150)
    unsigned int height() const;
    void setHeight(unsigned int height);

    // =========================================================================
    // Methods
    // =========================================================================

    /**
     * @brief Get rendering context
     * @param contextId "2d", "webgl", "webgl2", "bitmaprenderer"
     * @return Context or nullptr if not supported
     */
    CanvasRenderingContext2D* getContext(const std::string& contextId);

    /**
     * @brief Export canvas as data URL
     * @param type MIME type (default "image/png")
     * @param quality Quality for JPEG (0.0-1.0)
     * @return Data URL string
     */
    std::string toDataURL(const std::string& type = "image/png", double quality = 0.92) const;

    /**
     * @brief Export canvas as Blob (via callback)
     * @param callback Called with blob data
     * @param type MIME type
     * @param quality Quality for JPEG
     */
    void toBlob(std::function<void(const std::vector<uint8_t>&)> callback, 
                const std::string& type = "image/png", double quality = 0.92) const;

    /// Access pixel buffer directly
    std::vector<uint8_t>& pixelBuffer();
    const std::vector<uint8_t>& pixelBuffer() const;

    // =========================================================================
    // Event Handlers
    // =========================================================================

    void setOnContextLost(EventListener callback);
    void setOnContextRestored(EventListener callback);

    // =========================================================================
    // Clone
    // =========================================================================

    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
