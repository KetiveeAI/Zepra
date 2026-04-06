// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "path_builder.h"
#include <cmath>

namespace NXRender {
namespace PathGen {

void PathBuilder::flattenQuad(const Math::Vector2& p0, const Math::Vector2& p1, const Math::Vector2& p2, std::vector<Math::Vector2>& output) {
    // Recursive subdivision approach based on geometric flatness calculation
    Math::Vector2 dx = p0 - p1 * 2.0f + p2;
    float distSq = dx.lengthSquared();

    if (distSq <= tolerance_ * tolerance_) {
        output.push_back(p2);
        return;
    }

    // Subdivide
    Math::Vector2 p01 = (p0 + p1) * 0.5f;
    Math::Vector2 p12 = (p1 + p2) * 0.5f;
    Math::Vector2 p012 = (p01 + p12) * 0.5f;

    flattenQuad(p0, p01, p012, output);
    flattenQuad(p012, p12, p2, output);
}

void PathBuilder::flattenCubic(const Math::Vector2& p0, const Math::Vector2& p1, const Math::Vector2& p2, const Math::Vector2& p3, std::vector<Math::Vector2>& output) {
    // Evaluate flatness. We use the distance from control points to the baseline as a proxy
    Math::Vector2 dir = p3 - p0;
    float lenSq = dir.lengthSquared();
    
    float err = 0.0f;
    if (lenSq > 1e-6f) {
        float invLenSq = 1.0f / lenSq;
        Math::Vector2 norm(-dir.y, dir.x); // orthogonal
        
        float d1 = std::abs((p1 - p0).dot(norm)) * invLenSq;
        float d2 = std::abs((p2 - p0).dot(norm)) * invLenSq;
        err = std::max(d1, d2);
    } else {
        err = (p1 - p0).length() + (p2 - p0).length();
    }

    if (err <= tolerance_) {
        output.push_back(p3);
        return;
    }

    // Subdivide using De Casteljau's algorithm
    Math::Vector2 p01 = (p0 + p1) * 0.5f;
    Math::Vector2 p12 = (p1 + p2) * 0.5f;
    Math::Vector2 p23 = (p2 + p3) * 0.5f;

    Math::Vector2 p012 = (p01 + p12) * 0.5f;
    Math::Vector2 p123 = (p12 + p23) * 0.5f;

    Math::Vector2 p0123 = (p012 + p123) * 0.5f;

    flattenCubic(p0, p01, p012, p0123, output);
    flattenCubic(p0123, p123, p23, p3, output);
}

std::vector<Math::Vector2> PathBuilder::flatten(const Path& path) {
    std::vector<Math::Vector2> result;
    const auto& points = path.getPoints();

    if (points.empty()) return result;

    Math::Vector2 currentPoint = points[0].point;
    result.push_back(currentPoint);

    size_t i = 1;
    while(i < points.size()) {
        const auto& pt = points[i];
        if (pt.command == PathCommand::MoveTo) {
            currentPoint = pt.point;
            result.push_back(currentPoint);
            i++;
        } else if (pt.command == PathCommand::LineTo) {
            currentPoint = pt.point;
            result.push_back(currentPoint);
            i++;
        } else if (pt.command == PathCommand::QuadTo) {
            if (i + 1 < points.size()) {
                flattenQuad(currentPoint, pt.point, points[i+1].point, result);
                currentPoint = points[i+1].point;
                i += 2;
            } else {
                break;
            }
        } else if (pt.command == PathCommand::CubicTo) {
            if (i + 2 < points.size()) {
                flattenCubic(currentPoint, pt.point, points[i+1].point, points[i+2].point, result);
                currentPoint = points[i+2].point;
                i += 3;
            } else {
                break;
            }
        } else if (pt.command == PathCommand::Close) {
            // Technically close to the last MoveTo point, but for flatting simple connection is enough.
            // We just let the caller handle closing constraints in tessellation.
            i++;
        } else {
            i++;
        }
    }

    return result;
}

} // namespace PathGen
} // namespace NXRender
