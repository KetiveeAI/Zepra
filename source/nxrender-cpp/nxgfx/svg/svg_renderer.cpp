// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "svg_renderer.h"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <cctype>

namespace NXRender {
namespace SVG {

// ==================================================================
// SVGTransform
// ==================================================================

SVGTransform SVGTransform::identity() {
    return {{1, 0, 0, 1, 0, 0}};
}

SVGTransform SVGTransform::translate(float tx, float ty) {
    return {{1, 0, 0, 1, tx, ty}};
}

SVGTransform SVGTransform::scale(float sx, float sy) {
    return {{sx, 0, 0, sy, 0, 0}};
}

SVGTransform SVGTransform::rotate(float angleDeg, float cx, float cy) {
    float rad = angleDeg * 3.14159265f / 180.0f;
    float c = std::cos(rad), s = std::sin(rad);
    return {{c, s, -s, c,
             -cx * c + cy * s + cx,
             -cx * s - cy * c + cy}};
}

SVGTransform SVGTransform::skewX(float angleDeg) {
    float t = std::tan(angleDeg * 3.14159265f / 180.0f);
    return {{1, 0, t, 1, 0, 0}};
}

SVGTransform SVGTransform::skewY(float angleDeg) {
    float t = std::tan(angleDeg * 3.14159265f / 180.0f);
    return {{1, t, 0, 1, 0, 0}};
}

SVGTransform SVGTransform::fromMatrix(float a, float b, float c, float d, float e, float f) {
    return {{a, b, c, d, e, f}};
}

SVGTransform SVGTransform::operator*(const SVGTransform& other) const {
    SVGTransform result;
    result.matrix[0] = matrix[0] * other.matrix[0] + matrix[2] * other.matrix[1];
    result.matrix[1] = matrix[1] * other.matrix[0] + matrix[3] * other.matrix[1];
    result.matrix[2] = matrix[0] * other.matrix[2] + matrix[2] * other.matrix[3];
    result.matrix[3] = matrix[1] * other.matrix[2] + matrix[3] * other.matrix[3];
    result.matrix[4] = matrix[0] * other.matrix[4] + matrix[2] * other.matrix[5] + matrix[4];
    result.matrix[5] = matrix[1] * other.matrix[4] + matrix[3] * other.matrix[5] + matrix[5];
    return result;
}

Point SVGTransform::apply(const Point& p) const {
    return {
        matrix[0] * p.x + matrix[2] * p.y + matrix[4],
        matrix[1] * p.x + matrix[3] * p.y + matrix[5]
    };
}

SVGTransform SVGTransform::parse(const std::string& str) {
    SVGTransform result = identity();
    size_t pos = 0;

    while (pos < str.size()) {
        while (pos < str.size() && std::isspace(str[pos])) pos++;
        if (pos >= str.size()) break;

        // Read function name
        size_t nameStart = pos;
        while (pos < str.size() && std::isalpha(str[pos])) pos++;
        std::string name = str.substr(nameStart, pos - nameStart);

        while (pos < str.size() && str[pos] != '(') pos++;
        if (pos >= str.size()) break;
        pos++; // skip '('

        // Read args
        std::vector<float> args;
        while (pos < str.size() && str[pos] != ')') {
            while (pos < str.size() && (std::isspace(str[pos]) || str[pos] == ',')) pos++;
            if (str[pos] == ')') break;
            char* end;
            float val = std::strtof(&str[pos], &end);
            args.push_back(val);
            pos = end - str.c_str();
        }
        if (pos < str.size()) pos++; // skip ')'

        SVGTransform t;
        if (name == "translate") {
            float tx = args.size() > 0 ? args[0] : 0;
            float ty = args.size() > 1 ? args[1] : 0;
            t = translate(tx, ty);
        } else if (name == "scale") {
            float sx = args.size() > 0 ? args[0] : 1;
            float sy = args.size() > 1 ? args[1] : sx;
            t = scale(sx, sy);
        } else if (name == "rotate") {
            float angle = args.size() > 0 ? args[0] : 0;
            float cx = args.size() > 1 ? args[1] : 0;
            float cy = args.size() > 2 ? args[2] : 0;
            t = rotate(angle, cx, cy);
        } else if (name == "skewX") {
            t = skewX(args.size() > 0 ? args[0] : 0);
        } else if (name == "skewY") {
            t = skewY(args.size() > 0 ? args[0] : 0);
        } else if (name == "matrix" && args.size() >= 6) {
            t = fromMatrix(args[0], args[1], args[2], args[3], args[4], args[5]);
        }

        result = result * t;
    }

    return result;
}

// ==================================================================
// SVGPaint
// ==================================================================

SVGPaint SVGPaint::parse(const std::string& str) {
    if (str.empty() || str == "none") return none();
    if (str == "currentColor") return {Type::CurrentColor};
    if (str == "inherit") return {Type::Inherit};

    if (str.find("url(") == 0) {
        SVGPaint p;
        p.type = Type::URL;
        auto hash = str.find('#');
        auto end = str.find(')');
        if (hash != std::string::npos && end != std::string::npos) {
            p.url = str.substr(hash + 1, end - hash - 1);
        }
        return p;
    }

    SVGPaint p;
    p.type = Type::Color;
    // Hex color
    if (str[0] == '#') {
        uint32_t hex = 0;
        std::string h = str.substr(1);
        if (h.size() == 3) {
            h = {h[0], h[0], h[1], h[1], h[2], h[2]};
        }
        hex = static_cast<uint32_t>(std::strtoul(h.c_str(), nullptr, 16));
        p.color = Color(hex);
    } else if (str.find("rgb") == 0) {
        auto paren = str.find('(');
        auto end = str.rfind(')');
        if (paren != std::string::npos && end != std::string::npos) {
            std::string inner = str.substr(paren + 1, end - paren - 1);
            int r = 0, g = 0, b = 0;
            sscanf(inner.c_str(), "%d,%d,%d", &r, &g, &b);
            p.color = Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g),
                            static_cast<uint8_t>(b));
        }
    } else {
        // Named color (basic set)
        if (str == "black") p.color = Color::black();
        else if (str == "white") p.color = Color::white();
        else if (str == "red") p.color = Color(255, 0, 0);
        else if (str == "green") p.color = Color(0, 128, 0);
        else if (str == "blue") p.color = Color(0, 0, 255);
        else if (str == "yellow") p.color = Color(255, 255, 0);
        else if (str == "cyan") p.color = Color(0, 255, 255);
        else if (str == "magenta") p.color = Color(255, 0, 255);
        else if (str == "orange") p.color = Color(255, 165, 0);
        else if (str == "gray" || str == "grey") p.color = Color(128, 128, 128);
        else if (str == "transparent") { p.color = Color(0, 0, 0, 0); }
        else p.color = Color::black();
    }
    return p;
}

