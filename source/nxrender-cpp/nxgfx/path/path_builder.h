// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "path.h"
#include <vector>

namespace NXRender {
namespace PathGen {

class PathBuilder {
public:
    PathBuilder() = default;

    // Set allowable error (tolerance) for curve flattening
    void setTolerance(float t) { tolerance_ = t; }

    // Flattens a path into pure line segments (removes curves)
    std::vector<Math::Vector2> flatten(const Path& path);

private:
    float tolerance_ = 0.25f; // Screen space tolerance in pixels

    void flattenQuad(const Math::Vector2& p0, const Math::Vector2& p1, const Math::Vector2& p2, std::vector<Math::Vector2>& output);
    void flattenCubic(const Math::Vector2& p0, const Math::Vector2& p1, const Math::Vector2& p2, const Math::Vector2& p3, std::vector<Math::Vector2>& output);
};

} // namespace PathGen
} // namespace NXRender
