// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "stroke.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace NXRender {
namespace PathGen {

static const float PI = 3.14159265358979323846f;
static const float EPSILON = 1e-5f;

// ==================================================================
// DashPattern
// ==================================================================

float DashPattern::totalLength() const {
    float total = 0;
    for (float s : segments) total += s;
    return total;
}

float DashPattern::patternAt(float distance, bool& isDash) const {
    if (segments.empty()) {
        isDash = true;
        return 0;
    }

    float total = totalLength();
    if (total <= 0) {
        isDash = true;
        return 0;
    }

    // Apply offset and wrap
    float d = std::fmod(distance + offset, total);
    if (d < 0) d += total;

    float accum = 0;
    for (size_t i = 0; i < segments.size(); i++) {
        accum += segments[i];
        if (d < accum) {
            isDash = (i % 2 == 0); // Even indices are dashes, odd are gaps
            return accum - d;      // Remaining in this segment
        }
    }

    isDash = true;
    return 0;
}

// ==================================================================
// DashGenerator
// ==================================================================

DashGenerator::DashGenerator(const DashPattern& pattern)
    : pattern_(pattern)
    , totalPatternLength_(pattern.totalLength()) {}

DashGenerator::WalkState DashGenerator::initState() const {
    WalkState state;
    state.patternOffset = 0;
    state.segmentIndex = 0;
    state.isDash = true;
    state.remaining = pattern_.segments.empty() ? 0 : pattern_.segments[0];

    // Apply dash offset
    if (pattern_.offset > 0 && totalPatternLength_ > 0) {
        float off = std::fmod(pattern_.offset, totalPatternLength_);
        advance(state, off);
    }

    return state;
}

void DashGenerator::advance(WalkState& state, float distance) const {
    if (pattern_.segments.empty()) return;

    while (distance > 0 && distance > EPSILON) {
        if (distance < state.remaining) {
            state.remaining -= distance;
            state.patternOffset += distance;
            return;
        }

        distance -= state.remaining;
        state.patternOffset += state.remaining;

        state.segmentIndex = (state.segmentIndex + 1) % static_cast<int>(pattern_.segments.size());
        state.isDash = (state.segmentIndex % 2 == 0);
        state.remaining = pattern_.segments[static_cast<size_t>(state.segmentIndex)];
    }
}

std::vector<std::vector<Point>> DashGenerator::generate(const std::vector<Point>& input, bool isClosed) {
    std::vector<std::vector<Point>> result;
    if (input.size() < 2 || pattern_.segments.empty()) return result;

    WalkState state = initState();
    std::vector<Point> currentDash;
    size_t count = input.size();

    for (size_t i = 0; i < count - 1; i++) {
        const Point& p0 = input[i];
        const Point& p1 = input[(i + 1) % count];

        float dx = p1.x - p0.x;
        float dy = p1.y - p0.y;
        float segLen = std::sqrt(dx * dx + dy * dy);
        if (segLen < EPSILON) continue;

        float dirX = dx / segLen;
        float dirY = dy / segLen;
        float consumed = 0;

        while (consumed < segLen - EPSILON) {
            float step = std::min(state.remaining, segLen - consumed);

            float startX = p0.x + dirX * consumed;
            float startY = p0.y + dirY * consumed;
            float endX = p0.x + dirX * (consumed + step);
            float endY = p0.y + dirY * (consumed + step);

            if (state.isDash) {
                if (currentDash.empty()) {
                    currentDash.push_back(Point(startX, startY));
                }
                currentDash.push_back(Point(endX, endY));
            } else {
                if (!currentDash.empty()) {
                    result.push_back(std::move(currentDash));
                    currentDash.clear();
                }
            }

            consumed += step;
            advance(state, step);
        }
    }

    // Handle closed path
    if (isClosed && count >= 3) {
        const Point& p0 = input[count - 1];
        const Point& p1 = input[0];
        float dx = p1.x - p0.x;
        float dy = p1.y - p0.y;
        float segLen = std::sqrt(dx * dx + dy * dy);

        if (segLen > EPSILON) {
            float dirX = dx / segLen;
            float dirY = dy / segLen;
            float consumed = 0;

            while (consumed < segLen - EPSILON) {
                float step = std::min(state.remaining, segLen - consumed);
                float startX = p0.x + dirX * consumed;
                float startY = p0.y + dirY * consumed;
                float endX = p0.x + dirX * (consumed + step);
                float endY = p0.y + dirY * (consumed + step);

                if (state.isDash) {
                    if (currentDash.empty()) currentDash.push_back(Point(startX, startY));
                    currentDash.push_back(Point(endX, endY));
                } else {
                    if (!currentDash.empty()) {
                        result.push_back(std::move(currentDash));
                        currentDash.clear();
                    }
                }

                consumed += step;
                advance(state, step);
            }
        }
    }

    if (!currentDash.empty()) {
        result.push_back(std::move(currentDash));
    }

    return result;
}

// ==================================================================
// StrokeGenerator
// ==================================================================

StrokeGenerator::StrokeGenerator(const StrokeOptions& options) : options_(options) {}

StrokeGenerator::~StrokeGenerator() {}

void StrokeGenerator::generateCap(const Point& p, const Point& normal, const Point& dir,
                                   bool isStart, float halfWidth, std::vector<Point>& output) {
    float hw = halfWidth;
    if (options_.cap == LineCap::Butt) {
        if (isStart) {
            output.push_back(Point(p.x + normal.x * hw, p.y + normal.y * hw));
            output.push_back(Point(p.x - normal.x * hw, p.y - normal.y * hw));
        } else {
            output.push_back(Point(p.x - normal.x * hw, p.y - normal.y * hw));
            output.push_back(Point(p.x + normal.x * hw, p.y + normal.y * hw));
        }
    } else if (options_.cap == LineCap::Square) {
        float ex = dir.x * hw;
        float ey = dir.y * hw;
        if (isStart) {
            output.push_back(Point(p.x - ex + normal.x * hw, p.y - ey + normal.y * hw));
            output.push_back(Point(p.x - ex - normal.x * hw, p.y - ey - normal.y * hw));
        } else {
            output.push_back(Point(p.x + ex - normal.x * hw, p.y + ey - normal.y * hw));
            output.push_back(Point(p.x + ex + normal.x * hw, p.y + ey + normal.y * hw));
        }
    } else if (options_.cap == LineCap::Round) {
        float startAngle = std::atan2(normal.y, normal.x);
        if (!isStart) startAngle += PI;

        int segments = std::max(4, static_cast<int>(std::ceil(hw * PI / 4.0f)));
        float theta = PI / segments;

        for (int i = 0; i <= segments; ++i) {
            float a = startAngle + (isStart ? i * theta : -i * theta);
            output.push_back(Point(p.x + std::cos(a) * hw, p.y + std::sin(a) * hw));
        }
    }
}

void StrokeGenerator::generateJoin(const Point& p, const Point& nA, const Point& prevDir,
                                    const Point& nB, const Point& currDir, float halfWidth,
                                    std::vector<Point>& outputPos, std::vector<Point>& outputNeg) {
    float hw = halfWidth;
    float cross = prevDir.x * currDir.y - prevDir.y * currDir.x;
    if (std::abs(cross) < EPSILON) {
        outputPos.push_back(Point(p.x + nA.x * hw, p.y + nA.y * hw));
        outputNeg.push_back(Point(p.x - nA.x * hw, p.y - nA.y * hw));
        return;
    }

    bool rightOuter = cross > 0;

    float miterX = nA.x + nB.x;
    float miterY = nA.y + nB.y;
    float miterNormSq = miterX * miterX + miterY * miterY;
    float miterRatio = 2.0f / miterNormSq;
    float mtx = miterX * miterRatio;
    float mty = miterY * miterRatio;

    Point innerPoint(p.x - (rightOuter ? mtx : -mtx) * hw,
                     p.y - (rightOuter ? mty : -mty) * hw);

    if (rightOuter) outputNeg.push_back(innerPoint);
    else outputPos.push_back(innerPoint);

    auto generateOuter = [&](std::vector<Point>& outList) {
        float limitSq = options_.miterLimit * options_.miterLimit;
        if (options_.join == LineJoin::Miter && miterRatio <= limitSq) {
            outList.push_back(Point(p.x + (rightOuter ? mtx : -mtx) * hw,
                                    p.y + (rightOuter ? mty : -mty) * hw));
        } else if (options_.join == LineJoin::Round) {
            float startAngle = std::atan2(nA.y, nA.x);
            float endAngle = std::atan2(nB.y, nB.x);
            if (rightOuter && endAngle < startAngle) endAngle += 2.0f * PI;
            if (!rightOuter && endAngle > startAngle) endAngle -= 2.0f * PI;

            float diff = endAngle - startAngle;
            int segments = std::max(2, static_cast<int>(std::ceil(std::abs(diff) * hw / 2.0f)));
            float theta = diff / segments;

            for (int i = 0; i <= segments; ++i) {
                float a = startAngle + i * theta;
                float sign = rightOuter ? 1.0f : -1.0f;
                outList.push_back(Point(p.x + std::cos(a) * hw * sign,
                                        p.y + std::sin(a) * hw * sign));
            }
        } else {
            float sign = rightOuter ? 1.0f : -1.0f;
            outList.push_back(Point(p.x + nA.x * hw * sign, p.y + nA.y * hw * sign));
            outList.push_back(Point(p.x + nB.x * hw * sign, p.y + nB.y * hw * sign));
        }
    };

    if (rightOuter) generateOuter(outputPos);
    else generateOuter(outputNeg);
}

std::vector<Point> StrokeGenerator::expandPath(const std::vector<Point>& inputLine, bool isClosed) {
    if (inputLine.size() < 2) return {};

    std::vector<Point> outputPos;
    std::vector<Point> outputNeg;
    float hw = options_.width * 0.5f;

    auto getNormal = [](const Point& p1, const Point& p2) {
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < EPSILON) return Point(0, 0);
        return Point(-dy / len, dx / len);
    };

