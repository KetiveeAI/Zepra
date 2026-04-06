// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "path.h"
#include <algorithm>
#include <limits>

namespace NXRender {
namespace PathGen {

void Path::moveTo(float x, float y) {
    points_.push_back({{x, y}, PathCommand::MoveTo});
    currentPoint_ = {x, y};
}

void Path::lineTo(float x, float y) {
    points_.push_back({{x, y}, PathCommand::LineTo});
    currentPoint_ = {x, y};
}

void Path::quadTo(const Math::Vector2& control, const Math::Vector2& end) {
    points_.push_back({control, PathCommand::QuadTo});
    points_.push_back({end, PathCommand::QuadTo});
    currentPoint_ = end;
}

void Path::cubicTo(const Math::Vector2& control1, const Math::Vector2& control2, const Math::Vector2& end) {
    points_.push_back({control1, PathCommand::CubicTo});
    points_.push_back({control2, PathCommand::CubicTo});
    points_.push_back({end, PathCommand::CubicTo});
    currentPoint_ = end;
}

void Path::close() {
    if (!points_.empty()) {
        points_.push_back({currentPoint_, PathCommand::Close});
    }
}

void Path::clear() {
    points_.clear();
    currentPoint_ = {0.0f, 0.0f};
}

void Path::getBounds(Math::Vector2& outMin, Math::Vector2& outMax) const {
    if (points_.empty()) {
        outMin = outMax = {0.0f, 0.0f};
        return;
    }
    
    outMin = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    outMax = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

    for (const auto& pt : points_) {
        // Only considering endpoints for fast bounds. Precise bounds would require derivative roots, 
        // but AABB of control points encompasses the curve.
        outMin.x = std::min(outMin.x, pt.point.x);
        outMin.y = std::min(outMin.y, pt.point.y);
        outMax.x = std::max(outMax.x, pt.point.x);
        outMax.y = std::max(outMax.y, pt.point.y);
    }
}

} // namespace PathGen
} // namespace NXRender
