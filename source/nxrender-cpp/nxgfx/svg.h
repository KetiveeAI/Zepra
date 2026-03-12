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
#include <GL/gl.h>

namespace nxsvg {

struct Color {
    uint8_t r = 255, g = 255, b = 255, a = 255;
    
    static Color parse(const std::string& str) {
        Color c;
        if (str.empty() || str == "none") { c.a = 0; return c; }
        if (str == "white") { c.r = c.g = c.b = 255; return c; }
        if (str == "black") { c.r = c.g = c.b = 0; return c; }
        if (str == "red") { c.r = 255; c.g = c.b = 0; return c; }
        if (str == "green") { c.r = c.b = 0; c.g = 128; return c; }
        if (str == "blue") { c.r = c.g = 0; c.b = 255; return c; }
        if (str == "currentColor") { c.r = c.g = c.b = 255; return c; }
        
        if (str.length() > 0 && str[0] == '#') {
            try {
                if (str.length() == 7) {
                    c.r = std::stoi(str.substr(1, 2), nullptr, 16);
                    c.g = std::stoi(str.substr(3, 2), nullptr, 16);
                    c.b = std::stoi(str.substr(5, 2), nullptr, 16);
                } else if (str.length() == 4) {
                    c.r = std::stoi(str.substr(1, 1) + str.substr(1, 1), nullptr, 16);
                    c.g = std::stoi(str.substr(2, 1) + str.substr(2, 1), nullptr, 16);
                    c.b = std::stoi(str.substr(3, 1) + str.substr(3, 1), nullptr, 16);
                }
            } catch (...) {}
        }
        return c;
    }
    
    bool isCurrentColor() const { return r == 255 && g == 255 && b == 255 && a == 255; }
};

struct PathCommand {
    char type = 'M';
    std::vector<float> args;
};

struct Shape {
    enum Type { PATH, CIRCLE, RECT, LINE, ELLIPSE };
    Type type = PATH;
    std::vector<PathCommand> path;
    float cx = 0, cy = 0, r = 0, rx = 0, ry = 0;
    float x = 0, y = 0, w = 0, h = 0;
    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    Color stroke, fill;
    float strokeWidth = 1.5f;
    bool hasFill = false;
    bool hasStroke = false;
};

class SvgImage {
public:
    float width = 24, height = 24;
    float viewBoxX = 0, viewBoxY = 0, viewBoxW = 24, viewBoxH = 24;
    std::vector<Shape> shapes;

    void render(float x, float y, float size, uint8_t cr = 255, uint8_t cg = 255, uint8_t cb = 255) {
        float scale = size / std::max(viewBoxW, viewBoxH);
        float ox = x - viewBoxX * scale;
        float oy = y - viewBoxY * scale;
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        
        for (const Shape& s : shapes) {
            Color stroke = s.stroke;
            Color fill = s.fill;
            if (stroke.isCurrentColor()) { stroke.r = cr; stroke.g = cg; stroke.b = cb; }
            if (fill.isCurrentColor()) { fill.r = cr; fill.g = cg; fill.b = cb; }
            
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
                    renderPath(s.path, ox, oy, scale, fill, stroke, s.strokeWidth * scale, s.hasFill, s.hasStroke);
                    break;
                case Shape::ELLIPSE:
                    renderEllipse(ox + s.cx * scale, oy + s.cy * scale, 
                                  s.rx * scale, s.ry * scale, fill, stroke, s.strokeWidth * scale, s.hasFill, s.hasStroke);
                    break;
            }
        }
        
