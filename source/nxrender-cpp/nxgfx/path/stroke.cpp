// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "stroke.h"

namespace NXRender {
namespace PathGen {

void StrokeGenerator::addMiterJoin(std::vector<Math::Vector2>& outVerts, const Math::Vector2& prevNorm, const Math::Vector2& nextNorm, const Math::Vector2& pt, float halfWidth, float miterLimit, bool isLeft) {
    Math::Vector2 avgNorm = (prevNorm + nextNorm).normalized();
    float dot = prevNorm.dot(avgNorm);
    
    // Check if miter exceeds limit or angle is nearly 180 degrees
    if (dot < 0.1f || (1.0f / dot) > miterLimit) {
        addBevelJoin(outVerts, prevNorm, nextNorm, pt, halfWidth, isLeft);
        return;
    }
    
    float miterLen = halfWidth / dot;
    Math::Vector2 miterExt = avgNorm * miterLen;
    if (!isLeft) miterExt = miterExt * -1.0f;
    outVerts.push_back(pt + miterExt);
}

void StrokeGenerator::addBevelJoin(std::vector<Math::Vector2>& outVerts, const Math::Vector2& prevNorm, const Math::Vector2& nextNorm, const Math::Vector2& pt, float halfWidth, bool isLeft) {
    Math::Vector2 ext1 = prevNorm * halfWidth;
    Math::Vector2 ext2 = nextNorm * halfWidth;
    if (!isLeft) {
        ext1 = ext1 * -1.0f;
        ext2 = ext2 * -1.0f;
    }
    outVerts.push_back(pt + ext1);
    outVerts.push_back(pt + ext2);
}

std::vector<Math::Vector2> StrokeGenerator::expand(const std::vector<Math::Vector2>& points, bool closed, const StrokeOptions& options) {
    std::vector<Math::Vector2> result;
    if (points.size() < 2) return result;

    float hw = options.width * 0.5f;
    size_t n = points.size();

    std::vector<Math::Vector2> normals(closed ? n : n - 1);
    
    if (closed) {
        for(size_t i = 0; i < n; i++) {
            Math::Vector2 dir = points[(i + 1) % n] - points[i];
            dir = dir.normalized();
            normals[i] = Math::Vector2(-dir.y, dir.x);
        }
    } else {
        for(size_t i = 0; i < n - 1; i++) {
            Math::Vector2 dir = points[i + 1] - points[i];
            dir = dir.normalized();
            normals[i] = Math::Vector2(-dir.y, dir.x);
        }
    }

    std::vector<Math::Vector2> leftSide;
    std::vector<Math::Vector2> rightSide;

    // Generate Left and Right boundaries
    for(size_t i = 0; i < n; i++) {
        const Math::Vector2& p = points[i];
        
        if (!closed) {
            if (i == 0) {
                // start cap
                Math::Vector2 n0 = normals[0];
                if (options.cap == LineCap::Square) {
                    Math::Vector2 dir0(n0.y, -n0.x);
                    leftSide.push_back(p + n0 * hw - dir0 * hw);
                    rightSide.push_back(p - n0 * hw - dir0 * hw);
                } else {
                    leftSide.push_back(p + n0 * hw);
                    rightSide.push_back(p - n0 * hw);
                }
            } else if (i == n - 1) {
                // end cap
                Math::Vector2 n1 = normals[i - 1];
                if (options.cap == LineCap::Square) {
                    Math::Vector2 dir1(n1.y, -n1.x);
                    leftSide.push_back(p + n1 * hw + dir1 * hw);
                    rightSide.push_back(p - n1 * hw + dir1 * hw);
                } else {
                    leftSide.push_back(p + n1 * hw);
                    rightSide.push_back(p - n1 * hw);
                }
            } else {
                // intermediate join
                Math::Vector2 nPrev = normals[i - 1];
                Math::Vector2 nNext = normals[i];
                float cross = nPrev.x * nNext.y - nPrev.y * nNext.x;
                
                if (options.join == LineJoin::Miter) {
                    addMiterJoin(leftSide, nPrev, nNext, p, hw, options.miterLimit, true);
                    addMiterJoin(rightSide, nPrev, nNext, p, hw, options.miterLimit, false);
                } else {
                    addBevelJoin(leftSide, nPrev, nNext, p, hw, true);
                    addBevelJoin(rightSide, nPrev, nNext, p, hw, false);
                }
            }
        } else {
            // closed loop
            Math::Vector2 nPrev = normals[(i == 0) ? n - 1 : i - 1];
            Math::Vector2 nNext = normals[i];
            
            if (options.join == LineJoin::Miter) {
                addMiterJoin(leftSide, nPrev, nNext, p, hw, options.miterLimit, true);
                addMiterJoin(rightSide, nPrev, nNext, p, hw, options.miterLimit, false);
            } else {
                addBevelJoin(leftSide, nPrev, nNext, p, hw, true);
                addBevelJoin(rightSide, nPrev, nNext, p, hw, false);
            }
        }
    }

    // Stitch together to form a full polygon ring
    if (!closed) {
        // Outline travels up the left side, then down the right side in reverse
        result.reserve(leftSide.size() + rightSide.size());
        for (const auto& l : leftSide) result.push_back(l);
        for (int i = (int)rightSide.size() - 1; i >= 0; i--) result.push_back(rightSide[i]);
    } else {
        // Closed implies an inner hole and an outer shell. We return outer shell for now 
        // as true nested hole polygon filling requires complex winding. 
        // To be exact we return CCW ordered shell.
        result = leftSide; 
    }

    return result;
}

} // namespace PathGen
} // namespace NXRender
