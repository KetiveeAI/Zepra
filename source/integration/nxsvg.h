// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// NxSVG - Enhanced SVG Loader for Zepra Browser / NeolyxOS
// FIXES: Self-closing tags, style attribute parsing, proper SVG defaults

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <cmath>
#include <regex>
#include <iostream>
#include <algorithm>

#include "nxgfx/context.h"
#include "nxgfx/primitives.h"

namespace nxsvg {

using Color = NXRender::Color;
using Point = NXRender::Point;

    inline Color parseColor(const std::string& name) {
        if (name.empty() || name == "none" || name == "transparent") return Color(0, 0, 0, 0);
        
        // CSS named colors
        if (name == "black") return Color(0, 0, 0);
        if (name == "white") return Color(255, 255, 255);
        if (name == "red") return Color(255, 0, 0);
        if (name == "green") return Color(0, 128, 0);
        if (name == "blue") return Color(0, 0, 255);
        if (name == "yellow") return Color(255, 255, 0);
        if (name == "purple") return Color(128, 0, 128);
        if (name == "orange") return Color(255, 165, 0);
        if (name == "cyan" || name == "aqua") return Color(0, 255, 255);
        if (name == "magenta" || name == "fuchsia") return Color(255, 0, 255);
        if (name == "lime") return Color(0, 255, 0);
        if (name == "gray" || name == "grey") return Color(128, 128, 128);
        if (name == "silver") return Color(192, 192, 192);
        if (name == "maroon") return Color(128, 0, 0);
        if (name == "olive") return Color(128, 128, 0);
        if (name == "navy") return Color(0, 0, 128);
        if (name == "teal") return Color(0, 128, 128);
        if (name == "darkgray" || name == "darkgrey") return Color(169, 169, 169);
        if (name == "lightgray" || name == "lightgrey") return Color(211, 211, 211);
        if (name == "dimgray" || name == "dimgrey") return Color(105, 105, 105);
        if (name == "darkred") return Color(139, 0, 0);
        if (name == "darkgreen") return Color(0, 100, 0);
        if (name == "darkblue") return Color(0, 0, 139);
        if (name == "coral") return Color(255, 127, 80);
        if (name == "tomato") return Color(255, 99, 71);
        if (name == "gold") return Color(255, 215, 0);
        if (name == "skyblue") return Color(135, 206, 235);
        if (name == "steelblue") return Color(70, 130, 180);
        if (name == "indianred") return Color(205, 92, 92);
        if (name == "salmon") return Color(250, 128, 114);
        if (name == "pink") return Color(255, 192, 203);
        if (name == "brown") return Color(165, 42, 42);
        if (name == "tan") return Color(210, 180, 140);
        if (name == "wheat") return Color(245, 222, 179);
        if (name == "ivory") return Color(255, 255, 240);
        if (name == "lavender") return Color(230, 230, 250);
        if (name == "crimson") return Color(220, 20, 60);
        if (name == "indigo") return Color(75, 0, 130);
        if (name == "violet") return Color(238, 130, 238);
        if (name == "plum") return Color(221, 160, 221);
        if (name == "orchid") return Color(218, 112, 214);
        if (name == "turquoise") return Color(64, 224, 208);
        if (name == "slategray" || name == "slategrey") return Color(112, 128, 144);
        if (name == "currentColor" || name == "currentcolor") return Color(255, 255, 255, 255); // Sentinel for currentColor
        
        // Hex colors
        if (name[0] == '#') {
            std::string hex = name.substr(1);
            if (hex.length() == 3) {
                std::string r = hex.substr(0, 1) + hex.substr(0, 1);
                std::string g = hex.substr(1, 1) + hex.substr(1, 1);
                std::string b = hex.substr(2, 1) + hex.substr(2, 1);
                return Color(std::stoi(r, nullptr, 16), std::stoi(g, nullptr, 16), std::stoi(b, nullptr, 16));
            } else if (hex.length() == 6) {
                return Color(std::stoi(hex.substr(0, 2), nullptr, 16),
                             std::stoi(hex.substr(2, 2), nullptr, 16),
                             std::stoi(hex.substr(4, 2), nullptr, 16));
            } else if (hex.length() == 8) {
                return Color(std::stoi(hex.substr(0, 2), nullptr, 16),
                             std::stoi(hex.substr(2, 2), nullptr, 16),
                             std::stoi(hex.substr(4, 2), nullptr, 16),
                             std::stoi(hex.substr(6, 2), nullptr, 16));
            }
        }
        
        // rgb(r, g, b) and rgba(r, g, b, a)
        if (name.substr(0, 4) == "rgb(" || name.substr(0, 5) == "rgba(") {
            std::regex rgbRegex(R"(rgba?\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*(?:,\s*([0-9.]+)\s*)?\))");
            std::smatch m;
            if (std::regex_search(name, m, rgbRegex)) {
                int r = std::stoi(m[1].str());
                int g = std::stoi(m[2].str());
                int b = std::stoi(m[3].str());
                int a = 255;
                if (m[4].matched) {
                    float af = std::stof(m[4].str());
                    a = (int)(af <= 1.0f ? af * 255 : af);
                }
                return Color(r, g, b, a);
            }
        }
        
        return Color(0, 0, 0);
    }

struct PathCommand {
    char type = 'M';
    std::vector<float> args;
};

struct Shape {
    enum Type { PATH, CIRCLE, RECT, LINE, ELLIPSE, POLYLINE, POLYGON };
    Type type = PATH;
    std::vector<PathCommand> path;
    std::vector<std::pair<float, float>> points; // For POLYLINE/POLYGON
    float cx = 0, cy = 0, r = 0, rx = 0, ry = 0;
    float x = 0, y = 0, w = 0, h = 0;
    float rectRx = 0, rectRy = 0; // Rounded rect corner radii
    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    Color stroke = Color::black(), fill = Color::black();
    float strokeWidth = 1.5f;
    float opacity = 1.0f;
    float fillOpacity = 1.0f;
    float strokeOpacity = 1.0f;
    std::string fillRule = "nonzero"; // nonzero or evenodd
    bool hasFill = false;
    bool hasStroke = false;
};

// SVG Group - represents <g> elements with metadata
struct SvgGroup {
    std::string id;              // id attribute
    std::string name;            // data-name or inkscape:label
    std::string className;       // class attribute
    float opacity = 1.0f;
    bool visible = true;
    