    auto getDir = [](const Point& p1, const Point& p2) {
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < EPSILON) return Point(1, 0);
        return Point(dx / len, dy / len);
    };

    // Remove duplicates
    std::vector<Point> cleanLine;
    cleanLine.push_back(inputLine[0]);
    for (size_t i = 1; i < inputLine.size(); ++i) {
        if (std::abs(inputLine[i].x - cleanLine.back().x) > EPSILON ||
            std::abs(inputLine[i].y - cleanLine.back().y) > EPSILON) {
            cleanLine.push_back(inputLine[i]);
        }
    }

    if (isClosed && cleanLine.size() > 2) {
        if (std::abs(cleanLine.front().x - cleanLine.back().x) < EPSILON &&
            std::abs(cleanLine.front().y - cleanLine.back().y) < EPSILON) {
            cleanLine.pop_back();
        }
    }

    size_t count = cleanLine.size();
    if (count < 2) return {};

    std::vector<Point> normals(count);
    std::vector<Point> dirs(count);

    for (size_t i = 0; i < count; ++i) {
        size_t nextI = (i + 1) % count;
        if (!isClosed && i == count - 1) break;
        dirs[i] = getDir(cleanLine[i], cleanLine[nextI]);
        normals[i] = Point(-dirs[i].y, dirs[i].x);
    }
    if (!isClosed) {
        dirs[count - 1] = dirs[count - 2];
        normals[count - 1] = normals[count - 2];
    }

    if (!isClosed) {
        generateCap(cleanLine[0], normals[0], dirs[0], true, hw, outputPos);
    }

    for (size_t i = 0; i < count; ++i) {
        if (!isClosed && (i == 0 || i == count - 1)) {
            if (i == 0) continue;
        }

        size_t prevI = (i == 0) ? count - 1 : i - 1;

        if (isClosed || (i > 0 && i < count - 1)) {
            generateJoin(cleanLine[i], normals[prevI], dirs[prevI],
                         normals[i], dirs[i], hw, outputPos, outputNeg);
        }
    }

    if (!isClosed) {
        generateCap(cleanLine[count - 1], normals[count - 1], dirs[count - 1], false, hw, outputNeg);
    } else {
        generateJoin(cleanLine[0], normals[count - 1], dirs[count - 1],
                     normals[0], dirs[0], hw, outputPos, outputNeg);
    }

    std::vector<Point> finalContour = outputPos;
    for (auto it = outputNeg.rbegin(); it != outputNeg.rend(); ++it) {
        finalContour.push_back(*it);
    }

    return finalContour;
}

