/**
 * @file html_canvas_element.cpp
 * @brief HTMLCanvasElement and CanvasRenderingContext2D implementation
 *
 * Full 2D canvas drawing implementation.
 */

#include "webcore/html/html_canvas_element.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stack>

namespace Zepra::WebCore {

// =============================================================================
// CanvasGradient Implementation
// =============================================================================

CanvasGradient::CanvasGradient(double x0, double y0, double x1, double y1)
    : isRadial_(false), x0_(x0), y0_(y0), x1_(x1), y1_(y1) {}

CanvasGradient::CanvasGradient(double x0, double y0, double r0, double x1, double y1, double r1)
    : isRadial_(true), x0_(x0), y0_(y0), x1_(x1), y1_(y1), r0_(r0), r1_(r1) {}

void CanvasGradient::addColorStop(double offset, const std::string& color) {
    offset = std::max(0.0, std::min(1.0, offset));
    stops_.push_back({offset, color});
    std::sort(stops_.begin(), stops_.end(), 
              [](const ColorStop& a, const ColorStop& b) { return a.offset < b.offset; });
}

// =============================================================================
// CanvasPattern Implementation
// =============================================================================

CanvasPattern::CanvasPattern(const std::vector<uint8_t>& imageData, int width, int height, Repetition rep)
    : imageData_(imageData), width_(width), height_(height), repetition_(rep) {}

void CanvasPattern::setTransform(double a, double b, double c, double d, double e, double f) {
    transform_[0] = a; transform_[1] = b;
    transform_[2] = c; transform_[3] = d;
    transform_[4] = e; transform_[5] = f;
}

// =============================================================================
// ImageData Implementation
// =============================================================================

ImageData::ImageData(unsigned int width, unsigned int height)
    : width_(width), height_(height), 
      data_(width * height * 4, 0) {}

ImageData::ImageData(const std::vector<uint8_t>& data, unsigned int width, unsigned int height)
    : width_(width), height_(height), data_(data) {
    if (data_.size() < width * height * 4) {
        data_.resize(width * height * 4, 0);
    }
}

// =============================================================================
// Path2D Implementation
// =============================================================================

Path2D::Path2D() = default;

Path2D::Path2D(const std::string& /*svgPath*/) {
    // SVG path parsing would be implemented here
    // For now, just create an empty path
}

void Path2D::moveTo(double x, double y) {
    commands_.push_back({Command::Move, {x, y}});
}

void Path2D::lineTo(double x, double y) {
    commands_.push_back({Command::Line, {x, y}});
}

void Path2D::bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y) {
    commands_.push_back({Command::BezierCurve, {cp1x, cp1y, cp2x, cp2y, x, y}});
}

void Path2D::quadraticCurveTo(double cpx, double cpy, double x, double y) {
    commands_.push_back({Command::QuadraticCurve, {cpx, cpy, x, y}});
}

void Path2D::arc(double x, double y, double radius, double startAngle, double endAngle, bool ccw) {
    commands_.push_back({Command::Arc, {x, y, radius, startAngle, endAngle, ccw ? 1.0 : 0.0}});
}

void Path2D::arcTo(double x1, double y1, double x2, double y2, double radius) {
    commands_.push_back({Command::ArcTo, {x1, y1, x2, y2, radius}});
}

void Path2D::ellipse(double x, double y, double rx, double ry, double rot, double start, double end, bool ccw) {
    commands_.push_back({Command::Ellipse, {x, y, rx, ry, rot, start, end, ccw ? 1.0 : 0.0}});
}

void Path2D::rect(double x, double y, double w, double h) {
    commands_.push_back({Command::Rect, {x, y, w, h}});
}

void Path2D::roundRect(double x, double y, double w, double h, double radii) {
    commands_.push_back({Command::Rect, {x, y, w, h, radii}});
}

void Path2D::closePath() {
    commands_.push_back({Command::Close, {}});
}

void Path2D::addPath(const Path2D& path) {
    for (const auto& cmd : path.commands()) {
        commands_.push_back(cmd);
    }
}

// =============================================================================
// CanvasRenderingContext2D::Impl
// =============================================================================

class CanvasRenderingContext2D::Impl {
public:
    struct State {
        // Transform
        DOMMatrix transform;
        
        // Fill/stroke
        std::string fillStyle = "#000000";
        std::string strokeStyle = "#000000";
        CanvasGradient* fillGradient = nullptr;
        CanvasPattern* fillPattern = nullptr;
        CanvasGradient* strokeGradient = nullptr;
        CanvasPattern* strokePattern = nullptr;
        