    // Transform matrix (2D affine: a,b,c,d,e,f)
    float transform[6] = {1, 0, 0, 1, 0, 0};  // Identity
    bool hasTransform = false;
    
    // Shapes directly in this group
    std::vector<Shape> shapes;
    
    // Nested groups (children)
    std::vector<SvgGroup> children;
    
    // Bounding box (computed on parse)
    float minX = 0, minY = 0, maxX = 0, maxY = 0;
    bool boundsComputed = false;
};

class SvgImage {
public:
    float width = 24, height = 24;
    float viewBoxX = 0, viewBoxY = 0, viewBoxW = 24, viewBoxH = 24;
    std::vector<Shape> shapes;          // Top-level shapes (not in groups)
    std::vector<SvgGroup> groups;       // Top-level groups
    void render(float x, float y, float size, uint8_t cr = 255, uint8_t cg = 255, uint8_t cb = 255) {
        float scale = size / std::max(viewBoxW, viewBoxH);
        float ox = x - viewBoxX * scale;
        float oy = y - viewBoxY * scale;
        
        auto* gpu = NXRender::gpu();
        if (!gpu) return;

        // Note: NXRender handles blending by default or per-call
        
        for (const Shape& s : shapes) {
            Color stroke = s.stroke;
            Color fill = s.fill;
            
            // Apply opacity
            float finalOpacity = s.opacity;
            // Handle CurrentColor (simple replacement)
            // Note: NXRender::Color doesn't explicitly flag CurrentColor, check raw values or assumptions if needed.
            // But previous implementation checked r,g,b,a == 255.
            if (stroke.r == 255 && stroke.g == 255 && stroke.b == 255 && stroke.a == 255) { stroke.r = cr; stroke.g = cg; stroke.b = cb; }
            if (fill.r == 255 && fill.g == 255 && fill.b == 255 && fill.a == 255) { fill.r = cr; fill.g = cg; fill.b = cb; }

            stroke.a = (uint8_t)(stroke.a * finalOpacity * s.strokeOpacity);
            fill.a = (uint8_t)(fill.a * finalOpacity * s.fillOpacity);
            
            switch (s.type) {
                case Shape::CIRCLE:
                    renderCircle(ox + s.cx * scale, oy + s.cy * scale, s.r * scale, 
                                 fill, stroke, s.strokeWidth * scale, s.hasFill, s.hasStroke);
                    break;
                case Shape::RECT:
                    renderRect(ox + s.x * scale, oy + s.y * scale, s.w * scale, s.h * scale, 
                               fill, stroke, s.strokeWidth * scale, s.hasFill, s.hasStroke, s.rectRx * scale, s.rectRy * scale);
                    break;
                case Shape::LINE:
                    renderLine(ox + s.x1 * scale, oy + s.y1 * scale, 
                              ox + s.x2 * scale, oy + s.y2 * scale, stroke, s.strokeWidth * scale);
                    break;
                case Shape::PATH:
                    renderPath(s.path, ox, oy, scale, fill, stroke, s.strokeWidth * scale, s.hasFill, s.hasStroke, s.fillRule);
                    break;
                case Shape::ELLIPSE:
                    renderEllipse(ox + s.cx * scale, oy + s.cy * scale, 
                                  s.rx * scale, s.ry * scale, fill, stroke, s.strokeWidth * scale, s.hasFill, s.hasStroke);
                    break;
                case Shape::POLYLINE:
                case Shape::POLYGON:
                    renderPoly(s.points, ox, oy, scale, fill, stroke, s.strokeWidth * scale, s.hasFill, s.hasStroke, s.type == Shape::POLYGON);
                    break;
            }
        }
        
        // Also render groups
        for (const SvgGroup& g : groups) {
            renderGroupInternal(g, ox, oy, scale, cr, cg, cb, 1.0f);
        }
    }
    
    // Render a specific group by ID
    void renderGroup(const std::string& id, float x, float y, float size, 
                     uint8_t cr = 255, uint8_t cg = 255, uint8_t cb = 255) {
        float scale = size / std::max(viewBoxW, viewBoxH);
        float ox = x - viewBoxX * scale;
        float oy = y - viewBoxY * scale;
        
        const SvgGroup* g = getGroup(id);
        if (g) {
            renderGroupInternal(*g, ox, oy, scale, cr, cg, cb, 1.0f);
        }
    }
    
    // Get group by ID (for layer panel)
    const SvgGroup* getGroup(const std::string& id) const {
        for (const SvgGroup& g : groups) {
            if (g.id == id) return &g;
            const SvgGroup* found = findGroupRecursive(g.children, id);
            if (found) return found;
        }
        return nullptr;
    }
    
    // Get all top-level groups
    const std::vector<SvgGroup>& getGroups() const { return groups; }
    
    // Get total group count (including nested)
    size_t groupCount() const {
        size_t count = 0;
        for (const SvgGroup& g : groups) {
            count += 1 + countGroupsRecursive(g.children);
        }
        return count;
    }

private:
    // Recursive group finder
    const SvgGroup* findGroupRecursive(const std::vector<SvgGroup>& gs, const std::string& id) const {
        for (const SvgGroup& g : gs) {
            if (g.id == id) return &g;
            const SvgGroup* found = findGroupRecursive(g.children, id);
            if (found) return found;
        }
        return nullptr;
    }
    
    size_t countGroupsRecursive(const std::vector<SvgGroup>& gs) const {
        size_t count = gs.size();
        for (const SvgGroup& g : gs) {
            count += countGroupsRecursive(g.children);
        }
        return count;
    }
    
    // Internal group rendering with opacity inheritance
    void renderGroupInternal(const SvgGroup& g, float ox, float oy, float scale,
                             uint8_t cr, uint8_t cg, uint8_t cb, float parentOpacity) {
        if (!g.visible) return;
        
        float groupOpacity = parentOpacity * g.opacity;
        auto* gpu = NXRender::gpu();
        if (!gpu) return;
        
        // Apply group transform if present
        if (g.hasTransform) {
            gpu->pushTransform();
            // Apply 2D affine transform: a b c d e f
            // matrix(a, b, c, d, e, f) = translate(e, f) * matrix(a, b, c, d, 0, 0)
            gpu->translate(g.transform[4] * scale, g.transform[5] * scale);
            gpu->scale(g.transform[0], g.transform[3]);
        }
        
        // Render shapes in this group
        for (const Shape& s : g.shapes) {
            renderShapeWithOpacity(s, ox, oy, scale, cr, cg, cb, groupOpacity);
        }
        
        // Render child groups
        for (const SvgGroup& child : g.children) {
            renderGroupInternal(child, ox, oy, scale, cr, cg, cb, groupOpacity);
        }
        
        if (g.hasTransform) {
            gpu->popTransform();
        }
    }
    