        glLineWidth(1.0f);
    }

private:
    void renderCircle(float cx, float cy, float r, Color fill, Color stroke, float sw, bool hasFill, bool hasStroke) {
        if (r <= 0) return;
        if (hasFill && fill.a > 0) {
            glColor4ub(fill.r, fill.g, fill.b, fill.a);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(cx, cy);
            for (int i = 0; i <= 32; i++) {
                float ang = i * 6.28318f / 32.0f;
                glVertex2f(cx + cosf(ang) * r, cy + sinf(ang) * r);
            }
            glEnd();
        }
        if (hasStroke && stroke.a > 0) {
            glColor4ub(stroke.r, stroke.g, stroke.b, stroke.a);
            glLineWidth(std::max(1.0f, sw));
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 32; i++) {
                float ang = i * 6.28318f / 32.0f;
                glVertex2f(cx + cosf(ang) * r, cy + sinf(ang) * r);
            }
            glEnd();
        }
    }

    void renderEllipse(float cx, float cy, float rx, float ry, Color fill, Color stroke, float sw, bool hasFill, bool hasStroke) {
        if (rx <= 0 || ry <= 0) return;
        if (hasFill && fill.a > 0) {
            glColor4ub(fill.r, fill.g, fill.b, fill.a);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(cx, cy);
            for (int i = 0; i <= 32; i++) {
                float ang = i * 6.28318f / 32.0f;
                glVertex2f(cx + cosf(ang) * rx, cy + sinf(ang) * ry);
            }
            glEnd();
        }
        if (hasStroke && stroke.a > 0) {
            glColor4ub(stroke.r, stroke.g, stroke.b, stroke.a);
            glLineWidth(std::max(1.0f, sw));
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 32; i++) {
                float ang = i * 6.28318f / 32.0f;
                glVertex2f(cx + cosf(ang) * rx, cy + sinf(ang) * ry);
            }
            glEnd();
        }
    }

    void renderRect(float x, float y, float w, float h, Color fill, Color stroke, float sw, bool hasFill, bool hasStroke) {
        if (w <= 0 || h <= 0) return;
        if (hasFill && fill.a > 0) {
            glColor4ub(fill.r, fill.g, fill.b, fill.a);
            glBegin(GL_QUADS);
            glVertex2f(x, y); glVertex2f(x + w, y); 
            glVertex2f(x + w, y + h); glVertex2f(x, y + h);
            glEnd();
        }
        if (hasStroke && stroke.a > 0) {
            glColor4ub(stroke.r, stroke.g, stroke.b, stroke.a);
            glLineWidth(std::max(1.0f, sw));
            glBegin(GL_LINE_LOOP);
            glVertex2f(x, y); glVertex2f(x + w, y); 
            glVertex2f(x + w, y + h); glVertex2f(x, y + h);
            glEnd();
        }
    }

    void renderLine(float x1, float y1, float x2, float y2, Color stroke, float sw) {
        if (stroke.a > 0) {
            glColor4ub(stroke.r, stroke.g, stroke.b, stroke.a);
            glLineWidth(std::max(1.0f, sw));
            glBegin(GL_LINES);
            glVertex2f(x1, y1); glVertex2f(x2, y2);
            glEnd();
        }
    }

    void renderPath(const std::vector<PathCommand>& cmds, float ox, float oy, float scale, 
                    Color fill, Color stroke, float sw, bool hasFill, bool hasStroke) {
        if (cmds.empty()) return;
        
        std::vector<std::pair<float, float>> points;
        float cx = 0, cy = 0, startX = 0, startY = 0;

        for (const auto& cmd : cmds) {
            size_t argCount = cmd.args.size();
            
            switch (cmd.type) {
                case 'M':
                    if (argCount >= 2) {
                        cx = cmd.args[0]; cy = cmd.args[1];
                        startX = cx; startY = cy;
                        points.push_back({ox + cx * scale, oy + cy * scale});
                        for (size_t i = 2; i + 1 < argCount; i += 2) {
                            cx = cmd.args[i]; cy = cmd.args[i + 1];
                            points.push_back({ox + cx * scale, oy + cy * scale});
                        }
                    }
                    break;
                case 'm':
                    if (argCount >= 2) {
                        cx += cmd.args[0]; cy += cmd.args[1];
                        startX = cx; startY = cy;
                        points.push_back({ox + cx * scale, oy + cy * scale});
                        for (size_t i = 2; i + 1 < argCount; i += 2) {
                            cx += cmd.args[i]; cy += cmd.args[i + 1];
                            points.push_back({ox + cx * scale, oy + cy * scale});
                        }
                    }
                    break;
                case 'L':
                    for (size_t i = 0; i + 1 < argCount; i += 2) {
                        cx = cmd.args[i]; cy = cmd.args[i + 1];
                        points.push_back({ox + cx * scale, oy + cy * scale});
                    }
                    break;
                case 'l':
                    for (size_t i = 0; i + 1 < argCount; i += 2) {
                        cx += cmd.args[i]; cy += cmd.args[i + 1];
                        points.push_back({ox + cx * scale, oy + cy * scale});
                    }
                    break;
                case 'H':
                    for (size_t i = 0; i < argCount; i++) {
                        cx = cmd.args[i];
                        points.push_back({ox + cx * scale, oy + cy * scale});
                    }
                    break;
                case 'h':
                    for (size_t i = 0; i < argCount; i++) {
                        cx += cmd.args[i];
                        points.push_back({ox + cx * scale, oy + cy * scale});
                    }
                    break;
                case 'V':
                    for (size_t i = 0; i < argCount; i++) {
                        cy = cmd.args[i];
                        points.push_back({ox + cx * scale, oy + cy * scale});
                    }
                    break;
                case 'v':
                    for (size_t i = 0; i < argCount; i++) {
                        cy += cmd.args[i];
                        points.push_back({ox + cx * scale, oy + cy * scale});
                    }
                    break;
                case 'Z': case 'z':
                    cx = startX; cy = startY;
                    if (!points.empty() && (points.back().first != ox + cx * scale || points.back().second != oy + cy * scale)) {
                        points.push_back({ox + cx * scale, oy + cy * scale});
                    }
                    break;
                case 'C':
                    for (size_t i = 0; i + 5 < argCount; i += 6) {
                        float x1 = cmd.args[i], y1 = cmd.args[i+1];
                        float x2 = cmd.args[i+2], y2 = cmd.args[i+3];
                        float x3 = cmd.args[i+4], y3 = cmd.args[i+5];
                        for (int t = 1; t <= 8; t++) {
                            float u = t / 8.0f, u2 = u * u, u3 = u2 * u;
                            float m = 1 - u, m2 = m * m, m3 = m2 * m;
                            float px = m3 * cx + 3 * m2 * u * x1 + 3 * m * u2 * x2 + u3 * x3;
                            float py = m3 * cy + 3 * m2 * u * y1 + 3 * m * u2 * y2 + u3 * y3;
                            points.push_back({ox + px * scale, oy + py * scale});
                        }
                        cx = x3; cy = y3;
                    }
                    break;
                case 'c':
                    for (size_t i = 0; i + 5 < argCount; i += 6) {
                        float x1 = cx + cmd.args[i], y1 = cy + cmd.args[i+1];
                        float x2 = cx + cmd.args[i+2], y2 = cy + cmd.args[i+3];
                        float x3 = cx + cmd.args[i+4], y3 = cy + cmd.args[i+5];
                        for (int t = 1; t <= 8; t++) {
                            float u = t / 8.0f, u2 = u * u, u3 = u2 * u;
                            float m = 1 - u, m2 = m * m, m3 = m2 * m;
                            float px = m3 * cx + 3 * m2 * u * x1 + 3 * m * u2 * x2 + u3 * x3;
                            float py = m3 * cy + 3 * m2 * u * y1 + 3 * m * u2 * y2 + u3 * y3;
                            points.push_back({ox + px * scale, oy + py * scale});
                        }
                        cx = x3; cy = y3;
                    }
                    break;
            }
        }

        if (points.size() < 2) return;

        if (hasFill && fill.a > 0) {
            glColor4ub(fill.r, fill.g, fill.b, fill.a);
            glBegin(GL_TRIANGLE_FAN);
            for (auto& p : points) glVertex2f(p.first, p.second);
            glEnd();
        }
        
        if (hasStroke && stroke.a > 0) {
            glColor4ub(stroke.r, stroke.g, stroke.b, stroke.a);
            glLineWidth(std::max(1.0f, sw));
            glBegin(GL_LINE_STRIP);
            for (auto& p : points) glVertex2f(p.first, p.second);
            glEnd();
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
        parsePaths(svg, img);
        parseCircles(svg, img);
        parseRects(svg, img);
        parseEllipses(svg, img);
        parseLines(svg, img);
        images_[name] = img;
        return true;
    }

    void draw(const std::string& name, float x, float y, float size, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255) {
        auto it = images_.find(name);
        if (it != images_.end()) {
            it->second.render(x, y, size, r, g, b);
        }
    }

    bool has(const std::string& name) const {
        return images_.find(name) != images_.end();
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

    void parsePaths(const std::string& svg, SvgImage& img) {
        // FIXED: Match both <path ...> and <path ... />
        std::regex pathRegex(R"(<path\s[^>]*d\s*=\s*\"([^\"]+)\"[^>]*>|<path\s[^>]*d\s*=\s*\"([^\"]+)\"[^>]*/>)");
        auto begin = std::sregex_iterator(svg.begin(), svg.end(), pathRegex);
        auto end = std::sregex_iterator();
        
        for (auto it = begin; it != end; ++it) {
            Shape s;
            s.type = Shape::PATH;
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
            Shape s;
            s.type = Shape::CIRCLE;
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
            Shape s;
            s.type = Shape::ELLIPSE;
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
            Shape s;
            s.type = Shape::RECT;
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
            Shape s;
            s.type = Shape::LINE;
            std::string elem = it->str();
            s.x1 = parseAttr(elem, "x1", 0);
            s.y1 = parseAttr(elem, "y1", 0);
            s.x2 = parseAttr(elem, "x2", 0);
            s.y2 = parseAttr(elem, "y2", 0);
            parseShapeStyle(elem, s);
            img.shapes.push_back(s);
        }
    }

    void parseShapeStyle(const std::string& elem, Shape& s) {
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
            s.fill = Color::parse("black");
            s.hasFill = true;
            s.hasStroke = false;
        } else {
            // Handle explicit fill
            if (fillStr == "none") {
                s.hasFill = false;
            } else if (!fillStr.empty()) {
                s.fill = Color::parse(fillStr);
                s.hasFill = true;
            } else {
                // fill not specified but stroke is - default to black fill
                s.fill = Color::parse("black");
                s.hasFill = true;
            }
            
            // Handle explicit stroke
            if (strokeStr == "none") {
                s.hasStroke = false;
            } else if (!strokeStr.empty()) {
                s.stroke = Color::parse(strokeStr);
                s.hasStroke = true;
            } else {
                s.hasStroke = false;
            }
        }
        
        s.strokeWidth = parseAttr(elem, "stroke-width", 1.5f);
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
