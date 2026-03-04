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
        if (name.empty() || name == "none") return Color(0, 0, 0, 0);
        if (name == "black") return Color(0, 0, 0);
        if (name == "white") return Color(255, 255, 255);
        if (name == "red") return Color(255, 0, 0);
        if (name == "green") return Color(0, 255, 0);
        if (name == "blue") return Color(0, 0, 255);
        if (name == "yellow") return Color(255, 255, 0);
        if (name == "purple") return Color(128, 0, 128);
        
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
                               fill, stroke, s.strokeWidth * scale, s.hasFill, s.hasStroke);
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
                           fill, stroke, s.strokeWidth * scale, s.hasFill, s.hasStroke);
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

    void renderRect(float x, float y, float w, float h, Color fill, Color stroke, float sw, bool hasFill, bool hasStroke) {
        if (w <= 0 || h <= 0) return;
        auto* gpu = NXRender::gpu();
        NXRender::Rect r(x, y, w, h);
        
        if (hasFill && fill.a > 0) {
            gpu->fillRect(r, fill);
        }
        if (hasStroke && stroke.a > 0) {
            gpu->strokeRect(r, stroke, sw);
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
        bool closedSubpath = false;
        
        auto flushContour = [&]() {
            if (!currentContour.empty()) {
                contours.push_back(currentContour);
                currentContour.clear();
                closedSubpath = false;
            }
        };

        for (const auto& cmd : cmds) {
            size_t argCount = cmd.args.size();
            
            switch (cmd.type) {
                case 'M': // Move To
                    flushContour();
                    if (argCount >= 2) {
                        cx = cmd.args[0]; cy = cmd.args[1];
                        startX = cx; startY = cy;
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
                    break;
                case 'l': // Relative Line To
                    for (size_t i = 0; i + 1 < argCount; i += 2) {
                        cx += cmd.args[i]; cy += cmd.args[i + 1];
                        currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                    }
                    break;
                case 'H': // Horizontal Line To
                    for (size_t i = 0; i < argCount; i++) {
                        cx = cmd.args[i];
                        currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                    }
                    break;
                case 'h': // Relative Horizontal Line To
                    for (size_t i = 0; i < argCount; i++) {
                        cx += cmd.args[i];
                        currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                    }
                    break;
                case 'V': // Vertical Line To
                    for (size_t i = 0; i < argCount; i++) {
                        cy = cmd.args[i];
                        currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                    }
                    break;
                case 'v': // Relative Vertical Line To
                    for (size_t i = 0; i < argCount; i++) {
                        cy += cmd.args[i];
                        currentContour.emplace_back(ox + cx * scale, oy + cy * scale);
                    }
                    break;
                case 'Z': case 'z': // Close Path
                    closedSubpath = true;
                    cx = startX; cy = startY;
                    // Ensure loop is closed visually
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
                        for (int t = 1; t <= 8; t++) {
                            float u = t / 8.0f, u2 = u * u, u3 = u2 * u;
                            float m = 1 - u, m2 = m * m, m3 = m2 * m;
                            float px = m3 * cx + 3 * m2 * u * x1 + 3 * m * u2 * x2 + u3 * x3;
                            float py = m3 * cy + 3 * m2 * u * y1 + 3 * m * u2 * y2 + u3 * y3;
                            currentContour.emplace_back(ox + px * scale, oy + py * scale);
                        }
                        cx = x3; cy = y3;
                    }
                    break;
                case 'c': // Relative Cubic Bezier
                    for (size_t i = 0; i + 5 < argCount; i += 6) {
                        float x1 = cx + cmd.args[i], y1 = cy + cmd.args[i+1];
                        float x2 = cx + cmd.args[i+2], y2 = cy + cmd.args[i+3];
                        float x3 = cx + cmd.args[i+4], y3 = cy + cmd.args[i+5];
                        for (int t = 1; t <= 8; t++) {
                            float u = t / 8.0f, u2 = u * u, u3 = u2 * u;
                            float m = 1 - u, m2 = m * m, m3 = m2 * m;
                            float px = m3 * cx + 3 * m2 * u * x1 + 3 * m * u2 * x2 + u3 * x3;
                            float py = m3 * cy + 3 * m2 * u * y1 + 3 * m * u2 * y2 + u3 * y3;
                            currentContour.emplace_back(ox + px * scale, oy + py * scale);
                        }
                        cx = x3; cy = y3;
                    }
                    break;
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