    // Render single shape with opacity override
    void renderShapeWithOpacity(const Shape& s, float ox, float oy, float scale,
                                uint8_t cr, uint8_t cg, uint8_t cb, float groupOpacity) {
        Color stroke = s.stroke;
        Color fill = s.fill;
        
        float finalOpacity = s.opacity * groupOpacity;
        if (stroke.r == 255 && stroke.g == 255 && stroke.b == 255 && stroke.a == 255) {
            stroke.r = cr; stroke.g = cg; stroke.b = cb;
        }
        if (fill.r == 255 && fill.g == 255 && fill.b == 255 && fill.a == 255) {
            fill.r = cr; fill.g = cg; fill.b = cb;
        }
        
        stroke.a = (uint8_t)(stroke.a * finalOpacity * s.strokeOpacity);
        fill.a = (uint8_t)(fill.a * finalOpacity * s.fillOpacity);
        
        switch (s.type) {
            case Shape::CIRCLE:
                renderCircle(ox + s.cx * scale, oy + s.cy * scale, s.r * scale,
                             fill, stroke, s.strokeWidth * scale, s.hasFill, s.hasStroke);
                break;
            case Shape::RECT:
                renderRect(ox + s.x * scale, oy + s.y * scale, s.w * scale, s.h * scale,
                           fill, stroke, s.strokeWidth * scale, s.hasFill, s.hasStroke, s.rectRx * scale, s.rectRy * scale);
                break;
            case Shape::LINE:
                renderLine(ox + s.x1 * scale, oy + s.y1 * scale,
                          ox + s.x2 * scale, oy + s.y2 * scale, stroke, s.strokeWidth * scale);
                break;
            case Shape::PATH:
                renderPath(s.path, ox, oy, scale, fill, stroke, s.strokeWidth * scale,
                          s.hasFill, s.hasStroke, s.fillRule);
                break;
            case Shape::ELLIPSE:
                renderEllipse(ox + s.cx * scale, oy + s.cy * scale,
                              s.rx * scale, s.ry * scale, fill, stroke, s.strokeWidth * scale,
                              s.hasFill, s.hasStroke);
                break;
            case Shape::POLYLINE:
            case Shape::POLYGON:
                renderPoly(s.points, ox, oy, scale, fill, stroke, s.strokeWidth * scale,
                          s.hasFill, s.hasStroke, s.type == Shape::POLYGON);
                break;
        }
    }

    void renderCircle(float cx, float cy, float r, Color fill, Color stroke, float sw, bool hasFill, bool hasStroke) {
        if (r <= 0) return;
        auto* gpu = NXRender::gpu();
        if (hasFill && fill.a > 0) {
            gpu->fillCircle(cx, cy, r, fill);
        }
        if (hasStroke && stroke.a > 0) {
            gpu->strokeCircle(cx, cy, r, stroke, sw);
        }
    }

    void renderEllipse(float cx, float cy, float rx, float ry, Color fill, Color stroke, float sw, bool hasFill, bool hasStroke) {
        if (rx <= 0 || ry <= 0) return;
        // NXRender doesn't have specialized ellipse yet, we can approximate with scaling or adding it.
        // For now, let's use GPU context transform stack if possible, OR implement simple approximation here.
        // Implementing simple approximation since GpuContext might not expose full transform logic easily here without push/pop overhead.
        
        auto* gpu = NXRender::gpu();
        gpu->pushTransform();
        gpu->translate(cx, cy);
        gpu->scale(1.0f, ry/rx); // Scale Y to match aspect ratio
        if (hasFill && fill.a > 0) {
            gpu->fillCircle(0, 0, rx, fill);
        }
         if (hasStroke && stroke.a > 0) {
            // Stroke width will be distorted by scale if we aren't careful.
            // For correct stroke we might need actual ellipse points.
            // Let's stick to simple circle scaling for now as it's "more easy" and likely sufficient for icons.
            gpu->strokeCircle(0, 0, rx, stroke, sw); // stroke width might be distorted on Y axis
        }
        gpu->popTransform();
    }

    void renderRect(float x, float y, float w, float h, Color fill, Color stroke, float sw, bool hasFill, bool hasStroke, float rx = 0, float ry = 0) {
        if (w <= 0 || h <= 0) return;
        auto* gpu = NXRender::gpu();
        NXRender::Rect r(x, y, w, h);
        
        float cornerR = std::max(rx, ry);
        if (cornerR > 0) {
            // Clamp corner radius to half the smallest dimension
            cornerR = std::min(cornerR, std::min(w, h) / 2.0f);
            if (hasFill && fill.a > 0) {
                gpu->fillRoundedRect(r, fill, cornerR);
            }
            if (hasStroke && stroke.a > 0) {
                gpu->strokeRoundedRect(r, stroke, cornerR, sw);
            }
        } else {
            if (hasFill && fill.a > 0) {
                gpu->fillRect(r, fill);
            }
            if (hasStroke && stroke.a > 0) {
                gpu->strokeRect(r, stroke, sw);
            }
        }
    }

    void renderLine(float x1, float y1, float x2, float y2, Color stroke, float sw) {
        if (stroke.a > 0) {
            NXRender::gpu()->drawLine(x1, y1, x2, y2, stroke, sw);
        }
    }

    void renderPoly(const std::vector<std::pair<float, float>>& points, float ox, float oy, float scale,
                    Color fill, Color stroke, float sw, bool hasFill, bool hasStroke, bool closed) {
        if (points.size() < 2) return;
        
        std::vector<Point> nxPoints;
        nxPoints.reserve(points.size());
        for(const auto& p : points) nxPoints.emplace_back(ox + p.first * scale, oy + p.second * scale);

        auto* gpu = NXRender::gpu();

        if (closed && hasFill && fill.a > 0) {
            gpu->fillPath(nxPoints, fill);
        }

        if (hasStroke && stroke.a > 0) {
            gpu->strokePath(nxPoints, stroke, sw, closed);
        }
    }