// ==================================================================
// SVGNode
// ==================================================================

Rect SVGNode::computeBBox() const {
    switch (type) {
        case Type::Rect:
            return {x, y, width, height};
        case Type::Circle:
            return {cx - r, cy - r, r * 2, r * 2};
        case Type::Ellipse:
            return {cx - rx, cy - ry, rx * 2, ry * 2};
        case Type::Line:
            return {std::min(x1, x2), std::min(y1, y2),
                    std::abs(x2 - x1), std::abs(y2 - y1)};
        case Type::Path:
            return path.bounds();
        default:
            break;
    }
    // Compute from children
    float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
    for (const auto& child : children) {
        Rect b = child->computeBBox();
        minX = std::min(minX, b.x);
        minY = std::min(minY, b.y);
        maxX = std::max(maxX, b.x + b.width);
        maxY = std::max(maxY, b.y + b.height);
    }
    if (minX > maxX) return {0, 0, 0, 0};
    return {minX, minY, maxX - minX, maxY - minY};
}

// ==================================================================
// SVGDocument
// ==================================================================

SVGDocument::SVGDocument() {}
SVGDocument::~SVGDocument() {}

bool SVGDocument::parse(const uint8_t* data, size_t size) {
    return parse(std::string(reinterpret_cast<const char*>(data), size));
}

