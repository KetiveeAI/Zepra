// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "nxgfx/math/vector.h"
#include <vector>

namespace NXRender {
namespace PathGen {

enum class PathCommand {
    MoveTo,
    LineTo,
    QuadTo,
    CubicTo,
    Close
};

struct PathPoint {
    Math::Vector2 point;
    PathCommand command;
};

class Path {
public:
    Path() = default;

    void moveTo(float x, float y);
    void moveTo(const Math::Vector2& p) { moveTo(p.x, p.y); }
    
    void lineTo(float x, float y);
    void lineTo(const Math::Vector2& p) { lineTo(p.x, p.y); }
    
    void quadTo(const Math::Vector2& control, const Math::Vector2& end);
    void cubicTo(const Math::Vector2& control1, const Math::Vector2& control2, const Math::Vector2& end);
    
    void close();
    void clear();

    const std::vector<PathPoint>& getPoints() const { return points_; }
    bool isEmpty() const { return points_.empty(); }

    // Computes bounding box of the path
    void getBounds(Math::Vector2& outMin, Math::Vector2& outMax) const;

private:
    std::vector<PathPoint> points_;
    Math::Vector2 currentPoint_{0.0f, 0.0f};
};

} // namespace PathGen
} // namespace NXRender