    void renderPath(const std::vector<PathCommand>& cmds, float ox, float oy, float scale, 
                    Color fill, Color stroke, float sw, bool hasFill, bool hasStroke, const std::string& fillRule) {
        if (cmds.empty()) return;
        
        std::vector<std::vector<Point>> contours;
        std::vector<Point> currentContour;
        
        float cx = 0, cy = 0;
        float startX = 0, startY = 0;
        float lastCpX = 0, lastCpY = 0; // Last control point for smooth curves
        bool closedSubpath = false;
        
        auto flushContour = [&]() {
            if (!currentContour.empty()) {
                contours.push_back(currentContour);
                currentContour.clear();
                closedSubpath = false;
            }
        };

        const int BEZIER_STEPS = 16; // Higher resolution for sharp icons

        for (const auto& cmd : cmds) {
            size_t argCount = cmd.args.size();
            
            switch (cmd.type) {
                case 'M': // Move To
                    flushContour();
                    if (argCount >= 2) {
                        cx = cmd.args[0]; cy = cmd.args[1];
                        startX = cx; startY = cy;
                        lastCpX = cx; lastCpY = cy;
                        currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                        for (size_t i = 2; i + 1 < argCount; i += 2) {
                            cx = cmd.args[i]; cy = cmd.args[i + 1];
                            currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                        }
                    }
                    break;
                case 'm': // Relative Move To
                    flushContour();
                    if (argCount >= 2) {
                        cx += cmd.args[0]; cy += cmd.args[1];
                        startX = cx; startY = cy;
                        lastCpX = cx; lastCpY = cy;
                        currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                        for (size_t i = 2; i + 1 < argCount; i += 2) {
                            cx += cmd.args[i]; cy += cmd.args[i + 1];
                            currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                        }
                    }
                    break;
                case 'L': // Line To
                    for (size_t i = 0; i + 1 < argCount; i += 2) {
                        cx = cmd.args[i]; cy = cmd.args[i + 1];
                        currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                    }
                    lastCpX = cx; lastCpY = cy;
                    break;
                case 'l': // Relative Line To
                    for (size_t i = 0; i + 1 < argCount; i += 2) {
                        cx += cmd.args[i]; cy += cmd.args[i + 1];
                        currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                    }
                    lastCpX = cx; lastCpY = cy;
                    break;
                case 'H': // Horizontal Line To
                    for (size_t i = 0; i < argCount; i++) {
                        cx = cmd.args[i];
                        currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                    }
                    lastCpX = cx; lastCpY = cy;
                    break;
                case 'h': // Relative Horizontal Line To
                    for (size_t i = 0; i < argCount; i++) {
                        cx += cmd.args[i];
                        currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                    }
                    lastCpX = cx; lastCpY = cy;
                    break;
                case 'V': // Vertical Line To
                    for (size_t i = 0; i < argCount; i++) {
                        cy = cmd.args[i];
                        currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                    }
                    lastCpX = cx; lastCpY = cy;
                    break;
                case 'v': // Relative Vertical Line To
                    for (size_t i = 0; i < argCount; i++) {
                        cy += cmd.args[i];
                        currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                    }
                    lastCpX = cx; lastCpY = cy;
                    break;
                case 'Z': case 'z': // Close Path
                    closedSubpath = true;
                    cx = startX; cy = startY;
                    lastCpX = cx; lastCpY = cy;
                    if (!currentContour.empty()) {
                        auto& last = currentContour.back();
                        if (last.x != ox + cx * scale || last.y != oy + cy * scale) {
                            currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                        }
                    }
                    break;
                case 'C': // Cubic Bezier
                    for (size_t i = 0; i + 5 < argCount; i += 6) {
                        float x1 = cmd.args[i], y1 = cmd.args[i+1];
                        float x2 = cmd.args[i+2], y2 = cmd.args[i+3];
                        float x3 = cmd.args[i+4], y3 = cmd.args[i+5];
                        for (int t = 1; t <= BEZIER_STEPS; t++) {
                            float u = t / (float)BEZIER_STEPS, u2 = u * u, u3 = u2 * u;
                            float m = 1 - u, m2 = m * m, m3 = m2 * m;
                            float px = m3 * cx + 3 * m2 * u * x1 + 3 * m * u2 * x2 + u3 * x3;
                            float py = m3 * cy + 3 * m2 * u * y1 + 3 * m * u2 * y2 + u3 * y3;
                            currentContour.emplace_back(ox + px * scale, oy + py * scale);
                        }
                        lastCpX = x2; lastCpY = y2;
                        cx = x3; cy = y3;
                    }
                    break;
                case 'c': // Relative Cubic Bezier
                    for (size_t i = 0; i + 5 < argCount; i += 6) {
                        float x1 = cx + cmd.args[i], y1 = cy + cmd.args[i+1];
                        float x2 = cx + cmd.args[i+2], y2 = cy + cmd.args[i+3];
                        float x3 = cx + cmd.args[i+4], y3 = cy + cmd.args[i+5];
                        for (int t = 1; t <= BEZIER_STEPS; t++) {
                            float u = t / (float)BEZIER_STEPS, u2 = u * u, u3 = u2 * u;
                            float m = 1 - u, m2 = m * m, m3 = m2 * m;
                            float px = m3 * cx + 3 * m2 * u * x1 + 3 * m * u2 * x2 + u3 * x3;
                            float py = m3 * cy + 3 * m2 * u * y1 + 3 * m * u2 * y2 + u3 * y3;
                            currentContour.emplace_back(ox + px * scale, oy + py * scale);
                        }
                        lastCpX = x2; lastCpY = y2;
                        cx = x3; cy = y3;
                    }
                    break;
                case 'S': // Smooth Cubic Bezier
                    for (size_t i = 0; i + 3 < argCount; i += 4) {
                        // Reflect last control point
                        float x1 = 2 * cx - lastCpX, y1 = 2 * cy - lastCpY;
                        float x2 = cmd.args[i], y2 = cmd.args[i+1];
                        float x3 = cmd.args[i+2], y3 = cmd.args[i+3];
                        for (int t = 1; t <= BEZIER_STEPS; t++) {
                            float u = t / (float)BEZIER_STEPS, u2 = u * u, u3 = u2 * u;
                            float m = 1 - u, m2 = m * m, m3 = m2 * m;
                            float px = m3 * cx + 3 * m2 * u * x1 + 3 * m * u2 * x2 + u3 * x3;
                            float py = m3 * cy + 3 * m2 * u * y1 + 3 * m * u2 * y2 + u3 * y3;
                            currentContour.emplace_back(ox + px * scale, oy + py * scale);
                        }
                        lastCpX = x2; lastCpY = y2;
                        cx = x3; cy = y3;
                    }
                    break;
                case 's': // Relative Smooth Cubic Bezier
                    for (size_t i = 0; i + 3 < argCount; i += 4) {
                        float x1 = 2 * cx - lastCpX, y1 = 2 * cy - lastCpY;
                        float x2 = cx + cmd.args[i], y2 = cy + cmd.args[i+1];
                        float x3 = cx + cmd.args[i+2], y3 = cy + cmd.args[i+3];
                        for (int t = 1; t <= BEZIER_STEPS; t++) {
                            float u = t / (float)BEZIER_STEPS, u2 = u * u, u3 = u2 * u;
                            float m = 1 - u, m2 = m * m, m3 = m2 * m;
                            float px = m3 * cx + 3 * m2 * u * x1 + 3 * m * u2 * x2 + u3 * x3;
                            float py = m3 * cy + 3 * m2 * u * y1 + 3 * m * u2 * y2 + u3 * y3;
                            currentContour.emplace_back(ox + px * scale, oy + py * scale);
                        }
                        lastCpX = x2; lastCpY = y2;
                        cx = x3; cy = y3;
                    }
                    break;
                case 'Q': // Quadratic Bezier
                    for (size_t i = 0; i + 3 < argCount; i += 4) {
                        float x1 = cmd.args[i], y1 = cmd.args[i+1];
                        float x2 = cmd.args[i+2], y2 = cmd.args[i+3];
                        for (int t = 1; t <= BEZIER_STEPS; t++) {
                            float u = t / (float)BEZIER_STEPS;
                            float m = 1 - u;
                            float px = m * m * cx + 2 * m * u * x1 + u * u * x2;
                            float py = m * m * cy + 2 * m * u * y1 + u * u * y2;
                            currentContour.emplace_back(ox + px * scale, oy + py * scale);
                        }
                        lastCpX = x1; lastCpY = y1;
                        cx = x2; cy = y2;
                    }
                    break;
                case 'q': // Relative Quadratic Bezier
                    for (size_t i = 0; i + 3 < argCount; i += 4) {
                        float x1 = cx + cmd.args[i], y1 = cy + cmd.args[i+1];
                        float x2 = cx + cmd.args[i+2], y2 = cy + cmd.args[i+3];
                        for (int t = 1; t <= BEZIER_STEPS; t++) {
                            float u = t / (float)BEZIER_STEPS;
                            float m = 1 - u;
                            float px = m * m * cx + 2 * m * u * x1 + u * u * x2;
                            float py = m * m * cy + 2 * m * u * y1 + u * u * y2;
                            currentContour.emplace_back(ox + px * scale, oy + py * scale);
                        }
                        lastCpX = x1; lastCpY = y1;
                        cx = x2; cy = y2;
                    }
                    break;
                case 'T': // Smooth Quadratic Bezier
                    for (size_t i = 0; i + 1 < argCount; i += 2) {
                        float x1 = 2 * cx - lastCpX, y1 = 2 * cy - lastCpY;
                        float x2 = cmd.args[i], y2 = cmd.args[i+1];
                        for (int t = 1; t <= BEZIER_STEPS; t++) {
                            float u = t / (float)BEZIER_STEPS;
                            float m = 1 - u;
                            float px = m * m * cx + 2 * m * u * x1 + u * u * x2;
                            float py = m * m * cy + 2 * m * u * y1 + u * u * y2;
                            currentContour.emplace_back(ox + px * scale, oy + py * scale);
                        }
                        lastCpX = x1; lastCpY = y1;
                        cx = x2; cy = y2;
                    }
                    break;
                case 't': // Relative Smooth Quadratic Bezier
                    for (size_t i = 0; i + 1 < argCount; i += 2) {
                        float x1 = 2 * cx - lastCpX, y1 = 2 * cy - lastCpY;
                        float x2 = cx + cmd.args[i], y2 = cy + cmd.args[i+1];
                        for (int t = 1; t <= BEZIER_STEPS; t++) {
                            float u = t / (float)BEZIER_STEPS;
                            float m = 1 - u;
                            float px = m * m * cx + 2 * m * u * x1 + u * u * x2;
                            float py = m * m * cy + 2 * m * u * y1 + u * u * y2;
                            currentContour.emplace_back(ox + px * scale, oy + py * scale);
                        }
                        lastCpX = x1; lastCpY = y1;
                        cx = x2; cy = y2;
                    }
                    break;
                case 'A': case 'a': { // Elliptical Arc
                    bool relative = (cmd.type == 'a');
                    for (size_t i = 0; i + 6 < argCount; i += 7) {
                        float rx = std::abs(cmd.args[i]);
                        float ry = std::abs(cmd.args[i+1]);
                        float xrot = cmd.args[i+2] * M_PI / 180.0f;
                        int largeArc = (int)cmd.args[i+3];
                        int sweep = (int)cmd.args[i+4];
                        float ex = cmd.args[i+5], ey = cmd.args[i+6];
                        if (relative) { ex += cx; ey += cy; }
                        
                        if (rx < 0.001f || ry < 0.001f) {
                            // Degenerate arc → line
                            cx = ex; cy = ey;
                            currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                            continue;
                        }
                        
                        // SVG endpoint to center parameterization
                        float cosRot = std::cos(xrot), sinRot = std::sin(xrot);
                        float dx2 = (cx - ex) / 2.0f, dy2 = (cy - ey) / 2.0f;
                        float x1p = cosRot * dx2 + sinRot * dy2;
                        float y1p = -sinRot * dx2 + cosRot * dy2;
                        
                        float rx2 = rx * rx, ry2 = ry * ry;
                        float x1p2 = x1p * x1p, y1p2 = y1p * y1p;
                        
                        // Scale radii if needed
                        float lambda = x1p2 / rx2 + y1p2 / ry2;
                        if (lambda > 1.0f) {
                            float sq = std::sqrt(lambda);
                            rx *= sq; ry *= sq;
                            rx2 = rx * rx; ry2 = ry * ry;
                        }
                        
                        float num = rx2 * ry2 - rx2 * y1p2 - ry2 * x1p2;
                        float den = rx2 * y1p2 + ry2 * x1p2;
                        float sq = (den > 0) ? std::sqrt(std::max(0.0f, num / den)) : 0;
                        if (largeArc == sweep) sq = -sq;
                        
                        float cxp = sq * rx * y1p / ry;
                        float cyp = sq * -ry * x1p / rx;
                        
                        float centerX = cosRot * cxp - sinRot * cyp + (cx + ex) / 2.0f;
                        float centerY = sinRot * cxp + cosRot * cyp + (cy + ey) / 2.0f;
                        
                        float theta1 = std::atan2((y1p - cyp) / ry, (x1p - cxp) / rx);
                        float dtheta = std::atan2((-y1p - cyp) / ry, (-x1p - cxp) / rx) - theta1;
                        
                        if (sweep && dtheta < 0) dtheta += 2 * M_PI;
                        if (!sweep && dtheta > 0) dtheta -= 2 * M_PI;
                        
                        int steps = std::max(8, (int)(std::abs(dtheta) / (M_PI / 8)));
                        for (int t = 1; t <= steps; t++) {
                            float angle = theta1 + dtheta * t / steps;
                            float px = std::cos(angle) * rx;
                            float py = std::sin(angle) * ry;
                            float rx2_ = cosRot * px - sinRot * py + centerX;
                            float ry2_ = sinRot * px + cosRot * py + centerY;
                            currentContour.emplace_back(ox + rx2_ * scale, oy + ry2_ * scale);
                        }
                        
                        lastCpX = cx; lastCpY = cy;
                        cx = ex; cy = ey;
                    }
                    break;
                }
            }
        }
        flushContour();

        if (contours.empty()) return;

        auto* gpu = NXRender::gpu();
        
        // --- RENDER FILL (Complex/Stencil) ---
        if (hasFill && fill.a > 0) {
            gpu->fillComplexPath(contours, fill, fillRule);
        }

        // --- RENDER STROKE ---
        if (hasStroke && stroke.a > 0) {
            for (const auto& contour : contours) {
                 // For stroke, we treat each contour individually
                 // Note: 'closed' flag here is tricky if Z wasn't the last command, but we closed it in points.
                 // Ideally pass 'closed' flag from parsing logic but for now simple loop check or explicit flag
                 bool closed = (contour.size() > 1 && contour.front().distance(contour.back()) < 0.001f);
                 gpu->strokePath(contour, stroke, sw, closed);
            }
        }
    }
};

class SvgLoader {
public:
    bool loadFromFile(const std::string& name, const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "[NxSVG] Failed to open: " << path << "\n";
            return false;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        bool ok = loadFromString(name, buffer.str());
        if (ok) {
            std::cout << "[NxSVG] Loaded: " << name << " (" << images_[name].shapes.size() << " shapes)\n";
        }
        return ok;
    }

