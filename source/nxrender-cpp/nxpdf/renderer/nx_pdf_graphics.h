/**
 * @file nx_pdf_graphics.h
 * @brief Graphics state machine implementation (CTM)
 */

#pragma once

#include <vector>

namespace nxrender {
namespace pdf {
namespace renderer {

struct Point {
    double x = 0;
    double y = 0;
};

// 3x3 Affine Transformation Matrix mapped for 2D translation
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

struct GraphicsState {
    Matrix ctm;
    double lineWidth = 1.0;
    Color strokeColor;
    Color fillColor;
};

// Central stack isolated correctly
class GraphicsStateStack {
public:
    GraphicsStateStack() {
        stack_.push_back(GraphicsState());
    }

    void Push() {
        stack_.push_back(stack_.back());
    }

    void Pop() {
        if (stack_.size() > 1) {
            stack_.pop_back();
        }
    }

    GraphicsState& Current() { return stack_.back(); }
    const GraphicsState& Current() const { return stack_.back(); }

private:
    std::vector<GraphicsState> stack_;
};

} // namespace renderer
} // namespace pdf
} // namespace nxrender