bool SVGDocument::parse(const std::string& svgSource) {
    root_ = std::make_unique<SVGNode>();
    root_->type = SVGNode::Type::SVG;

    // Minimal XML parser — extracts attributes and builds tree
    // Production code would use a real XML parser (expat, pugixml)
    // This stub creates the document structure for API correctness

    // Extract width/height from root <svg> tag
    auto findAttr = [&](const std::string& src, const std::string& attr) -> std::string {
        auto pos = src.find(attr + "=");
        if (pos == std::string::npos) return "";
        pos += attr.size() + 1;
        if (pos >= src.size()) return "";
        char quote = src[pos];
        if (quote != '"' && quote != '\'') return "";
        pos++;
        auto end = src.find(quote, pos);
        if (end == std::string::npos) return "";
        return src.substr(pos, end - pos);
    };

    auto svgTag = svgSource.find("<svg");
    if (svgTag != std::string::npos) {
        auto tagEnd = svgSource.find('>', svgTag);
        std::string tag = svgSource.substr(svgTag, tagEnd - svgTag + 1);

        std::string w = findAttr(tag, "width");
        std::string h = findAttr(tag, "height");
        if (!w.empty()) width_ = std::strtof(w.c_str(), nullptr);
        if (!h.empty()) height_ = std::strtof(h.c_str(), nullptr);

        std::string vb = findAttr(tag, "viewBox");
        if (!vb.empty()) {
            float vx = 0, vy = 0, vw = 0, vh = 0;
            sscanf(vb.c_str(), "%f %f %f %f", &vx, &vy, &vw, &vh);
            viewBox_ = {vx, vy, vw, vh};
            if (width_ == 0) width_ = vw;
            if (height_ == 0) height_ = vh;
        }

        root_->width = width_;
        root_->height = height_;
        root_->viewBox = viewBox_;
    }

    buildIdMap(root_.get());
    return true;
}

SVGNode* SVGDocument::getElementById(const std::string& id) const {
    auto it = idMap_.find(id);
    return (it != idMap_.end()) ? it->second : nullptr;
}

SVGLinearGradient* SVGDocument::linearGradient(const std::string& id) const {
    auto it = linearGradients_.find(id);
    return (it != linearGradients_.end()) ? const_cast<SVGLinearGradient*>(&it->second) : nullptr;
}

SVGRadialGradient* SVGDocument::radialGradient(const std::string& id) const {
    auto it = radialGradients_.find(id);
    return (it != radialGradients_.end()) ? const_cast<SVGRadialGradient*>(&it->second) : nullptr;
}

SVGFilter* SVGDocument::filter(const std::string& id) const {
    auto it = filters_.find(id);
    return (it != filters_.end()) ? const_cast<SVGFilter*>(&it->second) : nullptr;
}

SVGClipPath* SVGDocument::clipPath(const std::string& id) const {
    auto it = clipPaths_.find(id);
    return (it != clipPaths_.end()) ? const_cast<SVGClipPath*>(&it->second) : nullptr;
}

SVGMask* SVGDocument::mask(const std::string& id) const {
    auto it = masks_.find(id);
    return (it != masks_.end()) ? const_cast<SVGMask*>(&it->second) : nullptr;
}

SVGPattern* SVGDocument::pattern(const std::string& id) const {
    auto it = patterns_.find(id);
    return (it != patterns_.end()) ? const_cast<SVGPattern*>(&it->second) : nullptr;
}

void SVGDocument::buildIdMap(SVGNode* node) {
    if (!node) return;
    if (!node->id.empty()) idMap_[node->id] = node;
    for (auto& child : node->children) {
        child->parent = node;
        buildIdMap(child.get());
    }
}

void SVGDocument::resolveStyles(SVGNode* node) {
    if (!node) return;
    // Style inheritance from parent
    if (node->parent) {
        auto& ps = node->parent->style;
        auto& s = node->style;
        // Inherited properties
        if (s.fill.type == SVGPaint::Type::Inherit) s.fill = ps.fill;
        if (s.stroke.type == SVGPaint::Type::Inherit) s.stroke = ps.stroke;
        // Font properties always inherit
        if (s.fontSize == 16 && ps.fontSize != 16) s.fontSize = ps.fontSize;
        if (s.fontFamily == "sans-serif" && ps.fontFamily != "sans-serif")
            s.fontFamily = ps.fontFamily;
    }
    for (auto& child : node->children) {
        resolveStyles(child.get());
    }
}

// ==================================================================
// SVGRenderer
// ==================================================================