    bool loadFromString(const std::string& name, const std::string& svg) {
        SvgImage img;
        parseViewBox(svg, img);
        parseGroups(svg, img);  // Parse groups first (with their shapes)
        parsePaths(svg, img);   // Then top-level shapes
        parseCircles(svg, img);
        parseRects(svg, img);
        parseEllipses(svg, img);
        parseLines(svg, img);
        parsePolylines(svg, img);
        parsePolygons(svg, img);
        images_[name] = img;
        return true;
    }

    void draw(const std::string& name, float x, float y, float size, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255) {
        auto it = images_.find(name);
        if (it != images_.end()) {
            it->second.render(x, y, size, r, g, b);
        }
    }
    
    // Draw specific group from image
    void drawGroup(const std::string& name, const std::string& groupId, 
                   float x, float y, float size, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255) {
        auto it = images_.find(name);
        if (it != images_.end()) {
            it->second.renderGroup(groupId, x, y, size, r, g, b);
        }
    }

    bool has(const std::string& name) const {
        return images_.find(name) != images_.end();
    }
    
    // Get mutable image pointer (for C wrapper)
    SvgImage* getImage(const std::string& name) {
        auto it = images_.find(name);
        if (it != images_.end()) {
            return &it->second;
        }
        return nullptr;
    }
    