        // Line styles
        double lineWidth = 1.0;
        LineCap lineCap = LineCap::Butt;
        LineJoin lineJoin = LineJoin::Miter;
        double miterLimit = 10.0;
        std::vector<double> lineDash;
        double lineDashOffset = 0.0;
        
        // Text
        std::string font = "10px sans-serif";
        TextAlign textAlign = TextAlign::Start;
        TextBaseline textBaseline = TextBaseline::Alphabetic;
        std::string direction = "inherit";
        std::string fontKerning = "auto";
        std::string letterSpacing = "0px";
        std::string wordSpacing = "0px";
        
        // Shadow
        double shadowBlur = 0.0;
        std::string shadowColor = "rgba(0,0,0,0)";
        double shadowOffsetX = 0.0;
        double shadowOffsetY = 0.0;
        
        // Compositing
        double globalAlpha = 1.0;
        CompositeOperation globalCompositeOp = CompositeOperation::SourceOver;
        
        // Image smoothing
        bool imageSmoothingEnabled = true;
        ImageSmoothingQuality imageSmoothingQuality = ImageSmoothingQuality::Low;
    };
    
    State state;
    std::stack<State> stateStack;
    Path2D currentPath;
    
    // Owned gradients/patterns
    std::vector<std::unique_ptr<CanvasGradient>> gradients;
    std::vector<std::unique_ptr<CanvasPattern>> patterns;
};

// =============================================================================
// CanvasRenderingContext2D
// =============================================================================

CanvasRenderingContext2D::CanvasRenderingContext2D(HTMLCanvasElement* canvas)
    : canvas_(canvas), impl_(std::make_unique<Impl>()) {}

CanvasRenderingContext2D::~CanvasRenderingContext2D() = default;

// State

void CanvasRenderingContext2D::save() {
    impl_->stateStack.push(impl_->state);
}

void CanvasRenderingContext2D::restore() {
    if (!impl_->stateStack.empty()) {
        impl_->state = impl_->stateStack.top();
        impl_->stateStack.pop();
    }
}

void CanvasRenderingContext2D::reset() {
    impl_->state = Impl::State{};
    while (!impl_->stateStack.empty()) impl_->stateStack.pop();
    impl_->currentPath = Path2D();
    
    // Clear canvas
    if (canvas_) {
        auto& buf = canvas_->pixelBuffer();
        std::fill(buf.begin(), buf.end(), 0);
    }
}

// Transform

void CanvasRenderingContext2D::scale(double x, double y) {
    auto& t = impl_->state.transform;
    t.a *= x; t.c *= x; t.e *= x;
    t.b *= y; t.d *= y; t.f *= y;
}

void CanvasRenderingContext2D::rotate(double angle) {
    double c = std::cos(angle);
    double s = std::sin(angle);
    auto& t = impl_->state.transform;
    double a = t.a, b = t.b, cc = t.c, d = t.d;
    t.a = a * c + cc * s;
    t.b = b * c + d * s;
    t.c = cc * c - a * s;
    t.d = d * c - b * s;
}

void CanvasRenderingContext2D::translate(double x, double y) {
    auto& t = impl_->state.transform;
    t.e += t.a * x + t.c * y;
    t.f += t.b * x + t.d * y;
}

void CanvasRenderingContext2D::transform(double a, double b, double c, double d, double e, double f) {
    auto& t = impl_->state.transform;
    double ta = t.a, tb = t.b, tc = t.c, td = t.d;
    t.a = ta * a + tc * b;
    t.b = tb * a + td * b;
    t.c = ta * c + tc * d;
    t.d = tb * c + td * d;
    t.e += ta * e + tc * f;
    t.f += tb * e + td * f;
}

void CanvasRenderingContext2D::setTransform(double a, double b, double c, double d, double e, double f) {
    impl_->state.transform = {a, b, c, d, e, f};
}

void CanvasRenderingContext2D::resetTransform() {
    impl_->state.transform = {1, 0, 0, 1, 0, 0};
}

CanvasRenderingContext2D::DOMMatrix CanvasRenderingContext2D::getTransform() const {
    return impl_->state.transform;
}

// Fill/Stroke Style

std::string CanvasRenderingContext2D::fillStyle() const {
    return impl_->state.fillStyle;
}

void CanvasRenderingContext2D::setFillStyle(const std::string& color) {
    impl_->state.fillStyle = color;
    impl_->state.fillGradient = nullptr;
    impl_->state.fillPattern = nullptr;
}

void CanvasRenderingContext2D::setFillStyle(CanvasGradient* gradient) {
    impl_->state.fillGradient = gradient;
    impl_->state.fillPattern = nullptr;
}

