// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "nxgfx/primitives.h"
#include <vector>
#include <cstdint>

namespace NXRender {
namespace PathGen {

enum class LineCap {
    Butt,
    Round,
    Square
};

enum class LineJoin {
    Miter,
    Round,
    Bevel
};

/**
 * @brief Dash pattern definition.
 *
 * A dash pattern is a repeating sequence of on/off lengths.
 * E.g., {10, 5} = 10px dash, 5px gap.
 *       {10, 5, 3, 5} = 10px dash, 5px gap, 3px dash, 5px gap.
 */
struct DashPattern {
    std::vector<float> segments; // Alternating dash/gap lengths
    float offset = 0;           // Starting offset into the pattern

    DashPattern() = default;
    DashPattern(std::initializer_list<float> segs) : segments(segs) {}
    DashPattern(std::initializer_list<float> segs, float off)
        : segments(segs), offset(off) {}

    bool empty() const { return segments.empty(); }
    float totalLength() const;
    float patternAt(float distance, bool& isDash) const;
};

struct StrokeOptions {
    float width = 1.0f;
    LineCap cap = LineCap::Butt;
    LineJoin join = LineJoin::Miter;
    float miterLimit = 4.0f;
    DashPattern dash;
};

/**
 * @brief Produces dashed polylines from a solid input path.
 *
 * Walks along the path at the dash/gap intervals and produces
 * sub-segments that represent only the "on" (dash) portions.
 */
class DashGenerator {
public:
    DashGenerator(const DashPattern& pattern);

    /**
     * @brief Break a polyline into dash segments.
     * @param input Input polyline.
     * @param isClosed Whether the path is closed.
     * @return Vector of polylines, one per dash.
     */
    std::vector<std::vector<Point>> generate(const std::vector<Point>& input, bool isClosed);

private:
    DashPattern pattern_;
    float totalPatternLength_;

    struct WalkState {
        float patternOffset;  // Current position in the pattern
        int segmentIndex;     // Current segment in the pattern
        bool isDash;          // Currently in a dash (true) or gap (false)
        float remaining;      // Remaining length in current segment
    };

    WalkState initState() const;
    void advance(WalkState& state, float distance) const;
};

/**
 * @brief Expands a 1D polyline into a 2D stroke polygon.
 *
 * Handles line caps, line joins, miter limits, and variable-width strokes.
 * Optionally applies dash patterns before expansion.
 */
class StrokeGenerator {
public:
    StrokeGenerator(const StrokeOptions& options = StrokeOptions());
    ~StrokeGenerator();

    /**
     * @brief Expand a polyline into the stroke boundary polygon.
     */
    std::vector<Point> expandPath(const std::vector<Point>& inputLine, bool isClosed);

    /**
     * @brief Expand with per-vertex width variation.
     * @param widths Width at each vertex (must match inputLine size).
     */
    std::vector<Point> expandPathVariable(const std::vector<Point>& inputLine,
                                           const std::vector<float>& widths, bool isClosed);

    /**
     * @brief Generate dashed stroke polygons.
     * @return Vector of polygons, one per dash segment.
     */
    std::vector<std::vector<Point>> expandDashed(const std::vector<Point>& inputLine, bool isClosed);

    /**
     * @brief Compute the total length of a polyline.
     */
    static float pathLength(const std::vector<Point>& path);

    /**
     * @brief Sample a point at a given distance along the path.
     */
    static Point pointAtDistance(const std::vector<Point>& path, float distance);

    /**
     * @brief Compute the tangent direction at a given distance.
     */
    static Point tangentAtDistance(const std::vector<Point>& path, float distance);

private:
    StrokeOptions options_;

    struct VertexNormal {
        Point n;
        float len;
    };

    void generateCap(const Point& p, const Point& normal, const Point& dir,
                     bool isStart, float halfWidth, std::vector<Point>& output);
    void generateJoin(const Point& p, const Point& nA, const Point& prevDir,
                      const Point& nB, const Point& currDir, float halfWidth,
                      std::vector<Point>& outputPos, std::vector<Point>& outputNeg);
};

} // namespace PathGen
} // namespace NXRender