    // Get const image pointer
    const SvgImage* getImage(const std::string& name) const {
        auto it = images_.find(name);
        if (it != images_.end()) {
            return &it->second;
        }
        return nullptr;
    }

private:
    std::unordered_map<std::string, SvgImage> images_;

    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }

    void parseViewBox(const std::string& svg, SvgImage& img) {
        std::regex vbRegex(R"(viewBox\s*=\s*\"([^\"]+)\")");
        std::smatch match;
        if (std::regex_search(svg, match, vbRegex)) {
            std::istringstream ss(match[1].str());
            ss >> img.viewBoxX >> img.viewBoxY >> img.viewBoxW >> img.viewBoxH;
        }
        
        std::regex wRegex(R"(width\s*=\s*\"([0-9.]+)\")");
        if (std::regex_search(svg, match, wRegex)) {
            try { img.width = std::stof(match[1].str()); } catch (...) {}
        }
        std::regex hRegex(R"(height\s*=\s*\"([0-9.]+)\")");
        if (std::regex_search(svg, match, hRegex)) {
            try { img.height = std::stof(match[1].str()); } catch (...) {}
        }
    }

    // Parse SVG groups with metadata
    void parseGroups(const std::string& svg, SvgImage& img) {
        // Find all <g> elements with attributes
        std::regex groupStartRegex(R"(<g\s+([^>]*)>)");
        std::regex groupEndRegex(R"(</g>)");
        
        auto begin = std::sregex_iterator(svg.begin(), svg.end(), groupStartRegex);
        auto end = std::sregex_iterator();
        
        for (auto it = begin; it != end; ++it) {
            SvgGroup group;
            std::string attrs = (*it)[1].str();
            
            // Parse group attributes
            group.id = parseStrAttr("<g " + attrs + ">", "id", "");
            group.name = parseStrAttr("<g " + attrs + ">", "inkscape:label", "");
            if (group.name.empty()) {
                group.name = parseStrAttr("<g " + attrs + ">", "data-name", "");
            }
            if (group.name.empty() && !group.id.empty()) {
                group.name = group.id;  // Use id as name fallback
            }
            group.className = parseStrAttr("<g " + attrs + ">", "class", "");
            group.opacity = parseAttr("<g " + attrs + ">", "opacity", 1.0f);
            
            // Parse visibility
            std::string display = parseStrAttr("<g " + attrs + ">", "display", "");
            std::string visibility = parseStrAttr("<g " + attrs + ">", "visibility", "");
            group.visible = (display != "none" && visibility != "hidden");
            
            // Parse transform
            std::string transformStr = parseStrAttr("<g " + attrs + ">", "transform", "");
            if (!transformStr.empty()) {
                parseTransform(transformStr, group.transform);
                group.hasTransform = true;
            }
            
            // Only add groups with id or name (skip anonymous groups)
            if (!group.id.empty() || !group.name.empty()) {
                img.groups.push_back(group);
            }
        }
    }
    
