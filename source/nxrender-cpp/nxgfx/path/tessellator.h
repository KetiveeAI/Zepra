// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "nxgfx/math/vector.h"
#include <vector>

namespace NXRender {
namespace PathGen {

struct Trip {
    int v0, v1, v2;
};

class Tessellator {
public:
    // Triangulates a simple polygon (no holes). 
    // Assumes outer polygon vertices are ordered counter-clockwise.
    // Ear-clipping algorithm.
    static std::vector<Trip> triangulate(const std::vector<Math::Vector2>& polygon);

private:
    static bool isConvex(const Math::Vector2& prev, const Math::Vector2& curr, const Math::Vector2& next);
    static bool isPointInTriangle(const Math::Vector2& pt, const Math::Vector2& v1, const Math::Vector2& v2, const Math::Vector2& v3);
    static float signedArea(const std::vector<Math::Vector2>& polygon);
};

} // namespace PathGen
} // namespace NXRender
