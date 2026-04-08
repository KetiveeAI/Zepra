// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file progress_bar.h
 * @brief Determinate and indeterminate progress bar widget with circular variant.
 */

#pragma once

#include "widget.h"
#include "../animation/animator.h"

namespace NXRender {

class ProgressBar : public Widget {
public:
    enum class Mode { Determinate, Indeterminate };
    enum class Shape { Linear, Circular };

    ProgressBar();

    // Value (0.0 - 1.0 for determinate mode)
    float value() const { return value_; }
    void setValue(float value);

    // Mode
    Mode mode() const { return mode_; }
    void setMode(Mode mode);

    // Shape
    Shape shape() const { return shape_; }
    void setShape(Shape shape);

    // Colors
    void setTrackColor(const Color& color) { trackColor_ = color; }
    void setFillColor(const Color& color) { fillColor_ = color; }
    void setSecondaryColor(const Color& color) { secondaryColor_ = color; }

    // Sizing
    void setThickness(float thickness) { thickness_ = thickness; }
    float thickness() const { return thickness_; }

    // Animation speed for indeterminate mode (ms for one cycle)
    void setCycleDuration(int ms) { cycleDurationMs_ = ms; }

    // Rendering
    void render(GpuContext* ctx) override;
    Size measure(const Size& available) override;

private:
    void renderLinear(GpuContext* ctx);
    void renderCircular(GpuContext* ctx);
    void renderIndeterminateLinear(GpuContext* ctx);
    void renderIndeterminateCircular(GpuContext* ctx);

    float value_ = 0.0f;
    Mode mode_ = Mode::Determinate;
    Shape shape_ = Shape::Linear;

    Color trackColor_ = Color(0xE0E0E0);
    Color fillColor_ = Color(0x2196F3);
    Color secondaryColor_ = Color(0x90CAF9);

    float thickness_ = 4.0f;
    int cycleDurationMs_ = 2000;

    // Indeterminate animation state
    float animPhase_ = 0.0f;
    std::chrono::steady_clock::time_point lastUpdate_;
    bool animStarted_ = false;
};

} // namespace NXRender