    // Parse SVG transform attribute (basic support)
    void parseTransform(const std::string& str, float transform[6]) {
        // Default identity
        transform[0] = 1; transform[1] = 0;
        transform[2] = 0; transform[3] = 1;
        transform[4] = 0; transform[5] = 0;
        
        // translate(x, y)
        std::regex translateRegex(R"(translate\s*\(\s*([0-9.-]+)(?:\s*,?\s*([0-9.-]+))?\s*\))");
        std::smatch m;
        if (std::regex_search(str, m, translateRegex)) {
            try {
                transform[4] = std::stof(m[1].str());
                if (m[2].matched) transform[5] = std::stof(m[2].str());
            } catch (...) {}
        }
        
        // scale(sx, sy)
        std::regex scaleRegex(R"(scale\s*\(\s*([0-9.-]+)(?:\s*,?\s*([0-9.-]+))?\s*\))");
        if (std::regex_search(str, m, scaleRegex)) {
            try {
                transform[0] = std::stof(m[1].str());
                transform[3] = m[2].matched ? std::stof(m[2].str()) : transform[0];
            } catch (...) {}
        }
        
        // matrix(a,b,c,d,e,f)
        std::regex matrixRegex(R"(matrix\s*\(\s*([0-9.-]+)\s*,?\s*([0-9.-]+)\s*,?\s*([0-9.-]+)\s*,?\s*([0-9.-]+)\s*,?\s*([0-9.-]+)\s*,?\s*([0-9.-]+)\s*\))");
        if (std::regex_search(str, m, matrixRegex)) {
            try {
                transform[0] = std::stof(m[1].str());
                transform[1] = std::stof(m[2].str());
                transform[2] = std::stof(m[3].str());
                transform[3] = std::stof(m[4].str());
                transform[4] = std::stof(m[5].str());
                transform[5] = std::stof(m[6].str());
            } catch (...) {}
        }
    }

    void parsePaths(const std::string& svg, SvgImage& img) {
        // FIXED: Match both <path ...> and <path ... />
        std::regex pathRegex(R"(<path\s[^>]*d\s*=\s*\"([^\"]+)\"[^>]*>|<path\s[^>]*d\s*=\s*\"([^\"]+)\"[^>]*/>)");
        auto begin = std::sregex_iterator(svg.begin(), svg.end(), pathRegex);
        auto end = std::sregex_iterator();
        
        for (auto it = begin; it != end; ++it) {
            nxsvg::Shape s;
            s.type = nxsvg::Shape::PATH;
            std::string elem = it->str();
            std::string d = (*it)[1].str().empty() ? (*it)[2].str() : (*it)[1].str();
            s.path = parsePath(d);
            
            parseShapeStyle(elem, s);
            if (!s.path.empty()) {
                img.shapes.push_back(s);
            }
        }
    }

    void parseCircles(const std::string& svg, SvgImage& img) {
        std::regex circleRegex(R"(<circle\s[^>]*>|<circle\s[^>]*/>)");
        auto begin = std::sregex_iterator(svg.begin(), svg.end(), circleRegex);
        auto end = std::sregex_iterator();
        
        for (auto it = begin; it != end; ++it) {
            nxsvg::Shape s;
            s.type = nxsvg::Shape::CIRCLE;
            std::string elem = it->str();
            s.cx = parseAttr(elem, "cx", 0);
            s.cy = parseAttr(elem, "cy", 0);
            s.r = parseAttr(elem, "r", 0);
            parseShapeStyle(elem, s);
            if (s.r > 0) img.shapes.push_back(s);
        }
    }

    void parseEllipses(const std::string& svg, SvgImage& img) {
        std::regex ellipseRegex(R"(<ellipse\s[^>]*>|<ellipse\s[^>]*/>)");
        auto begin = std::sregex_iterator(svg.begin(), svg.end(), ellipseRegex);
        auto end = std::sregex_iterator();
        
        for (auto it = begin; it != end; ++it) {
            nxsvg::Shape s;
            s.type = nxsvg::Shape::ELLIPSE;
            std::string elem = it->str();
            s.cx = parseAttr(elem, "cx", 0);
            s.cy = parseAttr(elem, "cy", 0);
            s.rx = parseAttr(elem, "rx", 0);
            s.ry = parseAttr(elem, "ry", 0);
            parseShapeStyle(elem, s);
            if (s.rx > 0 && s.ry > 0) img.shapes.push_back(s);
        }
    }

    void parseRects(const std::string& svg, SvgImage& img) {
        std::regex rectRegex(R"(<rect\s[^>]*>|<rect\s[^>]*/>)");
        auto begin = std::sregex_iterator(svg.begin(), svg.end(), rectRegex);
        auto end = std::sregex_iterator();
        
        for (auto it = begin; it != end; ++it) {
            nxsvg::Shape s;
            s.type = nxsvg::Shape::RECT; // Fixed type usage
            std::string elem = it->str();
            s.x = parseAttr(elem, "x", 0);
            s.y = parseAttr(elem, "y", 0);
            s.w = parseAttr(elem, "width", 0);
            s.h = parseAttr(elem, "height", 0);
            s.rectRx = parseAttr(elem, "rx", 0);
            s.rectRy = parseAttr(elem, "ry", 0);
            // SVG spec: if only rx or ry specified, the other equals it
            if (s.rectRx > 0 && s.rectRy == 0) s.rectRy = s.rectRx;
            if (s.rectRy > 0 && s.rectRx == 0) s.rectRx = s.rectRy;
            parseShapeStyle(elem, s);
            if (s.w > 0 && s.h > 0) img.shapes.push_back(s);
        }
    }

