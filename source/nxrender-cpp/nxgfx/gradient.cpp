// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "nxgfx/gradient.h"
#include "nxgfx/context.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <numeric>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace NXRender {

// ==================================================================
// Gradient
// ==================================================================

Gradient::Gradient() {}

void Gradient::sortStops() {
    std::sort(stops_.begin(), stops_.end(),
              [](const GradientStop& a, const GradientStop& b) {
                  return a.offset < b.offset;
              });
}

Color Gradient::sample(float t) const {
    if (stops_.empty()) return Color(0, 0, 0);
    if (stops_.size() == 1) return stops_[0].color;

    // Apply spread mode
    switch (spread_) {
        case GradientSpread::Pad:
            t = std::clamp(t, 0.0f, 1.0f);
            break;
        case GradientSpread::Repeat:
            t = t - std::floor(t);
            break;
        case GradientSpread::Reflect: {
            float period = std::floor(t);
            t = t - period;
            if (static_cast<int>(period) % 2 != 0) t = 1.0f - t;
            break;
        }
    }

    // Before first stop
    if (t <= stops_.front().offset) return stops_.front().color;
    // After last stop
    if (t >= stops_.back().offset) return stops_.back().color;

    // Find surrounding stops
    for (size_t i = 0; i < stops_.size() - 1; i++) {
        if (t >= stops_[i].offset && t <= stops_[i + 1].offset) {
            float segLen = stops_[i + 1].offset - stops_[i].offset;
            if (segLen <= 0.0f) return stops_[i].color;

            float localT = (t - stops_[i].offset) / segLen;
            const Color& a = stops_[i].color;
            const Color& b = stops_[i + 1].color;

            return Color(
                static_cast<uint8_t>(a.r + (b.r - a.r) * localT),
                static_cast<uint8_t>(a.g + (b.g - a.g) * localT),
                static_cast<uint8_t>(a.b + (b.b - a.b) * localT),
                static_cast<uint8_t>(a.a + (b.a - a.a) * localT)
            );
        }
    }

    return stops_.back().color;
}

std::vector<uint8_t> Gradient::rasterize(int width) const {
    std::vector<uint8_t> pixels(static_cast<size_t>(width) * 4);

    for (int x = 0; x < width; x++) {
        float t = static_cast<float>(x) / static_cast<float>(width - 1);
        Color c = sample(t);
        size_t idx = static_cast<size_t>(x) * 4;
        pixels[idx + 0] = c.r;
        pixels[idx + 1] = c.g;
        pixels[idx + 2] = c.b;
        pixels[idx + 3] = c.a;
    }

    return pixels;
}

std::vector<uint8_t> Gradient::rasterizeRect(int width, int height) const {
    std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float t;
            float fx = static_cast<float>(x) / static_cast<float>(width - 1);
            float fy = static_cast<float>(y) / static_cast<float>(height - 1);

            switch (type_) {
                case GradientType::Linear: {
                    // Project (fx, fy) onto gradient line
                    float dx = endPoint_.x - startPoint_.x;
                    float dy = endPoint_.y - startPoint_.y;
                    float len2 = dx * dx + dy * dy;
                    if (len2 < 0.0001f) {
                        t = 0.0f;
                    } else {
                        t = ((fx - startPoint_.x) * dx + (fy - startPoint_.y) * dy) / len2;
                    }
                    break;
                }
                case GradientType::Radial: {
                    float ddx = (fx - center_.x) / radiusX_;
                    float ddy = (fy - center_.y) / radiusY_;
                    t = std::sqrt(ddx * ddx + ddy * ddy);
                    break;
                }
                case GradientType::Conic: {
                    float angle = std::atan2(fy - center_.y, fx - center_.x);
                    angle = angle * 180.0f / static_cast<float>(M_PI) + 180.0f;
                    angle = std::fmod(angle - startAngle_ + 360.0f, 360.0f);
                    t = angle / 360.0f;
                    break;
                }
            }

            Color c = sample(t);
            size_t idx = (static_cast<size_t>(y) * width + x) * 4;
            pixels[idx + 0] = c.r;
            pixels[idx + 1] = c.g;
            pixels[idx + 2] = c.b;
            pixels[idx + 3] = c.a;
        }
    }

    return pixels;
}

uint32_t Gradient::createTexture(GpuContext* ctx, int resolution) const {
    if (!ctx) return 0;

    if (type_ == GradientType::Linear) {
        auto pixels = rasterize(resolution);
        return ctx->createTexture(resolution, 1, pixels.data());
    } else {
        auto pixels = rasterizeRect(resolution, resolution);
        return ctx->createTexture(resolution, resolution, pixels.data());
    }
}