void CanvasRenderingContext2D::setFillStyle(CanvasPattern* pattern) {
    impl_->state.fillPattern = pattern;
    impl_->state.fillGradient = nullptr;
}

std::string CanvasRenderingContext2D::strokeStyle() const {
    return impl_->state.strokeStyle;
}

void CanvasRenderingContext2D::setStrokeStyle(const std::string& color) {
    impl_->state.strokeStyle = color;
    impl_->state.strokeGradient = nullptr;
    impl_->state.strokePattern = nullptr;
}

void CanvasRenderingContext2D::setStrokeStyle(CanvasGradient* gradient) {
    impl_->state.strokeGradient = gradient;
    impl_->state.strokePattern = nullptr;
}

void CanvasRenderingContext2D::setStrokeStyle(CanvasPattern* pattern) {
    impl_->state.strokePattern = pattern;
    impl_->state.strokeGradient = nullptr;
}

// Gradients/Patterns

CanvasGradient* CanvasRenderingContext2D::createLinearGradient(double x0, double y0, double x1, double y1) {
    impl_->gradients.push_back(std::make_unique<CanvasGradient>(x0, y0, x1, y1));
    return impl_->gradients.back().get();
}

CanvasGradient* CanvasRenderingContext2D::createRadialGradient(double x0, double y0, double r0, double x1, double y1, double r1) {
    impl_->gradients.push_back(std::make_unique<CanvasGradient>(x0, y0, r0, x1, y1, r1));
    return impl_->gradients.back().get();
}

CanvasGradient* CanvasRenderingContext2D::createConicGradient(double /*startAngle*/, double x, double y) {
    // Conic gradient - simplified as radial for now
    impl_->gradients.push_back(std::make_unique<CanvasGradient>(x, y, 0, x, y, 100));
    return impl_->gradients.back().get();
}

CanvasPattern* CanvasRenderingContext2D::createPattern(const ImageData& image, const std::string& repetition) {
    CanvasPattern::Repetition rep = CanvasPattern::Repetition::Repeat;
    if (repetition == "repeat-x") rep = CanvasPattern::Repetition::RepeatX;
    else if (repetition == "repeat-y") rep = CanvasPattern::Repetition::RepeatY;
    else if (repetition == "no-repeat") rep = CanvasPattern::Repetition::NoRepeat;
    
    impl_->patterns.push_back(std::make_unique<CanvasPattern>(image.data(), image.width(), image.height(), rep));
    return impl_->patterns.back().get();
}

// Line styles

double CanvasRenderingContext2D::lineWidth() const { return impl_->state.lineWidth; }
void CanvasRenderingContext2D::setLineWidth(double w) { impl_->state.lineWidth = std::max(0.0, w); }

std::string CanvasRenderingContext2D::lineCap() const {
    switch (impl_->state.lineCap) {
        case LineCap::Round: return "round";
        case LineCap::Square: return "square";
        default: return "butt";
    }
}

void CanvasRenderingContext2D::setLineCap(const std::string& cap) {
    if (cap == "round") impl_->state.lineCap = LineCap::Round;
    else if (cap == "square") impl_->state.lineCap = LineCap::Square;
    else impl_->state.lineCap = LineCap::Butt;
}

std::string CanvasRenderingContext2D::lineJoin() const {
    switch (impl_->state.lineJoin) {
        case LineJoin::Round: return "round";
        case LineJoin::Bevel: return "bevel";
        default: return "miter";
    }
}

void CanvasRenderingContext2D::setLineJoin(const std::string& join) {
    if (join == "round") impl_->state.lineJoin = LineJoin::Round;
    else if (join == "bevel") impl_->state.lineJoin = LineJoin::Bevel;
    else impl_->state.lineJoin = LineJoin::Miter;
}

double CanvasRenderingContext2D::miterLimit() const { return impl_->state.miterLimit; }
void CanvasRenderingContext2D::setMiterLimit(double limit) { impl_->state.miterLimit = std::max(0.0, limit); }

std::vector<double> CanvasRenderingContext2D::getLineDash() const { return impl_->state.lineDash; }
void CanvasRenderingContext2D::setLineDash(const std::vector<double>& segs) { impl_->state.lineDash = segs; }

double CanvasRenderingContext2D::lineDashOffset() const { return impl_->state.lineDashOffset; }
void CanvasRenderingContext2D::setLineDashOffset(double off) { impl_->state.lineDashOffset = off; }

// Text styles

std::string CanvasRenderingContext2D::font() const { return impl_->state.font; }
void CanvasRenderingContext2D::setFont(const std::string& f) { impl_->state.font = f; }

