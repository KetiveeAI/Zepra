// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "nx_pdf_graphics.h"
#include <cmath>
#include <algorithm>

namespace nxrender {
namespace pdf {
namespace renderer {

// ==================================================================
// Matrix operations
// ==================================================================

Matrix Matrix::Inverse() const {
    double det = a * d - b * c;
    if (std::abs(det) < 1e-12) return Matrix(); // Singular

    double invDet = 1.0 / det;
    Matrix inv;
    inv.a = d * invDet;
    inv.b = -b * invDet;
    inv.c = -c * invDet;
    inv.d = a * invDet;
    inv.e = (c * f - d * e) * invDet;
    inv.f = (b * e - a * f) * invDet;
    return inv;
}

Matrix Matrix::Multiply(const Matrix& m) const {
    Matrix result;
    result.a = a * m.a + b * m.c;
    result.b = a * m.b + b * m.d;
    result.c = c * m.a + d * m.c;
    result.d = c * m.b + d * m.d;
    result.e = e * m.a + f * m.c + m.e;
    result.f = e * m.b + f * m.d + m.f;
    return result;
}

Matrix Matrix::Translate(double tx, double ty) {
    Matrix m;
    m.e = tx;
    m.f = ty;
    return m;
}

Matrix Matrix::Scale(double sx, double sy) {
    Matrix m;
    m.a = sx;
    m.d = sy;
    return m;
}

Matrix Matrix::Rotate(double angle) {
    double rad = angle * 3.14159265358979323846 / 180.0;
    Matrix m;
    m.a = std::cos(rad);
    m.b = std::sin(rad);
    m.c = -std::sin(rad);
    m.d = std::cos(rad);
    return m;
}

// ==================================================================
// Path construction
// ==================================================================

void PathBuilder::moveTo(double x, double y) {
    PathCommand cmd;
    cmd.type = PathCommandType::MoveTo;
    cmd.x = x;
    cmd.y = y;
    commands_.push_back(cmd);
    currentX_ = x;
    currentY_ = y;
}

void PathBuilder::lineTo(double x, double y) {
    PathCommand cmd;
    cmd.type = PathCommandType::LineTo;
    cmd.x = x;
    cmd.y = y;
    commands_.push_back(cmd);
    currentX_ = x;
    currentY_ = y;
}

void PathBuilder::curveTo(double x1, double y1, double x2, double y2,
                           double x3, double y3) {
    PathCommand cmd;
    cmd.type = PathCommandType::CurveTo;
    cmd.x = x1;
    cmd.y = y1;
    cmd.x2 = x2;
    cmd.y2 = y2;
    cmd.x3 = x3;
    cmd.y3 = y3;
    commands_.push_back(cmd);
    currentX_ = x3;
    currentY_ = y3;
}

void PathBuilder::closePath() {
    PathCommand cmd;
    cmd.type = PathCommandType::ClosePath;
    commands_.push_back(cmd);
}

void PathBuilder::rectangle(double x, double y, double w, double h) {
    moveTo(x, y);
    lineTo(x + w, y);
    lineTo(x + w, y + h);
    lineTo(x, y + h);
    closePath();
}

void PathBuilder::clear() {
    commands_.clear();
    currentX_ = 0;
    currentY_ = 0;
}

// Transform all path points by a matrix
void PathBuilder::transform(const Matrix& m) {
    for (auto& cmd : commands_) {
        Point p = m.Transform({cmd.x, cmd.y});
        cmd.x = p.x;
        cmd.y = p.y;
        if (cmd.type == PathCommandType::CurveTo) {
            Point p2 = m.Transform({cmd.x2, cmd.y2});
            Point p3 = m.Transform({cmd.x3, cmd.y3});
            cmd.x2 = p2.x;
            cmd.y2 = p2.y;
            cmd.x3 = p3.x;
            cmd.y3 = p3.y;
        }
    }
}

// Compute axis-aligned bounding box
void PathBuilder::bounds(double& minX, double& minY,
                          double& maxX, double& maxY) const {
    minX = minY = 1e18;
    maxX = maxY = -1e18;
    for (const auto& cmd : commands_) {
        if (cmd.type == PathCommandType::ClosePath) continue;
        minX = std::min(minX, cmd.x);
        minY = std::min(minY, cmd.y);
        maxX = std::max(maxX, cmd.x);
        maxY = std::max(maxY, cmd.y);
        if (cmd.type == PathCommandType::CurveTo) {
            minX = std::min({minX, cmd.x2, cmd.x3});
            minY = std::min({minY, cmd.y2, cmd.y3});
            maxX = std::max({maxX, cmd.x2, cmd.x3});
            maxY = std::max({maxY, cmd.y2, cmd.y3});
        }
    }
    if (commands_.empty()) {
        minX = minY = maxX = maxY = 0;
    }
}

// ==================================================================
// GraphicsStateStack extended operations
// ==================================================================

void GraphicsStateStack::SetStrokeColor(double r, double g, double b) {
    Current().strokeColor = {r, g, b};
}

void GraphicsStateStack::SetFillColor(double r, double g, double b) {
    Current().fillColor = {r, g, b};
}

void GraphicsStateStack::SetLineWidth(double w) {
    Current().lineWidth = w;
}

void GraphicsStateStack::ConcatCTM(const Matrix& m) {
    Current().ctm.Concat(m);
}

void GraphicsStateStack::SetLineCap(int cap) {
    Current().lineCap = cap;
}

void GraphicsStateStack::SetLineJoin(int join) {
    Current().lineJoin = join;
}

void GraphicsStateStack::SetMiterLimit(double limit) {
    Current().miterLimit = limit;
}

void GraphicsStateStack::SetDash(const std::vector<double>& pattern, double phase) {
    Current().dashPattern = pattern;
    Current().dashPhase = phase;
}

} // namespace renderer
} // namespace pdf
} // namespace nxrender
