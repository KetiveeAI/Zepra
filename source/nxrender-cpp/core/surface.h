// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file surface.h
 * @brief Drawing surfaces for compositor
 */

#pragma once

#include "../nxgfx/primitives.h"
#include <cstdint>
#include <memory>

namespace NXRender {

/**
 * @brief Pixel format
 */
enum class PixelFormat {
    RGBA8,
    BGRA8,
    RGB8,
    Alpha8
};

/**
 * @brief Drawing surface (off-screen buffer)
 */
class Surface {
public:
    Surface(int width, int height, PixelFormat format = PixelFormat::RGBA8);
    ~Surface();
    
    // Properties
    int width() const { return width_; }
    int height() const { return height_; }
    Size size() const { return Size(static_cast<float>(width_), static_cast<float>(height_)); }
    PixelFormat format() const { return format_; }
    
    // Pixel access
    uint8_t* pixels() { return pixels_.get(); }
    const uint8_t* pixels() const { return pixels_.get(); }
    int stride() const { return stride_; }
    
    // Clear
    void clear(uint32_t color = 0);
    
    // Resize
    void resize(int width, int height);
    
private:
    int width_;
    int height_;
    int stride_;
    PixelFormat format_;
    std::unique_ptr<uint8_t[]> pixels_;
};

} // namespace NXRender
