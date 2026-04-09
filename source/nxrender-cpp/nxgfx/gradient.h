// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file gradient.h
 * @brief Linear, radial, and conic gradient builder and renderer.
 */

#pragma once

#include "color.h"
#include "primitives.h"
#include <vector>
#include <cstdint>

namespace NXRender {

class GpuContext;

enum class GradientType : uint8_t {
    Linear,
    Radial,
    Conic
};

enum class GradientSpread : uint8_t {
    Pad,       // Extend edge color
    Repeat,    // Tile
    Reflect    // Mirror
};

struct GradientStop {
    float offset;  // 0.0 - 1.0
    Color color;

    GradientStop() : offset(0) {}
    GradientStop(float off, const Color& c) : offset(off), color(c) {}
};

/**
 * @brief Immutable gradient definition.
 *
 * Built via GradientBuilder, then rasterized to a 1D texture
 * (linear/conic) or 2D texture (radial) and cached by the
 * ResourceCache using a content hash key.
 */
class Gradient {
public:
    Gradient();

    GradientType type() const { return type_; }
    GradientSpread spread() const { return spread_; }
    const std::vector<GradientStop>& stops() const { return stops_; }
    size_t stopCount() const { return stops_.size(); }

    // Linear
    Point startPoint() const { return startPoint_; }
    Point endPoint() const { return endPoint_; }
    float angleDeg() const { return angleDeg_; }

    // Radial
    Point center() const { return center_; }
    float radiusX() const { return radiusX_; }
    float radiusY() const { return radiusY_; }
    Point focalPoint() const { return focalPoint_; }

    // Conic
    float startAngle() const { return startAngle_; }

    /**
     * @brief Sample the gradient color at a given position [0..1].
     */
    Color sample(float t) const;

    /**
     * @brief Rasterize the gradient into a pixel buffer.
     * For linear/conic: 1D strip (width x 1).
     * For radial: 2D (width x width).
     */
    std::vector<uint8_t> rasterize(int width) const;

    /**
     * @brief Rasterize directly into a rect of RGBA pixels.
     */
    std::vector<uint8_t> rasterizeRect(int width, int height) const;

    /**
     * @brief Create a GPU texture from this gradient.
     * @return Texture ID, or 0 on failure.
     */
    uint32_t createTexture(GpuContext* ctx, int resolution = 256) const;

    /**
     * @brief Content hash for cache lookups.
     */
    uint64_t hash() const;

private:
    friend class GradientBuilder;

    GradientType type_ = GradientType::Linear;
    GradientSpread spread_ = GradientSpread::Pad;
    std::vector<GradientStop> stops_;

    // Linear
    Point startPoint_;
    Point endPoint_ = Point(1, 0);
    float angleDeg_ = 0;

    // Radial
    Point center_ = Point(0.5f, 0.5f);
    float radiusX_ = 0.5f;
    float radiusY_ = 0.5f;
    Point focalPoint_ = Point(0.5f, 0.5f);

    // Conic
    float startAngle_ = 0;

    void sortStops();
};

/**
 * @brief Fluent builder for creating Gradient objects.
 */
class GradientBuilder {
public:
    GradientBuilder() = default;

    // Type
    GradientBuilder& linear() { grad_.type_ = GradientType::Linear; return *this; }
    GradientBuilder& radial() { grad_.type_ = GradientType::Radial; return *this; }
    GradientBuilder& conic() { grad_.type_ = GradientType::Conic; return *this; }

    // Stops
    GradientBuilder& addStop(float offset, const Color& color);
    GradientBuilder& clearStops();

    // Linear params
    GradientBuilder& from(float x, float y) { grad_.startPoint_ = Point(x, y); return *this; }
    GradientBuilder& to(float x, float y) { grad_.endPoint_ = Point(x, y); return *this; }
    GradientBuilder& angle(float degrees);

    // Radial params
    GradientBuilder& center(float x, float y) { grad_.center_ = Point(x, y); return *this; }
    GradientBuilder& radius(float r) { grad_.radiusX_ = r; grad_.radiusY_ = r; return *this; }
    GradientBuilder& radius(float rx, float ry) { grad_.radiusX_ = rx; grad_.radiusY_ = ry; return *this; }
    GradientBuilder& focalPoint(float x, float y) { grad_.focalPoint_ = Point(x, y); return *this; }

    // Conic
    GradientBuilder& startAngle(float deg) { grad_.startAngle_ = deg; return *this; }

    // Spread
    GradientBuilder& pad() { grad_.spread_ = GradientSpread::Pad; return *this; }
    GradientBuilder& repeat() { grad_.spread_ = GradientSpread::Repeat; return *this; }
    GradientBuilder& reflect() { grad_.spread_ = GradientSpread::Reflect; return *this; }

    Gradient build();

    // Presets
    static Gradient sunset(float angle = 135);
    static Gradient ocean(float angle = 180);
    static Gradient fire(float angle = 0);
    static Gradient rainbow();
    static Gradient nightSky();

private:
    Gradient grad_;
};

} // namespace NXRender