    void parseLines(const std::string& svg, SvgImage& img) {
        std::regex lineRegex(R"(<line\s[^>]*>|<line\s[^>]*/>)");
        auto begin = std::sregex_iterator(svg.begin(), svg.end(), lineRegex);
        auto end = std::sregex_iterator();
        
        for (auto it = begin; it != end; ++it) {
            nxsvg::Shape s;
            s.type = nxsvg::Shape::LINE;
            std::string elem = it->str();
            s.x1 = parseAttr(elem, "x1", 0);
            s.y1 = parseAttr(elem, "y1", 0);
            s.x2 = parseAttr(elem, "x2", 0);
            s.y2 = parseAttr(elem, "y2", 0);
            parseShapeStyle(elem, s);
            img.shapes.push_back(s);
        }
    }

    void parsePolylines(const std::string& svg, SvgImage& img) {
        std::regex polyRegex(R"(<polyline\s[^>]*>|<polyline\s[^>]*/>)");
        auto begin = std::sregex_iterator(svg.begin(), svg.end(), polyRegex);
        auto end = std::sregex_iterator();
        
        for (auto it = begin; it != end; ++it) {
            nxsvg::Shape s;
            s.type = nxsvg::Shape::POLYLINE;
            std::string elem = it->str();
            s.points = parsePoints(parseStrAttr(elem, "points", ""));
            parseShapeStyle(elem, s);
            if (!s.points.empty()) img.shapes.push_back(s);
        }
    }

    void parsePolygons(const std::string& svg, SvgImage& img) {
        std::regex polyRegex(R"(<polygon\s[^>]*>|<polygon\s[^>]*/>)");
        auto begin = std::sregex_iterator(svg.begin(), svg.end(), polyRegex);
        auto end = std::sregex_iterator();
        
        for (auto it = begin; it != end; ++it) {
            nxsvg::Shape s;
            s.type = nxsvg::Shape::POLYGON;
            std::string elem = it->str();
            s.points = parsePoints(parseStrAttr(elem, "points", ""));
            parseShapeStyle(elem, s);
            if (!s.points.empty()) img.shapes.push_back(s);
        }
    }

    void parseShapeStyle(const std::string& elem, nxsvg::Shape& s) {
        std::string fillStr, strokeStr;
        
        // Parse style attribute (e.g., style="fill:#fff;stroke:#000")
        std::string styleAttr = parseStrAttr(elem, "style", "");
        if (!styleAttr.empty()) {
            std::regex fillStyleRegex(R"(fill\s*:\s*([^;]+))");
            std::regex strokeStyleRegex(R"(stroke\s*:\s*([^;]+))");
            std::smatch m;
            
            if (std::regex_search(styleAttr, m, fillStyleRegex)) {
                fillStr = trim(m[1].str());
            }
            if (std::regex_search(styleAttr, m, strokeStyleRegex)) {
                strokeStr = trim(m[1].str());
            }
        }
        
        // Fall back to direct attributes
        if (fillStr.empty()) fillStr = parseStrAttr(elem, "fill", "");
        if (strokeStr.empty()) strokeStr = parseStrAttr(elem, "stroke", "");
        
        // Apply SVG defaults: black fill, no stroke (unless explicitly set)
        if (fillStr.empty() && strokeStr.empty()) {
            // No styling specified - use SVG default (black fill)
            s.fill = parseColor("black");
            s.hasFill = true;
            s.hasStroke = false;
        } else {
            // Handle explicit fill
            if (fillStr == "none") {
                s.hasFill = false;
            } else if (!fillStr.empty()) {
                s.fill = parseColor(fillStr);
                s.hasFill = true;
            } else {
                // fill not specified but stroke is - default to black fill
                s.fill = parseColor("black");
                s.hasFill = true;
            }
            
            // Handle explicit stroke
            if (strokeStr == "none") {
                s.hasStroke = false;
            } else if (!strokeStr.empty()) {
                s.stroke = parseColor(strokeStr);
                s.hasStroke = true;
            } else {
                s.hasStroke = false;
            }
        }
        
        s.strokeWidth = parseAttr(elem, "stroke-width", 1.5f);
        s.fillRule = parseStrAttr(elem, "fill-rule", "nonzero");
        s.opacity = parseAttr(elem, "opacity", 1.0f);
        s.fillOpacity = parseAttr(elem, "fill-opacity", 1.0f);
        s.strokeOpacity = parseAttr(elem, "stroke-opacity", 1.0f);
    }

    std::vector<std::pair<float, float>> parsePoints(const std::string& pointsStr) {
        std::vector<std::pair<float, float>> points;
        std::regex numRegex(R"(-?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)");
        auto begin = std::sregex_iterator(pointsStr.begin(), pointsStr.end(), numRegex);
        auto end = std::sregex_iterator();
        
        std::vector<float> coords;
        for (auto it = begin; it != end; ++it) {
            try { coords.push_back(std::stof(it->str())); } catch(...) {}
        }
        
        for (size_t i = 0; i + 1 < coords.size(); i += 2) {
            points.push_back({coords[i], coords[i+1]});
        }
        return points;
    }

    std::vector<PathCommand> parsePath(const std::string& d) {
        std::vector<PathCommand> cmds;
        
        std::regex cmdRegex(R"(([MmLlHhVvZzCcSsQqTtAa])([^MmLlHhVvZzCcSsQqTtAa]*))");
        auto begin = std::sregex_iterator(d.begin(), d.end(), cmdRegex);
        auto end = std::sregex_iterator();
        
        for (auto it = begin; it != end; ++it) {
            PathCommand cmd;
            cmd.type = it->str(1)[0];
            std::string argsStr = it->str(2);
            
            std::regex numRegex(R"(-?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)");
            auto numBegin = std::sregex_iterator(argsStr.begin(), argsStr.end(), numRegex);
            auto numEnd = std::sregex_iterator();
            
            for (auto nit = numBegin; nit != numEnd; ++nit) {
                try {
                    cmd.args.push_back(std::stof(nit->str()));
                } catch (...) {}
            }
            
            cmds.push_back(cmd);
        }
        
        return cmds;
    }

    float parseAttr(const std::string& elem, const std::string& attr, float def) {
        std::regex r(attr + R"(\s*=\s*\"([^\"]+)\")");
        std::smatch m;
        if (std::regex_search(elem, m, r)) {
            try { return std::stof(m[1].str()); } catch (...) {}
        }
        return def;
    }

    std::string parseStrAttr(const std::string& elem, const std::string& attr, const std::string& def) {
        std::regex r(attr + R"(\s*=\s*\"([^\"]+)\")");
        std::smatch m;
        if (std::regex_search(elem, m, r)) return m[1].str();
        return def;
    }
};

} // namespace nxsvg
