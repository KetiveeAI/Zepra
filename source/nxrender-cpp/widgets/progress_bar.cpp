// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "widgets/progress_bar.h"
#include "nxgfx/context.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace NXRender {

ProgressBar::ProgressBar() {
    backgroundColor_ = Color::transparent();
}

void ProgressBar::setValue(float v) {
    value_ = std::clamp(v, 0.0f, 1.0f);
    invalidate();
}

void ProgressBar::setMode(Mode m) {
    mode_ = m;
    if (m == Mode::Indeterminate) {
        animPhase_ = 0.0f;
        animStarted_ = false;
    }
    invalidate();
}

void ProgressBar::setShape(Shape s) {
    shape_ = s;
    invalidate();
}

Size ProgressBar::measure(const Size& available) {
    if (shape_ == Shape::Circular) {
        float dim = std::min(available.width, available.height);
        if (dim <= 0) dim = 48.0f;
        return Size(dim, dim);
    }
    // Linear: full width, fixed height
    float w = available.width > 0 ? available.width : 200.0f;
    return Size(w, thickness_);
}

void ProgressBar::render(GpuContext* ctx) {
    if (!ctx || !isVisible()) return;

    // Update animation phase for indeterminate mode
    if (mode_ == Mode::Indeterminate) {
        auto now = std::chrono::steady_clock::now();
        if (!animStarted_) {
            lastUpdate_ = now;
            animStarted_ = true;
        }
        float dtMs = std::chrono::duration<float, std::milli>(now - lastUpdate_).count();
        lastUpdate_ = now;
        animPhase_ += dtMs / static_cast<float>(cycleDurationMs_);
        if (animPhase_ > 1.0f) animPhase_ -= 1.0f;
    }

    if (shape_ == Shape::Linear) {
        if (mode_ == Mode::Determinate) renderLinear(ctx);
        else renderIndeterminateLinear(ctx);
    } else {
        if (mode_ == Mode::Determinate) renderCircular(ctx);
        else renderIndeterminateCircular(ctx);
    }

    renderChildren(ctx);
}

void ProgressBar::renderLinear(GpuContext* ctx) {
    const Rect& b = bounds();

    // Track
    ctx->fillRoundedRect(b, trackColor_, thickness_ * 0.5f);

    // Fill
    if (value_ > 0.0f) {
        float fillW = b.width * value_;
        Rect fillRect(b.x, b.y, fillW, b.height);
        ctx->fillRoundedRect(fillRect, fillColor_, thickness_ * 0.5f);
    }
}

void ProgressBar::renderIndeterminateLinear(GpuContext* ctx) {
    const Rect& b = bounds();

    // Track
    ctx->fillRoundedRect(b, trackColor_, thickness_ * 0.5f);

    // Sliding pulse
    // Two animations: primary bar and secondary bar
    // Primary: ease-in-out from left to right
    // Using sinusoidal wave for smooth back-and-forth
    float phase1 = animPhase_;
    float barWidth = b.width * 0.4f;

    // Primary bar position (ease-in-out sinusoidal)
    float t1 = (std::sin(phase1 * 2.0f * static_cast<float>(M_PI) - static_cast<float>(M_PI) * 0.5f) + 1.0f) * 0.5f;
    float x1 = b.x + t1 * (b.width - barWidth);
    ctx->fillRoundedRect(Rect(x1, b.y, barWidth, b.height), fillColor_, thickness_ * 0.5f);

    // Secondary bar (offset phase)
    float phase2 = animPhase_ + 0.3f;
    if (phase2 > 1.0f) phase2 -= 1.0f;
    float t2 = (std::sin(phase2 * 2.0f * static_cast<float>(M_PI) - static_cast<float>(M_PI) * 0.5f) + 1.0f) * 0.5f;
    float barWidth2 = b.width * 0.25f;
    float x2 = b.x + t2 * (b.width - barWidth2);
    ctx->fillRoundedRect(Rect(x2, b.y, barWidth2, b.height), secondaryColor_, thickness_ * 0.5f);
}

void ProgressBar::renderCircular(GpuContext* ctx) {
    const Rect& b = bounds();
    float cx = b.x + b.width * 0.5f;
    float cy = b.y + b.height * 0.5f;
    float outerRadius = std::min(b.width, b.height) * 0.5f;
    float innerRadius = outerRadius - thickness_;
    if (innerRadius < 1.0f) innerRadius = 1.0f;

    // Track circle
    ctx->strokeCircle(cx, cy, (outerRadius + innerRadius) * 0.5f, trackColor_, thickness_);

    // Arc for progress
    if (value_ > 0.0f) {
        int segments = std::max(16, static_cast<int>(value_ * 64));
        float startAngle = -static_cast<float>(M_PI) * 0.5f; // 12 o'clock
        float endAngle = startAngle + value_ * 2.0f * static_cast<float>(M_PI);
        float midRadius = (outerRadius + innerRadius) * 0.5f;

        std::vector<Point> arcPoints;
        for (int i = 0; i <= segments; i++) {
            float t = static_cast<float>(i) / static_cast<float>(segments);
            float angle = startAngle + t * (endAngle - startAngle);
            arcPoints.push_back(Point(cx + std::cos(angle) * midRadius,
                                      cy + std::sin(angle) * midRadius));
        }
        if (arcPoints.size() >= 2) {
            ctx->strokePath(arcPoints, fillColor_, thickness_, false);
        }
    }
}

void ProgressBar::renderIndeterminateCircular(GpuContext* ctx) {
    const Rect& b = bounds();
    float cx = b.x + b.width * 0.5f;
    float cy = b.y + b.height * 0.5f;
    float outerRadius = std::min(b.width, b.height) * 0.5f;
    float innerRadius = outerRadius - thickness_;
    if (innerRadius < 1.0f) innerRadius = 1.0f;

    // Track circle (dim)
    ctx->strokeCircle(cx, cy, (outerRadius + innerRadius) * 0.5f, trackColor_, thickness_);

    // Spinning arc (variable length)
    float rotationAngle = animPhase_ * 2.0f * static_cast<float>(M_PI) * 2.0f; // 2 full rotations per cycle
    float arcLength = 0.25f + 0.5f * (std::sin(animPhase_ * 2.0f * static_cast<float>(M_PI)) * 0.5f + 0.5f);
    float startAngle = rotationAngle;
    float endAngle = startAngle + arcLength * 2.0f * static_cast<float>(M_PI);
    float midRadius = (outerRadius + innerRadius) * 0.5f;

    int segments = 32;
    std::vector<Point> arcPoints;
    for (int i = 0; i <= segments; i++) {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        float angle = startAngle + t * (endAngle - startAngle);
        arcPoints.push_back(Point(cx + std::cos(angle) * midRadius,
                                  cy + std::sin(angle) * midRadius));
    }
    if (arcPoints.size() >= 2) {
        ctx->strokePath(arcPoints, fillColor_, thickness_, false);
    }
}

} // namespace NXRender