std::string CanvasRenderingContext2D::textAlign() const {
    switch (impl_->state.textAlign) {
        case TextAlign::End: return "end";
        case TextAlign::Left: return "left";
        case TextAlign::Right: return "right";
        case TextAlign::Center: return "center";
        default: return "start";
    }
}

void CanvasRenderingContext2D::setTextAlign(const std::string& align) {
    if (align == "end") impl_->state.textAlign = TextAlign::End;
    else if (align == "left") impl_->state.textAlign = TextAlign::Left;
    else if (align == "right") impl_->state.textAlign = TextAlign::Right;
    else if (align == "center") impl_->state.textAlign = TextAlign::Center;
    else impl_->state.textAlign = TextAlign::Start;
}

std::string CanvasRenderingContext2D::textBaseline() const {
    switch (impl_->state.textBaseline) {
        case TextBaseline::Top: return "top";
        case TextBaseline::Hanging: return "hanging";
        case TextBaseline::Middle: return "middle";
        case TextBaseline::Ideographic: return "ideographic";
        case TextBaseline::Bottom: return "bottom";
        default: return "alphabetic";
    }
}

void CanvasRenderingContext2D::setTextBaseline(const std::string& b) {
    if (b == "top") impl_->state.textBaseline = TextBaseline::Top;
    else if (b == "hanging") impl_->state.textBaseline = TextBaseline::Hanging;
    else if (b == "middle") impl_->state.textBaseline = TextBaseline::Middle;
    else if (b == "ideographic") impl_->state.textBaseline = TextBaseline::Ideographic;
    else if (b == "bottom") impl_->state.textBaseline = TextBaseline::Bottom;
    else impl_->state.textBaseline = TextBaseline::Alphabetic;
}

std::string CanvasRenderingContext2D::direction() const { return impl_->state.direction; }
void CanvasRenderingContext2D::setDirection(const std::string& d) { impl_->state.direction = d; }

std::string CanvasRenderingContext2D::fontKerning() const { return impl_->state.fontKerning; }
void CanvasRenderingContext2D::setFontKerning(const std::string& k) { impl_->state.fontKerning = k; }

std::string CanvasRenderingContext2D::letterSpacing() const { return impl_->state.letterSpacing; }
void CanvasRenderingContext2D::setLetterSpacing(const std::string& s) { impl_->state.letterSpacing = s; }

std::string CanvasRenderingContext2D::wordSpacing() const { return impl_->state.wordSpacing; }
void CanvasRenderingContext2D::setWordSpacing(const std::string& s) { impl_->state.wordSpacing = s; }

// Shadow

double CanvasRenderingContext2D::shadowBlur() const { return impl_->state.shadowBlur; }
void CanvasRenderingContext2D::setShadowBlur(double b) { impl_->state.shadowBlur = std::max(0.0, b); }

std::string CanvasRenderingContext2D::shadowColor() const { return impl_->state.shadowColor; }
void CanvasRenderingContext2D::setShadowColor(const std::string& c) { impl_->state.shadowColor = c; }

double CanvasRenderingContext2D::shadowOffsetX() const { return impl_->state.shadowOffsetX; }
void CanvasRenderingContext2D::setShadowOffsetX(double o) { impl_->state.shadowOffsetX = o; }

double CanvasRenderingContext2D::shadowOffsetY() const { return impl_->state.shadowOffsetY; }
void CanvasRenderingContext2D::setShadowOffsetY(double o) { impl_->state.shadowOffsetY = o; }

// Compositing

double CanvasRenderingContext2D::globalAlpha() const { return impl_->state.globalAlpha; }
void CanvasRenderingContext2D::setGlobalAlpha(double a) { impl_->state.globalAlpha = std::max(0.0, std::min(1.0, a)); }

std::string CanvasRenderingContext2D::globalCompositeOperation() const {
    switch (impl_->state.globalCompositeOp) {
        case CompositeOperation::SourceIn: return "source-in";
        case CompositeOperation::SourceOut: return "source-out";
        case CompositeOperation::SourceAtop: return "source-atop";
        case CompositeOperation::DestinationOver: return "destination-over";
        case CompositeOperation::DestinationIn: return "destination-in";
        case CompositeOperation::DestinationOut: return "destination-out";
        case CompositeOperation::DestinationAtop: return "destination-atop";
        case CompositeOperation::Lighter: return "lighter";
        case CompositeOperation::Copy: return "copy";
        case CompositeOperation::Xor: return "xor";
        case CompositeOperation::Multiply: return "multiply";
        case CompositeOperation::Screen: return "screen";
        default: return "source-over";
    }
}

