// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file batch_renderer.h
 * @brief Batched geometry submission to reduce draw calls.
 *
 * Accumulates quads/triangles and flushes on texture change, blend mode
 * change, or buffer full. Single VBO with dynamic sub-allocation.
 */

#pragma once

#include "../nxgfx/primitives.h"
#include "../nxgfx/color.h"
#include <cstdint>
#include <vector>

namespace NXRender {

class GpuContext;
class ShaderProgram;

/**
 * @brief Vertex format for the batch renderer.
 * 20 bytes per vertex.
 */
struct BatchVertex {
    float x, y;           // Position (8 bytes)
    float u, v;           // Texture coords (8 bytes)
    uint32_t colorPacked; // RGBA packed (4 bytes)

    BatchVertex() : x(0), y(0), u(0), v(0), colorPacked(0xFFFFFFFF) {}
    BatchVertex(float px, float py, float tu, float tv, const Color& c)
        : x(px), y(py), u(tu), v(tv)
        , colorPacked(static_cast<uint32_t>(c.r) |
                      (static_cast<uint32_t>(c.g) << 8) |
                      (static_cast<uint32_t>(c.b) << 16) |
                      (static_cast<uint32_t>(c.a) << 24)) {}
};

/**
 * @brief Reason a batch was flushed.
 */
enum class BatchBreakReason : uint8_t {
    None,
    TextureChange,
    BlendModeChange,
    ShaderChange,
    ClipRectChange,
    BufferFull,
    ManualFlush,
    EndOfFrame
};

/**
 * @brief Batch rendering statistics.
 */
struct BatchStats {
    uint32_t drawCalls = 0;
    uint32_t vertexCount = 0;
    uint32_t triangleCount = 0;
    uint32_t batchBreaks = 0;
    uint32_t textureChanges = 0;
    uint32_t blendModeChanges = 0;
    uint32_t bufferFullFlushes = 0;
};

/**
 * @brief Batched geometry renderer.
 *
 * Usage:
 *   batch.begin();
 *   batch.setTexture(texId);
 *   batch.pushQuad(x, y, w, h, color);
 *   batch.pushQuad(x2, y2, w2, h2, color2);
 *   batch.end(); // Flushes remaining geometry
 */
class BatchRenderer {
public:
    static constexpr size_t kMaxVertices = 16384;  // 320 KB vertex buffer
    static constexpr size_t kMaxQuads = kMaxVertices / 4;

    BatchRenderer();
    ~BatchRenderer();

    BatchRenderer(const BatchRenderer&) = delete;
    BatchRenderer& operator=(const BatchRenderer&) = delete;

    /**
     * @brief Initialize GL resources (VBO, IBO).
     */
    bool init();

    /**
     * @brief Release GL resources.
     */
    void shutdown();

    /**
     * @brief Begin a batch frame.
     */
    void begin(int viewportWidth, int viewportHeight);

    /**
     * @brief End the batch frame. Flushes remaining geometry.
     */
    void end();

    /**
     * @brief Force a flush of the current batch.
     */
    void flush();

    // ==================================================================
    // State setters (trigger batch break if different from current)
    // ==================================================================

    void setTexture(uint32_t textureId);
    void setShader(ShaderProgram* shader);
    void setBlendMode(int mode);
    void setClipRect(const Rect& rect);
    void clearClipRect();

    // ==================================================================
    // Geometry submission
    // ==================================================================

    /**
     * @brief Push a colored quad (no texture).
     */
    void pushQuad(float x, float y, float w, float h, const Color& color);

    /**
     * @brief Push a textured quad.
     */
    void pushTexturedQuad(float x, float y, float w, float h,
                          float u0, float v0, float u1, float v1,
                          const Color& tint = Color(255, 255, 255));

    /**
     * @brief Push a single triangle.
     */
    void pushTriangle(float x0, float y0, float x1, float y1, float x2, float y2,
                      const Color& color);

    /**
     * @brief Push raw vertices (must be multiple of 3 for triangles).
     */
    void pushVertices(const BatchVertex* vertices, size_t count);

    // ==================================================================
    // Stats
    // ==================================================================

    const BatchStats& stats() const { return stats_; }
    void resetStats();

private:
    void internalFlush();
    void ensureCapacity(size_t vertexCount);
    void buildProjectionMatrix(float width, float height, float* matrix);

    uint32_t vbo_ = 0;
    uint32_t ibo_ = 0;
    uint32_t vao_ = 0;

    std::vector<BatchVertex> vertices_;
    size_t vertexCount_ = 0;

    // Current state
    uint32_t currentTexture_ = 0;
    ShaderProgram* currentShader_ = nullptr;
    int currentBlendMode_ = 0;
    Rect currentClipRect_;
    bool hasClipRect_ = false;

    int viewportWidth_ = 0;
    int viewportHeight_ = 0;
    bool active_ = false;

    BatchStats stats_;
};

} // namespace NXRender
