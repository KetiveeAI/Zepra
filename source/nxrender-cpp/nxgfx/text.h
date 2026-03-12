// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file text.h
 * @brief Text rendering for NXRender
 */

#pragma once

#include "color.h"
#include "primitives.h"
#include <string>
#include <memory>

namespace NXRender {

/**
 * @brief Font style
 */
struct FontStyle {
    std::string family = "sans-serif";
    float size = 14.0f;
    int weight = 400;  // 100-900
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;
};

/**
 * @brief Text metrics
 */
struct TextMetrics {
    float width = 0;
    float height = 0;
    float ascent = 0;
    float descent = 0;
    float lineHeight = 0;
};

/**
 * @brief Text renderer (uses FreeType internally)
 */
class TextRenderer {
public:
    static TextRenderer& instance();
    
    // Font loading
    bool loadFont(const std::string& path, const std::string& name);
    void setDefaultFont(const std::string& name);
    
    // Measurement
    TextMetrics measure(const std::string& text, const FontStyle& style);
    float measureWidth(const std::string& text, float fontSize);
    
    // Rendering (called through GpuContext)
    void render(const std::string& text, float x, float y, 
                const Color& color, const FontStyle& style);
    
private:
    TextRenderer();
    ~TextRenderer();
    
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace NXRender