std::vector<uint8_t> SVGRenderer::render(const SVGDocument& doc, const RenderConfig& config) {
    int w = (config.width > 0) ? config.width : static_cast<int>(doc.width());
    int h = (config.height > 0) ? config.height : static_cast<int>(doc.height());
    if (w <= 0) w = 300;
    if (h <= 0) h = 150;

    int stride = w * 4;
    std::vector<uint8_t> pixels(static_cast<size_t>(stride) * h, 0);

    // Fill background
    for (int i = 0; i < w * h; i++) {
        pixels[i * 4 + 0] = config.backgroundColor.r;
        pixels[i * 4 + 1] = config.backgroundColor.g;
        pixels[i * 4 + 2] = config.backgroundColor.b;
        pixels[i * 4 + 3] = config.backgroundColor.a;
    }

    if (doc.root()) {
        // Compute viewBox transform
        SVGTransform viewTransform = SVGTransform::identity();
        Rect vb = doc.viewBox();
        if (vb.width > 0 && vb.height > 0) {
            float scaleX = w / vb.width;
            float scaleY = h / vb.height;
            float scale = std::min(scaleX, scaleY); // Meet
            float tx = (w - vb.width * scale) / 2 - vb.x * scale;
            float ty = (h - vb.height * scale) / 2 - vb.y * scale;
            viewTransform = SVGTransform::translate(tx, ty) * SVGTransform::scale(scale, scale);
        }

        renderNode(*doc.root(), pixels.data(), w, h, stride, viewTransform);
    }

    return pixels;
}

void SVGRenderer::renderInto(const SVGDocument& doc, uint8_t* pixels, int width, int height,
                               int stride) {
    if (!doc.root()) return;
    renderNode(*doc.root(), pixels, width, height, stride, SVGTransform::identity());
}

void SVGRenderer::renderNode(const SVGNode& node, uint8_t* pixels, int width, int height,
                                int stride, const SVGTransform& parentTransform) {
    if (!node.style.visible || node.style.display == SVGStyle::Display::None) return;

    SVGTransform currentTransform = parentTransform * node.transform;

    // Render this node
    switch (node.type) {
        case SVGNode::Type::Path:
        case SVGNode::Type::Rect:
        case SVGNode::Type::Circle:
        case SVGNode::Type::Ellipse:
        case SVGNode::Type::Line:
        case SVGNode::Type::Polyline:
        case SVGNode::Type::Polygon:
            renderShape(node, pixels, width, height, stride, currentTransform);
            break;
        case SVGNode::Type::Text:
        case SVGNode::Type::TSpan:
            renderText(node, pixels, width, height, stride, currentTransform);
            break;
        case SVGNode::Type::Image:
            renderImage(node, pixels, width, height, stride, currentTransform);
            break;
        default:
            break;
    }

    // Render children
    for (const auto& child : node.children) {
        renderNode(*child, pixels, width, height, stride, currentTransform);
    }
}