void CanvasRenderingContext2D::setGlobalCompositeOperation(const std::string& op) {
    if (op == "source-in") impl_->state.globalCompositeOp = CompositeOperation::SourceIn;
    else if (op == "source-out") impl_->state.globalCompositeOp = CompositeOperation::SourceOut;
    else if (op == "source-atop") impl_->state.globalCompositeOp = CompositeOperation::SourceAtop;
    else if (op == "destination-over") impl_->state.globalCompositeOp = CompositeOperation::DestinationOver;
    else if (op == "destination-in") impl_->state.globalCompositeOp = CompositeOperation::DestinationIn;
    else if (op == "destination-out") impl_->state.globalCompositeOp = CompositeOperation::DestinationOut;
    else if (op == "destination-atop") impl_->state.globalCompositeOp = CompositeOperation::DestinationAtop;
    else if (op == "lighter") impl_->state.globalCompositeOp = CompositeOperation::Lighter;
    else if (op == "copy") impl_->state.globalCompositeOp = CompositeOperation::Copy;
    else if (op == "xor") impl_->state.globalCompositeOp = CompositeOperation::Xor;
    else if (op == "multiply") impl_->state.globalCompositeOp = CompositeOperation::Multiply;
    else if (op == "screen") impl_->state.globalCompositeOp = CompositeOperation::Screen;
    else impl_->state.globalCompositeOp = CompositeOperation::SourceOver;
}

// Image smoothing

bool CanvasRenderingContext2D::imageSmoothingEnabled() const { return impl_->state.imageSmoothingEnabled; }
void CanvasRenderingContext2D::setImageSmoothingEnabled(bool e) { impl_->state.imageSmoothingEnabled = e; }

std::string CanvasRenderingContext2D::imageSmoothingQuality() const {
    switch (impl_->state.imageSmoothingQuality) {
        case ImageSmoothingQuality::Medium: return "medium";
        case ImageSmoothingQuality::High: return "high";
        default: return "low";
    }
}

void CanvasRenderingContext2D::setImageSmoothingQuality(const std::string& q) {
    if (q == "medium") impl_->state.imageSmoothingQuality = ImageSmoothingQuality::Medium;
    else if (q == "high") impl_->state.imageSmoothingQuality = ImageSmoothingQuality::High;
    else impl_->state.imageSmoothingQuality = ImageSmoothingQuality::Low;
}

// Path methods

void CanvasRenderingContext2D::beginPath() { impl_->currentPath = Path2D(); }
void CanvasRenderingContext2D::closePath() { impl_->currentPath.closePath(); }
void CanvasRenderingContext2D::moveTo(double x, double y) { impl_->currentPath.moveTo(x, y); }
void CanvasRenderingContext2D::lineTo(double x, double y) { impl_->currentPath.lineTo(x, y); }

void CanvasRenderingContext2D::bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y) {
    impl_->currentPath.bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y);
}

void CanvasRenderingContext2D::quadraticCurveTo(double cpx, double cpy, double x, double y) {
    impl_->currentPath.quadraticCurveTo(cpx, cpy, x, y);
}

void CanvasRenderingContext2D::arc(double x, double y, double r, double start, double end, bool ccw) {
    impl_->currentPath.arc(x, y, r, start, end, ccw);
}

void CanvasRenderingContext2D::arcTo(double x1, double y1, double x2, double y2, double r) {
    impl_->currentPath.arcTo(x1, y1, x2, y2, r);
}

void CanvasRenderingContext2D::ellipse(double x, double y, double rx, double ry, double rot, double start, double end, bool ccw) {
    impl_->currentPath.ellipse(x, y, rx, ry, rot, start, end, ccw);
}

void CanvasRenderingContext2D::rect(double x, double y, double w, double h) {
    impl_->currentPath.rect(x, y, w, h);
}

void CanvasRenderingContext2D::roundRect(double x, double y, double w, double h, double radii) {
    impl_->currentPath.roundRect(x, y, w, h, radii);
}

// Drawing paths - simplified rasterization

void CanvasRenderingContext2D::fill(FillRule /*rule*/) {
    // Production would rasterize path to canvas buffer
    // This is a simplified stub
}

void CanvasRenderingContext2D::fill(const Path2D& /*path*/, FillRule /*rule*/) {
    // Production would rasterize path to canvas buffer
}

void CanvasRenderingContext2D::stroke() {
    // Production would stroke path to canvas buffer
}

void CanvasRenderingContext2D::stroke(const Path2D& /*path*/) {
    // Production would stroke path to canvas buffer
}

