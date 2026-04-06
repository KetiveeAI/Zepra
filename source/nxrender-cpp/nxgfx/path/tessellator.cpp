// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "tessellator.h"
#include <list>
#include <cmath>
#include <algorithm>

namespace NXRender {
namespace PathGen {

float Tessellator::signedArea(const std::vector<Math::Vector2>& polygon) {
    float area = 0.0f;
    size_t n = polygon.size();
    for(size_t i = 0; i < n; i++) {
        size_t j = (i + 1) % n;
        area += (polygon[i].x * polygon[j].y) - (polygon[j].x * polygon[i].y);
    }
    return area / 2.0f;
}

bool Tessellator::isConvex(const Math::Vector2& prev, const Math::Vector2& curr, const Math::Vector2& next) {
    // Cross product to test convexity. Assuming CCW polygon, a positive cross product means convex
    float cross = (curr.x - prev.x) * (next.y - curr.y) - (curr.y - prev.y) * (next.x - curr.x);
    return cross > 0.0f;
}

bool Tessellator::isPointInTriangle(const Math::Vector2& pt, const Math::Vector2& a, const Math::Vector2& b, const Math::Vector2& c) {
    float cross1 = (b.x - a.x) * (pt.y - a.y) - (b.y - a.y) * (pt.x - a.x);
    float cross2 = (c.x - b.x) * (pt.y - b.y) - (c.y - b.y) * (pt.x - b.x);
    float cross3 = (a.x - c.x) * (pt.y - c.y) - (a.y - c.y) * (pt.x - c.x);

    // Point is strictly inside if all signs are positive or all signs are negative
    bool has_neg = (cross1 < 0) || (cross2 < 0) || (cross3 < 0);
    bool has_pos = (cross1 > 0) || (cross2 > 0) || (cross3 > 0);

    return !(has_neg && has_pos);
}

std::vector<Trip> Tessellator::triangulate(const std::vector<Math::Vector2>& polygon) {
    std::vector<Trip> triangles;
    if (polygon.size() < 3) return triangles;

    std::vector<int> indices;
    indices.reserve(polygon.size());
    for(size_t i = 0; i < polygon.size(); i++) {
        indices.push_back(static_cast<int>(i));
    }

    // Force CCW order
    if (signedArea(polygon) < 0) {
        std::reverse(indices.begin(), indices.end());
    }

    while(indices.size() > 3) {
        bool earFound = false;
        int n = static_cast<int>(indices.size());

        for(int i = 0; i < n; i++) {
            int prev_idx = indices[(i == 0) ? (n - 1) : (i - 1)];
            int curr_idx = indices[i];
            int next_idx = indices[(i + 1) % n];

            const Math::Vector2& p = polygon[prev_idx];
            const Math::Vector2& c = polygon[curr_idx];
            const Math::Vector2& n_pt = polygon[next_idx];

            if (isConvex(p, c, n_pt)) {
                bool isEar = true;
                for(int j = 0; j < n; j++) {
                    int test_idx = indices[j];
                    if (test_idx == prev_idx || test_idx == curr_idx || test_idx == next_idx) continue;
                    
                    if (isPointInTriangle(polygon[test_idx], p, c, n_pt)) {
                        isEar = false;
                        break;
                    }
                }

                if (isEar) {
                    triangles.push_back({prev_idx, curr_idx, next_idx});
                    indices.erase(indices.begin() + i);
                    earFound = true;
                    break;
                }
            }
        }
        
        if (!earFound) {
            // Degenerate polygon or self-intersecting fallback.
            // Rather than infinite looping, we forcefully drop a vertex.
            indices.erase(indices.begin());
        }
    }

    if (indices.size() == 3) {
        triangles.push_back({indices[0], indices[1], indices[2]});
    }

    return triangles;
}

} // namespace PathGen
} // namespace NXRender
