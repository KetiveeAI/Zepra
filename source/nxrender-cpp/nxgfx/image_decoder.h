// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file image_decoder.h
 * @brief Async image decoding pipeline wrapping stb_image with proper
 * error handling, memory management, and thread pool integration.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace NXRender {

class ThreadPool;

/**
 * @brief Pixel format of decoded image.
 */
enum class ImageFormat : uint8_t {
    R8,       // Grayscale
    RG8,      // Grayscale + alpha
    RGB8,     // RGB
    RGBA8     // RGBA
};

/**
 * @brief Decoded image data.
 */
struct DecodedImage {
    int width = 0;
    int height = 0;
    int channels = 0;
    ImageFormat format = ImageFormat::RGBA8;
    std::vector<uint8_t> pixels;
    std::string error;

    bool isValid() const { return width > 0 && height > 0 && !pixels.empty(); }
    size_t sizeBytes() const { return pixels.size(); }
};

/**
 * @brief Callback for async decode completion.
 */
using DecodeCallback = std::function<void(std::unique_ptr<DecodedImage>)>;

/**
 * @brief Image decoder with sync and async modes.
 */
class ImageDecoder {
public:
    static ImageDecoder& instance();

    /**
     * @brief Decode an image from a file path (synchronous).
     * @param path File path to the image.
     * @param requestedChannels Number of channels to decode (0 = auto).
     * @return Decoded image data.
     */
    std::unique_ptr<DecodedImage> decodeFile(const std::string& path, int requestedChannels = 4);

    /**
     * @brief Decode an image from memory (synchronous).
     * @param data Pointer to image file data (PNG, JPEG, BMP, etc.).
     * @param size Size of the data in bytes.
     * @param requestedChannels Number of channels to decode (0 = auto).
     * @return Decoded image data.
     */
    std::unique_ptr<DecodedImage> decodeMemory(const uint8_t* data, size_t size, int requestedChannels = 4);

    /**
     * @brief Decode an image from a file path (asynchronous).
     * The callback is invoked on the worker thread — marshall to main thread if needed.
     */
    void decodeFileAsync(const std::string& path, DecodeCallback callback, int requestedChannels = 4);

    /**
     * @brief Decode an image from memory (asynchronous).
     * The data is copied internally — the caller can release the buffer after this call.
     */
    void decodeMemoryAsync(const uint8_t* data, size_t size, DecodeCallback callback,
                           int requestedChannels = 4);

    /**
     * @brief Query image dimensions without fully decoding.
     * @return true if successful.
     */
    bool queryDimensions(const std::string& path, int& width, int& height, int& channels);
    bool queryDimensionsMemory(const uint8_t* data, size_t size, int& width, int& height, int& channels);

    /**
     * @brief Resize an image using bilinear interpolation.
     */
    std::unique_ptr<DecodedImage> resize(const DecodedImage& src, int newWidth, int newHeight);

    /**
     * @brief Generate mipmaps for a decoded image.
     */
    std::vector<DecodedImage> generateMipmaps(const DecodedImage& src, int maxLevels = 0);

    /**
     * @brief Flip image vertically (OpenGL expects bottom-up).
     */
    static void flipVertical(DecodedImage& img);

    /**
     * @brief Premultiply alpha (for correct blending).
     */
    static void premultiplyAlpha(DecodedImage& img);

private:
    ImageDecoder() = default;
    ImageDecoder(const ImageDecoder&) = delete;
    ImageDecoder& operator=(const ImageDecoder&) = delete;

    static ImageFormat channelsToFormat(int channels);
};

} // namespace NXRender
