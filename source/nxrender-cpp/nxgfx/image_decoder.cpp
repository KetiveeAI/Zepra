// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "nxgfx/image_decoder.h"
#include "platform/thread_pool.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

// stb_image (vendored)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO // We handle file I/O ourselves for better error reporting
#include "../../integration/stb_image.h"

namespace NXRender {

ImageDecoder& ImageDecoder::instance() {
    static ImageDecoder decoder;
    return decoder;
}

ImageFormat ImageDecoder::channelsToFormat(int channels) {
    switch (channels) {
        case 1: return ImageFormat::R8;
        case 2: return ImageFormat::RG8;
        case 3: return ImageFormat::RGB8;
        case 4: return ImageFormat::RGBA8;
        default: return ImageFormat::RGBA8;
    }
}

std::unique_ptr<DecodedImage> ImageDecoder::decodeFile(const std::string& path, int requestedChannels) {
    auto result = std::make_unique<DecodedImage>();

    // Read file into memory
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        result->error = "Failed to open file: " + path;
        return result;
    }

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fileSize <= 0 || fileSize > 256 * 1024 * 1024) { // 256 MB limit
        result->error = "Invalid file size: " + std::to_string(fileSize);
        fclose(f);
        return result;
    }

    std::vector<uint8_t> fileData(static_cast<size_t>(fileSize));
    size_t bytesRead = fread(fileData.data(), 1, static_cast<size_t>(fileSize), f);
    fclose(f);

    if (bytesRead != static_cast<size_t>(fileSize)) {
        result->error = "Failed to read complete file: " + path;
        return result;
    }

    return decodeMemory(fileData.data(), fileData.size(), requestedChannels);
}

std::unique_ptr<DecodedImage> ImageDecoder::decodeMemory(const uint8_t* data, size_t size,
                                                          int requestedChannels) {
    auto result = std::make_unique<DecodedImage>();

    if (!data || size == 0) {
        result->error = "Null or empty data";
        return result;
    }

    int width, height, channels;
    stbi_uc* pixels = stbi_load_from_memory(data, static_cast<int>(size),
                                             &width, &height, &channels,
                                             requestedChannels);

    if (!pixels) {
        result->error = "stb_image decode failed: ";
        const char* reason = stbi_failure_reason();
        if (reason) result->error += reason;
        return result;
    }

    int actualChannels = (requestedChannels > 0) ? requestedChannels : channels;
    size_t pixelDataSize = static_cast<size_t>(width) * height * actualChannels;

    result->width = width;
    result->height = height;
    result->channels = actualChannels;
    result->format = channelsToFormat(actualChannels);
    result->pixels.assign(pixels, pixels + pixelDataSize);

    stbi_image_free(pixels);

    return result;
}

void ImageDecoder::decodeFileAsync(const std::string& path, DecodeCallback callback,
                                    int requestedChannels) {
    ThreadPool::instance().submitVoid([this, path, callback, requestedChannels]() {
        auto result = decodeFile(path, requestedChannels);
        if (callback) callback(std::move(result));
    });
}

void ImageDecoder::decodeMemoryAsync(const uint8_t* data, size_t size, DecodeCallback callback,
                                      int requestedChannels) {
    // Copy data since the caller may release it
    auto dataCopy = std::make_shared<std::vector<uint8_t>>(data, data + size);

    ThreadPool::instance().submitVoid([this, dataCopy, callback, requestedChannels]() {
        auto result = decodeMemory(dataCopy->data(), dataCopy->size(), requestedChannels);
        if (callback) callback(std::move(result));
    });
}

bool ImageDecoder::queryDimensions(const std::string& path, int& width, int& height, int& channels) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fileSize <= 0) { fclose(f); return false; }

    // Read enough for header (first 8KB should be enough for any format)
    size_t headerSize = std::min(static_cast<size_t>(fileSize), static_cast<size_t>(8192));
    std::vector<uint8_t> header(headerSize);
    fread(header.data(), 1, headerSize, f);
    fclose(f);

    return queryDimensionsMemory(header.data(), headerSize, width, height, channels);
}