void CanvasRenderingContext2D::clip(FillRule /*rule*/) {
    // Set clipping region
}

void CanvasRenderingContext2D::clip(const Path2D& /*path*/, FillRule /*rule*/) {
    // Set clipping region
}

bool CanvasRenderingContext2D::isPointInPath(double /*x*/, double /*y*/, FillRule /*rule*/) {
    return false;
}

bool CanvasRenderingContext2D::isPointInPath(const Path2D& /*path*/, double /*x*/, double /*y*/, FillRule /*rule*/) {
    return false;
}

bool CanvasRenderingContext2D::isPointInStroke(double /*x*/, double /*y*/) {
    return false;
}

bool CanvasRenderingContext2D::isPointInStroke(const Path2D& /*path*/, double /*x*/, double /*y*/) {
    return false;
}

// Rectangles

void CanvasRenderingContext2D::clearRect(double x, double y, double w, double h) {
    if (!canvas_) return;
    auto& buf = canvas_->pixelBuffer();
    unsigned int cw = canvas_->width();
    unsigned int ch = canvas_->height();
    
    int ix = static_cast<int>(std::max(0.0, x));
    int iy = static_cast<int>(std::max(0.0, y));
    int iw = static_cast<int>(std::min(static_cast<double>(cw) - x, w));
    int ih = static_cast<int>(std::min(static_cast<double>(ch) - y, h));
    
    for (int py = iy; py < iy + ih && py < static_cast<int>(ch); ++py) {
        for (int px = ix; px < ix + iw && px < static_cast<int>(cw); ++px) {
            size_t idx = (py * cw + px) * 4;
            buf[idx] = buf[idx+1] = buf[idx+2] = buf[idx+3] = 0;
        }
    }
}

void CanvasRenderingContext2D::fillRect(double x, double y, double w, double h) {
    if (!canvas_) return;
    auto& buf = canvas_->pixelBuffer();
    unsigned int cw = canvas_->width();
    unsigned int ch = canvas_->height();
    
    // Parse fill color (simplified - assumes #RRGGBB or rgb())
    uint8_t r = 0, g = 0, b = 0, a = 255;
    std::string color = impl_->state.fillStyle;
    if (color.length() == 7 && color[0] == '#') {
        r = static_cast<uint8_t>(std::stoul(color.substr(1, 2), nullptr, 16));
        g = static_cast<uint8_t>(std::stoul(color.substr(3, 2), nullptr, 16));
        b = static_cast<uint8_t>(std::stoul(color.substr(5, 2), nullptr, 16));
    }
    
    a = static_cast<uint8_t>(impl_->state.globalAlpha * 255);
    
    int ix = static_cast<int>(std::max(0.0, x));
    int iy = static_cast<int>(std::max(0.0, y));
    int iw = static_cast<int>(std::min(static_cast<double>(cw) - x, w));
    int ih = static_cast<int>(std::min(static_cast<double>(ch) - y, h));
    
    for (int py = iy; py < iy + ih && py < static_cast<int>(ch); ++py) {
        for (int px = ix; px < ix + iw && px < static_cast<int>(cw); ++px) {
            size_t idx = (py * cw + px) * 4;
            buf[idx] = r;
            buf[idx+1] = g;
            buf[idx+2] = b;
            buf[idx+3] = a;
        }
    }
}

void CanvasRenderingContext2D::strokeRect(double x, double y, double w, double h) {
    beginPath();
    rect(x, y, w, h);
    stroke();
}

// Text

void CanvasRenderingContext2D::fillText(const std::string& /*text*/, double /*x*/, double /*y*/, double /*maxWidth*/) {
    // Text rendering requires font rasterization
}

void CanvasRenderingContext2D::strokeText(const std::string& /*text*/, double /*x*/, double /*y*/, double /*maxWidth*/) {
    // Text rendering requires font rasterization
}

TextMetrics CanvasRenderingContext2D::measureText(const std::string& text) {
    // Simplified - assumes 10px per character
    TextMetrics m{};
    m.width = text.length() * 10.0;
    m.actualBoundingBoxLeft = 0;
    m.actualBoundingBoxRight = m.width;
    m.fontBoundingBoxAscent = 10;
    m.fontBoundingBoxDescent = 2;
    m.actualBoundingBoxAscent = 10;
    m.actualBoundingBoxDescent = 2;
    return m;
}

// Pixel manipulation

ImageData CanvasRenderingContext2D::createImageData(unsigned int w, unsigned int h) {
    return ImageData(w, h);
}