std::vector<Point> StrokeGenerator::expandPathVariable(const std::vector<Point>& inputLine,
                                                        const std::vector<float>& widths,
                                                        bool isClosed) {
    if (inputLine.size() < 2 || widths.size() != inputLine.size()) return {};

    std::vector<Point> outputPos;
    std::vector<Point> outputNeg;

    auto getNormal = [](const Point& p1, const Point& p2) {
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < EPSILON) return Point(0, 0);
        return Point(-dy / len, dx / len);
    };

    size_t count = inputLine.size();

    for (size_t i = 0; i < count; ++i) {
        size_t nextI = std::min(i + 1, count - 1);
        Point normal = getNormal(inputLine[i], inputLine[nextI]);
        float hw = widths[i] * 0.5f;

        outputPos.push_back(Point(inputLine[i].x + normal.x * hw,
                                   inputLine[i].y + normal.y * hw));
        outputNeg.push_back(Point(inputLine[i].x - normal.x * hw,
                                   inputLine[i].y - normal.y * hw));
    }

    std::vector<Point> contour = outputPos;
    for (auto it = outputNeg.rbegin(); it != outputNeg.rend(); ++it) {
        contour.push_back(*it);
    }

    return contour;
}

std::vector<std::vector<Point>> StrokeGenerator::expandDashed(const std::vector<Point>& inputLine,
                                                                bool isClosed) {
    std::vector<std::vector<Point>> result;
    if (options_.dash.empty()) {
        result.push_back(expandPath(inputLine, isClosed));
        return result;
    }

    DashGenerator gen(options_.dash);
    auto dashes = gen.generate(inputLine, isClosed);

    for (auto& dashLine : dashes) {
        if (dashLine.size() >= 2) {
            auto polygon = expandPath(dashLine, false);
            if (!polygon.empty()) {
                result.push_back(std::move(polygon));
            }
        }
    }

    return result;
}