bool ImageDecoder::queryDimensionsMemory(const uint8_t* data, size_t size,
                                          int& width, int& height, int& channels) {
    return stbi_info_from_memory(data, static_cast<int>(size), &width, &height, &channels) != 0;
}

std::unique_ptr<DecodedImage> ImageDecoder::resize(const DecodedImage& src, int newWidth, int newHeight) {
    auto result = std::make_unique<DecodedImage>();
    if (!src.isValid() || newWidth <= 0 || newHeight <= 0) {
        result->error = "Invalid resize parameters";
        return result;
    }

    result->width = newWidth;
    result->height = newHeight;
    result->channels = src.channels;
    result->format = src.format;
    result->pixels.resize(static_cast<size_t>(newWidth) * newHeight * src.channels);

    float xRatio = static_cast<float>(src.width) / static_cast<float>(newWidth);
    float yRatio = static_cast<float>(src.height) / static_cast<float>(newHeight);

    for (int y = 0; y < newHeight; y++) {
        float srcY = static_cast<float>(y) * yRatio;
        int y0 = static_cast<int>(srcY);
        int y1 = std::min(y0 + 1, src.height - 1);
        float fy = srcY - static_cast<float>(y0);

        for (int x = 0; x < newWidth; x++) {
            float srcX = static_cast<float>(x) * xRatio;
            int x0 = static_cast<int>(srcX);
            int x1 = std::min(x0 + 1, src.width - 1);
            float fx = srcX - static_cast<float>(x0);

            for (int c = 0; c < src.channels; c++) {
                // Bilinear interpolation
                float tl = static_cast<float>(src.pixels[(y0 * src.width + x0) * src.channels + c]);
                float tr = static_cast<float>(src.pixels[(y0 * src.width + x1) * src.channels + c]);
                float bl = static_cast<float>(src.pixels[(y1 * src.width + x0) * src.channels + c]);
                float br = static_cast<float>(src.pixels[(y1 * src.width + x1) * src.channels + c]);

                float top = tl + (tr - tl) * fx;
                float bottom = bl + (br - bl) * fx;
                float value = top + (bottom - top) * fy;

                result->pixels[(y * newWidth + x) * src.channels + c] =
                    static_cast<uint8_t>(std::clamp(value, 0.0f, 255.0f));
            }
        }
    }

    return result;
}

std::vector<DecodedImage> ImageDecoder::generateMipmaps(const DecodedImage& src, int maxLevels) {
    std::vector<DecodedImage> mipmaps;
    if (!src.isValid()) return mipmaps;

    int width = src.width;
    int height = src.height;
    const DecodedImage* current = &src;

    int level = 0;
    while (width > 1 && height > 1) {
        if (maxLevels > 0 && level >= maxLevels) break;

        int newWidth = std::max(1, width / 2);
        int newHeight = std::max(1, height / 2);

        auto mip = resize(*current, newWidth, newHeight);
        if (!mip->isValid()) break;

        mipmaps.push_back(std::move(*mip));
        current = &mipmaps.back();
        width = newWidth;
        height = newHeight;
        level++;
    }

    return mipmaps;
}

void ImageDecoder::flipVertical(DecodedImage& img) {
    if (!img.isValid()) return;

    int rowSize = img.width * img.channels;
    std::vector<uint8_t> temp(rowSize);

    for (int y = 0; y < img.height / 2; y++) {
        uint8_t* top = &img.pixels[y * rowSize];
        uint8_t* bottom = &img.pixels[(img.height - 1 - y) * rowSize];
        memcpy(temp.data(), top, rowSize);
        memcpy(top, bottom, rowSize);
        memcpy(bottom, temp.data(), rowSize);
    }
}

void ImageDecoder::premultiplyAlpha(DecodedImage& img) {
    if (!img.isValid() || img.channels != 4) return;

    size_t pixelCount = static_cast<size_t>(img.width) * img.height;
    for (size_t i = 0; i < pixelCount; i++) {
        uint8_t* p = &img.pixels[i * 4];
        float a = p[3] / 255.0f;
        p[0] = static_cast<uint8_t>(p[0] * a);
        p[1] = static_cast<uint8_t>(p[1] * a);
        p[2] = static_cast<uint8_t>(p[2] * a);
    }
}

} // namespace NXRender
