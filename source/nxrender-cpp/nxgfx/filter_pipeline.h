// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file filter_pipeline.h
 * @brief GPU-accelerated image filter pipeline.
 *
 * Filters operate on render target FBOs. They're chained in order
 * and applied during the composite phase.
 */

#pragma once

#include "primitives.h"
#include "color.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <string>
#include <functional>

namespace NXRender {

class GpuContext;
class ShaderProgram;

enum class FilterType : uint8_t {
    GaussianBlur,
    BoxBlur,
    DropShadow,
    InnerShadow,
    Brightness,
    Contrast,
    Saturate,
    HueRotate,
    Invert,
    Grayscale,
    Sepia,
    Opacity,
    ColorMatrix,
    Custom
};

/**
 * @brief Base filter configuration.
 */
struct FilterConfig {
    FilterType type;
    float parameters[16] = {};  // Meaning depends on type
    int paramCount = 0;
    std::string customShader;   // For FilterType::Custom

    static FilterConfig gaussianBlur(float radius);
    static FilterConfig boxBlur(float radius);
    static FilterConfig dropShadow(float offsetX, float offsetY, float blur, const Color& color);
    static FilterConfig innerShadow(float offsetX, float offsetY, float blur, const Color& color);
    static FilterConfig brightness(float factor);
    static FilterConfig contrast(float factor);
    static FilterConfig saturate(float factor);
    static FilterConfig hueRotate(float degrees);
    static FilterConfig invert(float amount = 1.0f);
    static FilterConfig grayscale(float amount = 1.0f);
    static FilterConfig sepia(float amount = 1.0f);
    static FilterConfig opacity(float amount);
    static FilterConfig colorMatrix(const float matrix[20]);
};

/**
 * @brief A chain of filters to apply in sequence.
 */
class FilterPipeline {
public:
    FilterPipeline();
    ~FilterPipeline();

    FilterPipeline(const FilterPipeline&) = delete;
    FilterPipeline& operator=(const FilterPipeline&) = delete;

    /**
     * @brief Initialize GPU resources for filtering.
     */
    bool init(GpuContext* ctx);
    void shutdown();

    /**
     * @brief Add a filter to the chain.
     */
    void addFilter(const FilterConfig& filter);
    void clearFilters();
    size_t filterCount() const { return filters_.size(); }

    /**
     * @brief Apply all filters to the source texture.
     * @param sourceTexture Input texture ID.
     * @param width Texture width.
     * @param height Texture height.
     * @return Result texture ID (may be a new texture or the source if no filters).
     */
    uint32_t apply(uint32_t sourceTexture, int width, int height);

    /**
     * @brief Apply a single Gaussian blur pass.
     */
    uint32_t applyGaussianBlur(uint32_t sourceTexture, int width, int height,
                                float radius, bool horizontal);

    /**
     * @brief Apply a 4x5 color matrix transformation.
     */
    uint32_t applyColorMatrix(uint32_t sourceTexture, int width, int height,
                               const float matrix[20]);

    /**
     * @brief Apply brightness/contrast adjustment.
     */
    uint32_t applyBrightnessContrast(uint32_t sourceTexture, int width, int height,
                                      float brightness, float contrast);

private:
    struct PingPongFBO {
        uint32_t fbo[2] = {};
        uint32_t texture[2] = {};
        int width = 0, height = 0;
        int current = 0;

        uint32_t currentTexture() const { return texture[current]; }
        uint32_t currentFBO() const { return fbo[current]; }
        void swap() { current = 1 - current; }
    };

    bool ensurePingPong(int width, int height);
    void renderFullscreenQuad();
    void computeGaussianKernel(float radius, std::vector<float>& weights,
                                std::vector<float>& offsets, int& sampleCount);

    GpuContext* ctx_ = nullptr;
    PingPongFBO pingPong_;
    uint32_t fullscreenVAO_ = 0;
    uint32_t fullscreenVBO_ = 0;

    std::vector<FilterConfig> filters_;

    // Cached shaders
    ShaderProgram* blurShader_ = nullptr;
    ShaderProgram* colorMatrixShader_ = nullptr;
    ShaderProgram* adjustShader_ = nullptr;
    ShaderProgram* passthrough_ = nullptr;
};

} // namespace NXRender
