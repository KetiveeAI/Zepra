// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include <vector>
#include <cmath>

namespace nxrender {
namespace pdf {
namespace renderer {

struct Point {
    double x = 0;
    double y = 0;
};

struct Matrix {
    double a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;

    void Concat(const Matrix& m) {
        double na = a * m.a + b * m.c;
        double nb = a * m.b + b * m.d;
        double nc = c * m.a + d * m.c;
        double nd = c * m.b + d * m.d;
        double ne = e * m.a + f * m.c + m.e;
        double nf = e * m.b + f * m.d + m.f;
        a = na; b = nb; c = nc; d = nd; e = ne; f = nf;
    }

    Point Transform(const Point& p) const {
        return { p.x * a + p.y * c + e, p.x * b + p.y * d + f };
    }

    Matrix Inverse() const;
    Matrix Multiply(const Matrix& m) const;

    static Matrix Translate(double tx, double ty);
    static Matrix Scale(double sx, double sy);
    static Matrix Rotate(double angle);
};

struct Color {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
};

inline Color CMYKToRGB(double c, double m, double y, double k) {
    return {
        (1.0 - c) * (1.0 - k),
        (1.0 - m) * (1.0 - k),
        (1.0 - y) * (1.0 - k)
    };
}

// ==================================================================
// Path representation
// ==================================================================

enum class PathCommandType {
    MoveTo,
    LineTo,
    CurveTo,
    ClosePath
};

struct PathCommand {
    PathCommandType type = PathCommandType::MoveTo;
    double x = 0, y = 0;     // Primary point
    double x2 = 0, y2 = 0;   // Control point 2 (CurveTo)
    double x3 = 0, y3 = 0;   // End point (CurveTo)
};

class PathBuilder {
public:
    void moveTo(double x, double y);
    void lineTo(double x, double y);
    void curveTo(double x1, double y1, double x2, double y2,
                  double x3, double y3);
    void closePath();
    void rectangle(double x, double y, double w, double h);
    void clear();

    void transform(const Matrix& m);
    void bounds(double& minX, double& minY, double& maxX, double& maxY) const;

    const std::vector<PathCommand>& commands() const { return commands_; }
    bool empty() const { return commands_.empty(); }

    double currentX() const { return currentX_; }
    double currentY() const { return currentY_; }

private:
    std::vector<PathCommand> commands_;
    double currentX_ = 0;
    double currentY_ = 0;
};

// ==================================================================
// Graphics state
// ==================================================================

struct GraphicsState {
    Matrix ctm;
    double lineWidth = 1.0;
    Color strokeColor;
    Color fillColor;
    int lineCap = 0;
    int lineJoin = 0;
    double miterLimit = 10.0;
    std::vector<double> dashPattern;
    double dashPhase = 0;
    double flatness = 1.0;
};

class GraphicsStateStack {
public:
    GraphicsStateStack() {
        stack_.push_back(GraphicsState());
    }

    void Push() { stack_.push_back(stack_.back()); }

    void Pop() {
        if (stack_.size() > 1) stack_.pop_back();
    }

    GraphicsState& Current() { return stack_.back(); }
    const GraphicsState& Current() const { return stack_.back(); }

    // Convenience setters
    void SetStrokeColor(double r, double g, double b);
    void SetFillColor(double r, double g, double b);
    void SetLineWidth(double w);
    void ConcatCTM(const Matrix& m);
    void SetLineCap(int cap);
    void SetLineJoin(int join);
    void SetMiterLimit(double limit);
    void SetDash(const std::vector<double>& pattern, double phase);

    size_t depth() const { return stack_.size(); }

private:
    std::vector<GraphicsState> stack_;
};

} // namespace renderer
} // namespace pdf
} // namespace nxrender