float StrokeGenerator::pathLength(const std::vector<Point>& path) {
    float total = 0;
    for (size_t i = 1; i < path.size(); i++) {
        float dx = path[i].x - path[i - 1].x;
        float dy = path[i].y - path[i - 1].y;
        total += std::sqrt(dx * dx + dy * dy);
    }
    return total;
}

Point StrokeGenerator::pointAtDistance(const std::vector<Point>& path, float distance) {
    if (path.empty()) return Point(0, 0);
    if (path.size() == 1) return path[0];

    float accum = 0;
    for (size_t i = 1; i < path.size(); i++) {
        float dx = path[i].x - path[i - 1].x;
        float dy = path[i].y - path[i - 1].y;
        float segLen = std::sqrt(dx * dx + dy * dy);

        if (accum + segLen >= distance) {
            float t = (segLen > EPSILON) ? (distance - accum) / segLen : 0;
            return Point(path[i - 1].x + dx * t, path[i - 1].y + dy * t);
        }
        accum += segLen;
    }

    return path.back();
}

Point StrokeGenerator::tangentAtDistance(const std::vector<Point>& path, float distance) {
    if (path.size() < 2) return Point(1, 0);

    float accum = 0;
    for (size_t i = 1; i < path.size(); i++) {
        float dx = path[i].x - path[i - 1].x;
        float dy = path[i].y - path[i - 1].y;
        float segLen = std::sqrt(dx * dx + dy * dy);

        if (accum + segLen >= distance || i == path.size() - 1) {
            if (segLen < EPSILON) return Point(1, 0);
            return Point(dx / segLen, dy / segLen);
        }
        accum += segLen;
    }

    return Point(1, 0);
}

} // namespace PathGen
} // namespace NXRender