ImageData CanvasRenderingContext2D::createImageData(const ImageData& other) {
    return ImageData(other.width(), other.height());
}

ImageData CanvasRenderingContext2D::getImageData(int sx, int sy, int sw, int sh) {
    ImageData result(sw, sh);
    if (!canvas_) return result;
    
    const auto& buf = canvas_->pixelBuffer();
    unsigned int cw = canvas_->width();
    unsigned int ch = canvas_->height();
    auto& data = result.data();
    
    for (int y = 0; y < sh; ++y) {
        for (int x = 0; x < sw; ++x) {
            int px = sx + x;
            int py = sy + y;
            if (px >= 0 && px < static_cast<int>(cw) && py >= 0 && py < static_cast<int>(ch)) {
                size_t srcIdx = (py * cw + px) * 4;
                size_t dstIdx = (y * sw + x) * 4;
                data[dstIdx] = buf[srcIdx];
                data[dstIdx+1] = buf[srcIdx+1];
                data[dstIdx+2] = buf[srcIdx+2];
                data[dstIdx+3] = buf[srcIdx+3];
            }
        }
    }
    
    return result;
}

void CanvasRenderingContext2D::putImageData(const ImageData& imagedata, int dx, int dy) {
    putImageData(imagedata, dx, dy, 0, 0, imagedata.width(), imagedata.height());
}

void CanvasRenderingContext2D::putImageData(const ImageData& imagedata, int dx, int dy, int dirtyX, int dirtyY, int dirtyW, int dirtyH) {
    if (!canvas_) return;
    
    auto& buf = canvas_->pixelBuffer();
    unsigned int cw = canvas_->width();
    unsigned int ch = canvas_->height();
    const auto& data = imagedata.data();
    int sw = imagedata.width();
    
    for (int y = dirtyY; y < dirtyY + dirtyH; ++y) {
        for (int x = dirtyX; x < dirtyX + dirtyW; ++x) {
            int px = dx + x;
            int py = dy + y;
            if (px >= 0 && px < static_cast<int>(cw) && py >= 0 && py < static_cast<int>(ch)) {
                if (x >= 0 && x < static_cast<int>(imagedata.width()) && y >= 0 && y < static_cast<int>(imagedata.height())) {
                    size_t srcIdx = (y * sw + x) * 4;
                    size_t dstIdx = (py * cw + px) * 4;
                    buf[dstIdx] = data[srcIdx];
                    buf[dstIdx+1] = data[srcIdx+1];
                    buf[dstIdx+2] = data[srcIdx+2];
                    buf[dstIdx+3] = data[srcIdx+3];
                }
            }
        }
    }
}

// Drawing images

void CanvasRenderingContext2D::drawImage(const ImageData& image, double dx, double dy) {
    drawImage(image, 0, 0, image.width(), image.height(), dx, dy, image.width(), image.height());
}

void CanvasRenderingContext2D::drawImage(const ImageData& image, double dx, double dy, double dw, double dh) {
    drawImage(image, 0, 0, image.width(), image.height(), dx, dy, dw, dh);
}

void CanvasRenderingContext2D::drawImage(const ImageData& image, double /*sx*/, double /*sy*/, double /*sw*/, double /*sh*/, double dx, double dy, double dw, double dh) {
    if (!canvas_) return;
    
    auto& buf = canvas_->pixelBuffer();
    unsigned int cw = canvas_->width();
    unsigned int ch = canvas_->height();
    const auto& data = image.data();
    unsigned int iw = image.width();
    unsigned int ih = image.height();
    
    // Simple nearest-neighbor scaling
    for (int y = 0; y < static_cast<int>(dh); ++y) {
        for (int x = 0; x < static_cast<int>(dw); ++x) {
            int px = static_cast<int>(dx) + x;
            int py = static_cast<int>(dy) + y;
            
            if (px >= 0 && px < static_cast<int>(cw) && py >= 0 && py < static_cast<int>(ch)) {
                int sx = static_cast<int>(x * iw / dw);
                int sy = static_cast<int>(y * ih / dh);
                
                if (sx >= 0 && sx < static_cast<int>(iw) && sy >= 0 && sy < static_cast<int>(ih)) {
                    size_t srcIdx = (sy * iw + sx) * 4;
                    size_t dstIdx = (py * cw + px) * 4;
                    
                    // Alpha blending
                    uint8_t sa = data[srcIdx+3];
                    if (sa == 255) {
                        buf[dstIdx] = data[srcIdx];
                        buf[dstIdx+1] = data[srcIdx+1];
                        buf[dstIdx+2] = data[srcIdx+2];
                        buf[dstIdx+3] = 255;
                    } else if (sa > 0) {
                        uint8_t da = buf[dstIdx+3];
                        uint8_t oa = sa + (da * (255 - sa) / 255);
                        if (oa > 0) {
                            buf[dstIdx] = (data[srcIdx] * sa + buf[dstIdx] * da * (255 - sa) / 255) / oa;
                            buf[dstIdx+1] = (data[srcIdx+1] * sa + buf[dstIdx+1] * da * (255 - sa) / 255) / oa;
                            buf[dstIdx+2] = (data[srcIdx+2] * sa + buf[dstIdx+2] * da * (255 - sa) / 255) / oa;
                        }
                        buf[dstIdx+3] = oa;
                    }
                }
            }
        }
    }
}