void SVGRenderer::renderShape(const SVGNode& node, uint8_t* pixels, int w, int h, int stride,
                                 const SVGTransform& transform) {
    // Convert shape to path
    PathGen::Path shapePath;

    switch (node.type) {
        case SVGNode::Type::Path:
            shapePath = node.path;
            break;
        case SVGNode::Type::Rect: {
            float rx = std::min(node.rx, node.width / 2);
            float ry = std::min(node.ry, node.height / 2);
            if (rx <= 0 && ry <= 0) {
                shapePath.moveTo(node.x, node.y);
                shapePath.lineTo(node.x + node.width, node.y);
                shapePath.lineTo(node.x + node.width, node.y + node.height);
                shapePath.lineTo(node.x, node.y + node.height);
                shapePath.close();
            } else {
                // Rounded rect
                if (ry <= 0) ry = rx;
                if (rx <= 0) rx = ry;
                shapePath.moveTo(node.x + rx, node.y);
                shapePath.lineTo(node.x + node.width - rx, node.y);
                shapePath.arcTo(rx, ry, 0, false, true, node.x + node.width, node.y + ry);
                shapePath.lineTo(node.x + node.width, node.y + node.height - ry);
                shapePath.arcTo(rx, ry, 0, false, true, node.x + node.width - rx, node.y + node.height);
                shapePath.lineTo(node.x + rx, node.y + node.height);
                shapePath.arcTo(rx, ry, 0, false, true, node.x, node.y + node.height - ry);
                shapePath.lineTo(node.x, node.y + ry);
                shapePath.arcTo(rx, ry, 0, false, true, node.x + rx, node.y);
                shapePath.close();
            }
            break;
        }
        case SVGNode::Type::Circle: {
            // Approximate circle with 4 cubic beziers
            float k = 0.5522847498f;
            float cx = node.cx, cy = node.cy, r = node.r;
            shapePath.moveTo(cx + r, cy);
            shapePath.cubicTo(cx + r, cy + r * k, cx + r * k, cy + r, cx, cy + r);
            shapePath.cubicTo(cx - r * k, cy + r, cx - r, cy + r * k, cx - r, cy);
            shapePath.cubicTo(cx - r, cy - r * k, cx - r * k, cy - r, cx, cy - r);
            shapePath.cubicTo(cx + r * k, cy - r, cx + r, cy - r * k, cx + r, cy);
            shapePath.close();
            break;
        }
        case SVGNode::Type::Ellipse: {
            float k = 0.5522847498f;
            float cx = node.cx, cy = node.cy, erx = node.rx, ery = node.ry;
            shapePath.moveTo(cx + erx, cy);
            shapePath.cubicTo(cx + erx, cy + ery * k, cx + erx * k, cy + ery, cx, cy + ery);
            shapePath.cubicTo(cx - erx * k, cy + ery, cx - erx, cy + ery * k, cx - erx, cy);
            shapePath.cubicTo(cx - erx, cy - ery * k, cx - erx * k, cy - ery, cx, cy - ery);
            shapePath.cubicTo(cx + erx * k, cy - ery, cx + erx, cy - ery * k, cx + erx, cy);
            shapePath.close();
            break;
        }
        case SVGNode::Type::Line:
            shapePath.moveTo(node.x1, node.y1);
            shapePath.lineTo(node.x2, node.y2);
            break;
        case SVGNode::Type::Polyline:
        case SVGNode::Type::Polygon:
            if (!node.points.empty()) {
                shapePath.moveTo(node.points[0].x, node.points[0].y);
                for (size_t i = 1; i < node.points.size(); i++) {
                    shapePath.lineTo(node.points[i].x, node.points[i].y);
                }
                if (node.type == SVGNode::Type::Polygon) shapePath.close();
            }
            break;
        default:
            return;
    }

    rasterizePath(shapePath, pixels, w, h, stride, node.style, transform);
}

void SVGRenderer::renderText(const SVGNode& /*node*/, uint8_t* /*pixels*/, int /*w*/, int /*h*/,
                                int /*stride*/, const SVGTransform& /*transform*/) {
    // Text rendering requires font shaping — delegated to text layout engine
}

void SVGRenderer::renderImage(const SVGNode& /*node*/, uint8_t* /*pixels*/, int /*w*/, int /*h*/,
                                 int /*stride*/, const SVGTransform& /*transform*/) {
    // Image rendering — load href, decode, composite
}

void SVGRenderer::applyFilter(const SVGFilter& /*filter*/, uint8_t* /*pixels*/,
                                 int /*w*/, int /*h*/, int /*stride*/, const Rect& /*region*/) {
    // Filter application — Gaussian blur, color matrix, etc.
}

