// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file display_list.h
 * @brief Retained-mode display list for recording and replaying draw commands.
 *
 * Draw commands are recorded into a flat buffer during layout/paint traversal,
 * then replayed against GpuContext during compositing. This decouples paint
 * from rasterization and enables:
 * - Deferred rendering (paint on any thread, replay on GPU thread)
 * - Hit testing without re-painting
 * - Display list caching for unchanged layers
 */

#pragma once

#include "../nxgfx/primitives.h"
#include "../nxgfx/color.h"
#include "memory_pool.h"
#include <cstdint>
#include <vector>
#include <string>

namespace NXRender {

class GpuContext;

enum class DrawCommand : uint8_t {
    FillRect,
    StrokeRect,
    FillRoundedRect,
    StrokeRoundedRect,
    FillCircle,
    StrokeCircle,
    DrawLine,
    DrawText,
    DrawTexture,
    DrawTextureRegion,
    FillRectGradient,
    DrawShadow,
    FillPath,
    StrokePath,
    FillComplexPath,
    PushClip,
    PopClip,
    PushTransform,
    PopTransform,
    Translate,
    Scale,
    Rotate,
    SetBlendMode,
    Clear,
    SetRenderTarget,
    Nop
};

/**
 * @brief Path data stored inline in the display list.
 */
struct PathData {
    std::vector<Point> points;
    bool closed = false;
};

struct ComplexPathData {
    std::vector<std::vector<Point>> contours;
    std::string fillRule; // "nonzero" or "evenodd"
};

/**
 * @brief Single display list entry.
 *
 * Uses a tagged union approach — only the fields relevant to the command
 * type are meaningful. Kept as a struct-of-fields (not a variant) because
 * the size difference between commands is small and the hot path is linear
 * iteration.
 */
struct DisplayListEntry {
    DrawCommand command = DrawCommand::Nop;

    // Geometry
    Rect rect;
    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    float radius = 0;
    CornerRadii cornerRadii;
    float lineWidth = 1.0f;

    // Color
    Color color;
    Color endColor; // For gradients

    // Text
    std::string text;
    float fontSize = 14.0f;

    // Texture
    uint32_t textureId = 0;
    Rect srcRect; // For sub-texture drawing

    // Transform
    float tx = 0, ty = 0;
    float sx = 1, sy = 1;
    float angle = 0;

    // Shadow
    float blur = 0;
    float offsetX = 0, offsetY = 0;

    // Gradient direction
    bool horizontal = true;

    // Blend mode (stored as int to avoid circular header dependency)
    int blendMode = 0;

    // Render target
    uint32_t renderTargetId = 0;

    // Path (heap-allocated, only for path commands)
    PathData* pathData = nullptr;
    ComplexPathData* complexPathData = nullptr;
};

/**
 * @brief Retained-mode command buffer.
 *
 * Record draw operations during paint, replay against GpuContext during
 * compositing. Entries are stored flat in a vector for linear iteration.
 */
class DisplayList {
public:
    DisplayList();
    ~DisplayList();

    DisplayList(DisplayList&&) noexcept;
    DisplayList& operator=(DisplayList&&) noexcept;

    // Non-copyable (owns heap-allocated path data)
    DisplayList(const DisplayList&) = delete;
    DisplayList& operator=(const DisplayList&) = delete;

    // ======================================================================
    // Recording API — mirrors GpuContext interface
    // ======================================================================

    void fillRect(const Rect& rect, const Color& color);
    void strokeRect(const Rect& rect, const Color& color, float lineWidth = 1.0f);
    void fillRoundedRect(const Rect& rect, const Color& color, float radius);
    void fillRoundedRect(const Rect& rect, const Color& color, const CornerRadii& radii);
    void strokeRoundedRect(const Rect& rect, const Color& color, float radius, float lineWidth = 1.0f);

    void fillCircle(float cx, float cy, float radius, const Color& color);
    void strokeCircle(float cx, float cy, float radius, const Color& color, float lineWidth = 1.0f);

    void drawLine(float x1, float y1, float x2, float y2, const Color& color, float lineWidth = 1.0f);

    void drawText(const std::string& text, float x, float y, const Color& color, float fontSize = 14.0f);
    void drawTexture(uint32_t texture, const Rect& dest);
    void drawTextureRegion(uint32_t texture, const Rect& src, const Rect& dest);

    void fillRectGradient(const Rect& rect, const Color& start, const Color& end, bool horizontal = true);
    void drawShadow(const Rect& rect, const Color& color, float blur, float offsetX = 0, float offsetY = 0);

    void fillPath(const std::vector<Point>& points, const Color& color);
    void strokePath(const std::vector<Point>& points, const Color& color, float lineWidth = 1.0f, bool closed = false);
    void fillComplexPath(const std::vector<std::vector<Point>>& contours, const Color& color,
                         const std::string& rule = "nonzero");

    void pushClip(const Rect& rect);
    void popClip();
    void pushTransform();
    void popTransform();
    void translate(float x, float y);
    void scale(float sx, float sy);
    void rotate(float radians);
    void setBlendMode(int mode);
    void clear(const Color& color);
    void setRenderTarget(uint32_t targetId);

    // ======================================================================
    // Playback
    // ======================================================================

    /**
     * @brief Replay all recorded commands against a GpuContext.
     */
    void replay(GpuContext* ctx) const;

    // ======================================================================
    // State
    // ======================================================================

    size_t entryCount() const { return entries_.size(); }
    bool empty() const { return entries_.empty(); }
    void clear();

    /**
     * @brief Approximate GPU-side bounding rect of all draw commands.
     */
    Rect bounds() const { return bounds_; }

private:
    DisplayListEntry& append(DrawCommand cmd);
    void expandBounds(const Rect& rect);

    std::vector<DisplayListEntry> entries_;
    Rect bounds_;

    // Heap-allocated path data owned by this display list
    std::vector<PathData*> ownedPaths_;
    std::vector<ComplexPathData*> ownedComplexPaths_;
};

} // namespace NXRender