uint64_t Gradient::hash() const {
    uint64_t h = static_cast<uint64_t>(type_) * 0x9E3779B97F4A7C15ULL;
    h ^= static_cast<uint64_t>(spread_) * 0x517CC1B727220A95ULL;

    for (const auto& stop : stops_) {
        uint64_t stopHash = *reinterpret_cast<const uint32_t*>(&stop.offset);
        stopHash ^= static_cast<uint64_t>(stop.color.r) << 24;
        stopHash ^= static_cast<uint64_t>(stop.color.g) << 16;
        stopHash ^= static_cast<uint64_t>(stop.color.b) << 8;
        stopHash ^= static_cast<uint64_t>(stop.color.a);
        h ^= stopHash * 0x9E3779B97F4A7C15ULL;
        h = (h << 31) | (h >> 33);
    }

    // Mix in geometry
    auto mixFloat = [&h](float f) {
        uint32_t bits;
        memcpy(&bits, &f, sizeof(bits));
        h ^= static_cast<uint64_t>(bits) * 0x517CC1B727220A95ULL;
        h = (h << 27) | (h >> 37);
    };

    mixFloat(startPoint_.x); mixFloat(startPoint_.y);
    mixFloat(endPoint_.x); mixFloat(endPoint_.y);
    mixFloat(center_.x); mixFloat(center_.y);
    mixFloat(radiusX_); mixFloat(radiusY_);
    mixFloat(startAngle_);

    return h;
}

// ==================================================================
// GradientBuilder
// ==================================================================

GradientBuilder& GradientBuilder::addStop(float offset, const Color& color) {
    grad_.stops_.emplace_back(offset, color);
    return *this;
}

GradientBuilder& GradientBuilder::clearStops() {
    grad_.stops_.clear();
    return *this;
}

GradientBuilder& GradientBuilder::angle(float degrees) {
    grad_.angleDeg_ = degrees;
    float rad = degrees * static_cast<float>(M_PI) / 180.0f;
    grad_.startPoint_ = Point(0.5f - std::cos(rad) * 0.5f,
                               0.5f - std::sin(rad) * 0.5f);
    grad_.endPoint_ = Point(0.5f + std::cos(rad) * 0.5f,
                             0.5f + std::sin(rad) * 0.5f);
    return *this;
}

Gradient GradientBuilder::build() {
    grad_.sortStops();

    // Ensure we have at least a start and end stop
    if (grad_.stops_.empty()) {
        grad_.stops_.emplace_back(0.0f, Color(0, 0, 0));
        grad_.stops_.emplace_back(1.0f, Color(255, 255, 255));
    } else if (grad_.stops_.size() == 1) {
        grad_.stops_.emplace_back(1.0f, grad_.stops_[0].color);
    }

    return grad_;
}

Gradient GradientBuilder::sunset(float angle) {
    return GradientBuilder()
        .linear()
        .angle(angle)
        .addStop(0.0f, Color(0xFF, 0x51, 0x2F))
        .addStop(0.5f, Color(0xFF, 0x99, 0x3F))
        .addStop(1.0f, Color(0xFC, 0xD2, 0x5C))
        .build();
}

Gradient GradientBuilder::ocean(float angle) {
    return GradientBuilder()
        .linear()
        .angle(angle)
        .addStop(0.0f, Color(0x00, 0x52, 0xD4))
        .addStop(0.5f, Color(0x41, 0x9E, 0xDB))
        .addStop(1.0f, Color(0x65, 0xC7, 0xF7))
        .build();
}

Gradient GradientBuilder::fire(float angle) {
    return GradientBuilder()
        .linear()
        .angle(angle)
        .addStop(0.0f, Color(0xF8, 0x31, 0x00))
        .addStop(0.3f, Color(0xFF, 0x6B, 0x08))
        .addStop(0.6f, Color(0xFF, 0xA2, 0x00))
        .addStop(1.0f, Color(0xFF, 0xD4, 0x3B))
        .build();
}

Gradient GradientBuilder::rainbow() {
    return GradientBuilder()
        .linear()
        .angle(90)
        .addStop(0.00f, Color(0xFF, 0x00, 0x00))
        .addStop(0.17f, Color(0xFF, 0xA5, 0x00))
        .addStop(0.33f, Color(0xFF, 0xFF, 0x00))
        .addStop(0.50f, Color(0x00, 0x80, 0x00))
        .addStop(0.67f, Color(0x00, 0x00, 0xFF))
        .addStop(0.83f, Color(0x4B, 0x00, 0x82))
        .addStop(1.00f, Color(0x94, 0x00, 0xD3))
        .build();
}

Gradient GradientBuilder::nightSky() {
    return GradientBuilder()
        .radial()
        .center(0.5f, 0.5f)
        .radius(0.7f)
        .addStop(0.0f, Color(0x0F, 0x0C, 0x29))
        .addStop(0.5f, Color(0x30, 0x2B, 0x63))
        .addStop(1.0f, Color(0x24, 0x24, 0x3E))
        .build();
}

} // namespace NXRender