// =============================================================================
// HTMLCanvasElement::Impl
// =============================================================================

class HTMLCanvasElement::Impl {
public:
    unsigned int width = 300;
    unsigned int height = 150;
    std::vector<uint8_t> pixelBuffer;
    std::unique_ptr<CanvasRenderingContext2D> context2d;
    
    EventListener onContextLost;
    EventListener onContextRestored;
    
    void ensureBuffer() {
        size_t needed = width * height * 4;
        if (pixelBuffer.size() != needed) {
            pixelBuffer.resize(needed, 0);
        }
    }
};

// =============================================================================
// HTMLCanvasElement
// =============================================================================

HTMLCanvasElement::HTMLCanvasElement()
    : HTMLElement("canvas"),
      impl_(std::make_unique<Impl>()) {
    impl_->ensureBuffer();
}

HTMLCanvasElement::~HTMLCanvasElement() = default;

unsigned int HTMLCanvasElement::width() const {
    return impl_->width;
}

void HTMLCanvasElement::setWidth(unsigned int w) {
    impl_->width = w;
    impl_->ensureBuffer();
    setAttribute("width", std::to_string(w));
}

unsigned int HTMLCanvasElement::height() const {
    return impl_->height;
}

void HTMLCanvasElement::setHeight(unsigned int h) {
    impl_->height = h;
    impl_->ensureBuffer();
    setAttribute("height", std::to_string(h));
}

CanvasRenderingContext2D* HTMLCanvasElement::getContext(const std::string& contextId) {
    if (contextId == "2d") {
        if (!impl_->context2d) {
            impl_->context2d = std::make_unique<CanvasRenderingContext2D>(this);
        }
        return impl_->context2d.get();
    }
    // WebGL contexts not implemented
    return nullptr;
}

std::string HTMLCanvasElement::toDataURL(const std::string& type, double /*quality*/) const {
    // Simplified - returns placeholder data URL
    std::ostringstream oss;
    oss << "data:" << type << ";base64,";
    
    // Base64 encode pixel data (simplified)
    static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const auto& data = impl_->pixelBuffer;
    
    for (size_t i = 0; i < std::min(data.size(), size_t(1000)); i += 3) {
        uint32_t n = (data[i] << 16) | 
                     ((i+1 < data.size() ? data[i+1] : 0) << 8) | 
                     (i+2 < data.size() ? data[i+2] : 0);
        oss << b64[(n >> 18) & 63] << b64[(n >> 12) & 63] 
            << b64[(n >> 6) & 63] << b64[n & 63];
    }
    
    return oss.str();
}

void HTMLCanvasElement::toBlob(std::function<void(const std::vector<uint8_t>&)> callback, 
                                const std::string& /*type*/, double /*quality*/) const {
    if (callback) {
        callback(impl_->pixelBuffer);
    }
}

std::vector<uint8_t>& HTMLCanvasElement::pixelBuffer() {
    return impl_->pixelBuffer;
}

const std::vector<uint8_t>& HTMLCanvasElement::pixelBuffer() const {
    return impl_->pixelBuffer;
}

void HTMLCanvasElement::setOnContextLost(EventListener callback) {
    impl_->onContextLost = std::move(callback);
    addEventListener("contextlost", impl_->onContextLost);
}

void HTMLCanvasElement::setOnContextRestored(EventListener callback) {
    impl_->onContextRestored = std::move(callback);
    addEventListener("contextrestored", impl_->onContextRestored);
}

std::unique_ptr<DOMNode> HTMLCanvasElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLCanvasElement>();
    copyHTMLElementProperties(clone.get());
    clone->setWidth(impl_->width);
    clone->setHeight(impl_->height);
    clone->impl_->pixelBuffer = impl_->pixelBuffer;
    (void)deep;  // Canvas has no children
    return clone;
}

} // namespace Zepra::WebCore