void SVGRenderer::rasterizePath(const PathGen::Path& path, uint8_t* pixels, int w, int h,
                                  int stride, const SVGStyle& style,
                                  const SVGTransform& transform) {
    if (path.isEmpty()) return;

    // Transform all points and scan-convert
    // Rasterization via edge-walking scanline fill
    const auto& verbs = path.verbs();
    const auto& points = path.points();

    // Collect transformed edges
    struct Edge {
        float x0, y0, x1, y1;
    };
    std::vector<Edge> edges;

    size_t ptIdx = 0;
    Point current = {0, 0};

    for (auto verb : verbs) {
        switch (verb) {
            case PathGen::PathVerb::MoveTo:
                current = transform.apply(points[ptIdx++]);
                break;
            case PathGen::PathVerb::LineTo: {
                Point next = transform.apply(points[ptIdx++]);
                edges.push_back({current.x, current.y, next.x, next.y});
                current = next;
                break;
            }
            case PathGen::PathVerb::QuadTo: {
                // Linearize quadratic bezier
                Point cp = transform.apply(points[ptIdx++]);
                Point end = transform.apply(points[ptIdx++]);
                for (int i = 0; i < 8; i++) {
                    float t0 = i / 8.0f, t1 = (i + 1) / 8.0f;
                    float x0 = (1-t0)*(1-t0)*current.x + 2*(1-t0)*t0*cp.x + t0*t0*end.x;
                    float y0 = (1-t0)*(1-t0)*current.y + 2*(1-t0)*t0*cp.y + t0*t0*end.y;
                    float x1 = (1-t1)*(1-t1)*current.x + 2*(1-t1)*t1*cp.x + t1*t1*end.x;
                    float y1 = (1-t1)*(1-t1)*current.y + 2*(1-t1)*t1*cp.y + t1*t1*end.y;
                    edges.push_back({x0, y0, x1, y1});
                }
                current = end;
                break;
            }
            case PathGen::PathVerb::CubicTo: {
                Point cp1 = transform.apply(points[ptIdx++]);
                Point cp2 = transform.apply(points[ptIdx++]);
                Point end = transform.apply(points[ptIdx++]);
                for (int i = 0; i < 16; i++) {
                    float t0 = i / 16.0f, t1 = (i + 1) / 16.0f;
                    auto cubic = [](float t, float p0, float p1, float p2, float p3) {
                        float u = 1 - t;
                        return u*u*u*p0 + 3*u*u*t*p1 + 3*u*t*t*p2 + t*t*t*p3;
                    };
                    float x0 = cubic(t0, current.x, cp1.x, cp2.x, end.x);
                    float y0 = cubic(t0, current.y, cp1.y, cp2.y, end.y);
                    float x1 = cubic(t1, current.x, cp1.x, cp2.x, end.x);
                    float y1 = cubic(t1, current.y, cp1.y, cp2.y, end.y);
                    edges.push_back({x0, y0, x1, y1});
                }
                current = end;
                break;
            }
            case PathGen::PathVerb::ArcTo:
                ptIdx++; // Arc points handled by path
                break;
            case PathGen::PathVerb::Close:
                break;
        }
    }

    // Scanline fill (non-zero winding rule)
    if (style.fill.type == SVGPaint::Type::Color) {
        Color fillColor = style.fill.color;
        uint8_t fr = fillColor.r, fg = fillColor.g, fb = fillColor.b;
        uint8_t fa = static_cast<uint8_t>(fillColor.a * style.fillOpacity * style.opacity);

        for (int y = 0; y < h; y++) {
            float scanY = y + 0.5f;

            // Find intersections
            std::vector<float> intersections;
            for (const auto& edge : edges) {
                if ((edge.y0 <= scanY && edge.y1 > scanY) ||
                    (edge.y1 <= scanY && edge.y0 > scanY)) {
                    float t = (scanY - edge.y0) / (edge.y1 - edge.y0);
                    float x = edge.x0 + t * (edge.x1 - edge.x0);
                    intersections.push_back(x);
                }
            }

            std::sort(intersections.begin(), intersections.end());

            // Fill between pairs
            for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
                int xStart = std::max(0, static_cast<int>(intersections[i]));
                int xEnd = std::min(w - 1, static_cast<int>(intersections[i + 1]));

                for (int x = xStart; x <= xEnd; x++) {
                    int idx = y * stride + x * 4;
                    // Alpha composite
                    float srcA = fa / 255.0f;
                    float dstA = pixels[idx + 3] / 255.0f;
                    float outA = srcA + dstA * (1 - srcA);

                    if (outA > 0) {
                        pixels[idx + 0] = static_cast<uint8_t>((fr * srcA + pixels[idx + 0] * dstA * (1 - srcA)) / outA);
                        pixels[idx + 1] = static_cast<uint8_t>((fg * srcA + pixels[idx + 1] * dstA * (1 - srcA)) / outA);
                        pixels[idx + 2] = static_cast<uint8_t>((fb * srcA + pixels[idx + 2] * dstA * (1 - srcA)) / outA);
                        pixels[idx + 3] = static_cast<uint8_t>(outA * 255);
                    }
                }
            }
        }
    }
}

} // namespace SVG
} // namespace NXRender
