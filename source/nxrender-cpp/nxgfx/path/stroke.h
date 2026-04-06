// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "nxgfx/math/vector.h"
#include <vector>

namespace NXRender {
namespace PathGen {

enum class LineJoin {
    Miter,
    Bevel
};

enum class LineCap {
    Butt,
    Square
};

struct StrokeOptions {
    float width = 1.0f;
    float miterLimit = 10.0f;
    LineJoin join = LineJoin::Miter;
    LineCap cap = LineCap::Butt;
};

class StrokeGenerator {
public:
    // Expands a list of flat points into a stroked polygon that can be tessellated.
    // Assumes points is an open or closed flat path.
    static std::vector<Math::Vector2> expand(const std::vector<Math::Vector2>& points, bool closed, const StrokeOptions& options);
    
private:
    static void addMiterJoin(std::vector<Math::Vector2>& outVerts, const Math::Vector2& prevNorm, const Math::Vector2& nextNorm, const Math::Vector2& pt, float halfWidth, float miterLimit, bool isLeft);
    static void addBevelJoin(std::vector<Math::Vector2>& outVerts, const Math::Vector2& prevNorm, const Math::Vector2& nextNorm, const Math::Vector2& pt, float halfWidth, bool isLeft);
};

} // namespace PathGen
} // namespace NXRender
