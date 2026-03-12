// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file surface.cpp
 * @brief Drawing surface implementation
 */

#include "core/surface.h"
#include <cstring>

namespace NXRender {

static int bytesPerPixel(PixelFormat format) {
    switch (format) {
        case PixelFormat::RGBA8:
        case PixelFormat::BGRA8:
            return 4;
        case PixelFormat::RGB8:
            return 3;
        case PixelFormat::Alpha8:
            return 1;
    }
    return 4;
}

Surface::Surface(int width, int height, PixelFormat format)
    : width_(width)
    , height_(height)
    , format_(format) {
    stride_ = width * bytesPerPixel(format);
    pixels_ = std::make_unique<uint8_t[]>(stride_ * height);
    clear();
}

Surface::~Surface() = default;

void Surface::clear(uint32_t color) {
    if (format_ == PixelFormat::RGBA8 || format_ == PixelFormat::BGRA8) {
        uint32_t* p = reinterpret_cast<uint32_t*>(pixels_.get());
        for (int i = 0; i < width_ * height_; i++) {
            p[i] = color;
        }
    } else {
        std::memset(pixels_.get(), 0, stride_ * height_);
    }
}

void Surface::resize(int width, int height) {
    if (width == width_ && height == height_) return;
    
    width_ = width;
    height_ = height;
    stride_ = width * bytesPerPixel(format_);
    pixels_ = std::make_unique<uint8_t[]>(stride_ * height);
    clear();
}

} // namespace NXRender
