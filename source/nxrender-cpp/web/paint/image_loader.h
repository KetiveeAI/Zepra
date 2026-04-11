// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace NXRender {

// ==================================================================
// Image format detection and metadata
// ==================================================================

enum class ImageFormat : uint8_t {
    Unknown, PNG, JPEG, WebP, AVIF, GIF, SVG, ICO, BMP, TIFF
};

struct ImageMetadata {
    int width = 0, height = 0;
    int channels = 0;       // 1=gray, 2=gray+alpha, 3=RGB, 4=RGBA
    int bitDepth = 8;
    bool animated = false;
    int frameCount = 1;
    float duration = 0;     // Total animation duration (seconds)
    ImageFormat format = ImageFormat::Unknown;
    std::string mimeType;
    size_t fileSize = 0;

    // EXIF orientation (1-8)
    int orientation = 1;

    // Color space
    enum class ColorSpace { sRGB, DisplayP3, AdobeRGB, Unknown } colorSpace = ColorSpace::sRGB;
};

// ==================================================================
// Decoded image data
// ==================================================================

struct DecodedImage {
    std::vector<uint8_t> pixels;  // RGBA interleaved
    int width = 0, height = 0;
    int stride = 0;               // Bytes per row
    ImageFormat sourceFormat = ImageFormat::Unknown;

    bool valid() const { return width > 0 && height > 0 && !pixels.empty(); }
    size_t sizeBytes() const { return pixels.size(); }
};

struct AnimationFrame {
    DecodedImage image;
    float delay = 0;              // Seconds until next frame
    int disposeOp = 0;            // 0=none, 1=background, 2=previous
    int blendOp = 0;              // 0=source, 1=over
    int x = 0, y = 0;            // Frame position within canvas
};

// ==================================================================
// Image decoder interface
// ==================================================================

class ImageDecoder {
public:
    virtual ~ImageDecoder() = default;

    virtual bool canDecode(ImageFormat format) const = 0;
    virtual bool canDecode(const uint8_t* data, size_t size) const = 0;

    // Probe metadata without full decode
    virtual ImageMetadata probe(const uint8_t* data, size_t size) const = 0;

    // Full decode
    virtual DecodedImage decode(const uint8_t* data, size_t size) const = 0;

    // Decode at specific size (for responsive images)
    virtual DecodedImage decodeAtSize(const uint8_t* data, size_t size,
                                       int targetWidth, int targetHeight) const;

    // Animated image decode
    virtual std::vector<AnimationFrame> decodeAnimation(const uint8_t* data, size_t size) const;

    // Progressive decode support
    virtual bool supportsProgressive() const { return false; }
    virtual DecodedImage decodeProgressive(const uint8_t* data, size_t size,
                                            int pass) const;
};

// ==================================================================
// Built-in decoders
// ==================================================================

class PNGDecoder : public ImageDecoder {
public:
    bool canDecode(ImageFormat format) const override { return format == ImageFormat::PNG; }
    bool canDecode(const uint8_t* data, size_t size) const override;
    ImageMetadata probe(const uint8_t* data, size_t size) const override;
    DecodedImage decode(const uint8_t* data, size_t size) const override;
    bool supportsProgressive() const override { return true; }
    DecodedImage decodeProgressive(const uint8_t* data, size_t size, int pass) const override;
};

class JPEGDecoder : public ImageDecoder {
public:
    bool canDecode(ImageFormat format) const override { return format == ImageFormat::JPEG; }
    bool canDecode(const uint8_t* data, size_t size) const override;
    ImageMetadata probe(const uint8_t* data, size_t size) const override;
    DecodedImage decode(const uint8_t* data, size_t size) const override;
    DecodedImage decodeAtSize(const uint8_t* data, size_t size,
                               int targetWidth, int targetHeight) const override;
    bool supportsProgressive() const override { return true; }
};

class WebPDecoder : public ImageDecoder {
public:
    bool canDecode(ImageFormat format) const override { return format == ImageFormat::WebP; }
    bool canDecode(const uint8_t* data, size_t size) const override;
    ImageMetadata probe(const uint8_t* data, size_t size) const override;
    DecodedImage decode(const uint8_t* data, size_t size) const override;
    std::vector<AnimationFrame> decodeAnimation(const uint8_t* data, size_t size) const override;
};

class GIFDecoder : public ImageDecoder {
public:
    bool canDecode(ImageFormat format) const override { return format == ImageFormat::GIF; }
    bool canDecode(const uint8_t* data, size_t size) const override;
    ImageMetadata probe(const uint8_t* data, size_t size) const override;
    DecodedImage decode(const uint8_t* data, size_t size) const override;
    std::vector<AnimationFrame> decodeAnimation(const uint8_t* data, size_t size) const override;
};

class AVIFDecoder : public ImageDecoder {
public:
    bool canDecode(ImageFormat format) const override { return format == ImageFormat::AVIF; }
    bool canDecode(const uint8_t* data, size_t size) const override;
    ImageMetadata probe(const uint8_t* data, size_t size) const override;
    DecodedImage decode(const uint8_t* data, size_t size) const override;
};

// ==================================================================
// Image loading states
// ==================================================================

enum class ImageLoadState : uint8_t {
    Idle, Loading, HeaderDecoded, Decoding, Complete, Error
};

// ==================================================================
// Image loading request
// ==================================================================

struct ImageLoadRequest {
    std::string url;
    std::string srcset;           // For responsive selection
    std::string sizes;            // Media-based size hints
    int layoutWidth = 0;          // Layout-computed width
    int layoutHeight = 0;
    float devicePixelRatio = 1.0f;
    bool lazy = false;            // loading="lazy"
    int priority = 0;             // 0=normal, 1=high, -1=low

    enum class Decoding { Auto, Sync, Async } decoding = Decoding::Auto;
    enum class FetchPriority { Auto, High, Low } fetchPriority = FetchPriority::Auto;
};

// ==================================================================
// Image resource — loaded/decoded image
// ==================================================================

class ImageResource {
public:
    ImageResource(const std::string& url);
    ~ImageResource();

    const std::string& url() const { return url_; }
    ImageLoadState state() const { return state_; }
    const ImageMetadata& metadata() const { return metadata_; }
    const DecodedImage& decoded() const { return decoded_; }

    // Texture handle (set by renderer after GPU upload)
    void setTextureId(uint32_t id) { textureId_ = id; }
    uint32_t textureId() const { return textureId_; }
    bool hasTexture() const { return textureId_ != 0; }

    // Animation support
    bool isAnimated() const { return frames_.size() > 1; }
    size_t frameCount() const { return frames_.size(); }
    const AnimationFrame* frame(size_t index) const;
    size_t currentFrame() const { return currentFrame_; }
    void advanceFrame(float deltaMs);

    // Natural size (from metadata or intrinsic)
    int naturalWidth() const { return metadata_.width; }
    int naturalHeight() const { return metadata_.height; }

    // Error info
    const std::string& errorMessage() const { return errorMessage_; }

    // Callbacks
    using LoadCallback = std::function<void(ImageResource*)>;
    void onLoad(LoadCallback cb) { onLoad_ = cb; }
    void onError(LoadCallback cb) { onError_ = cb; }

    // Internal state transitions
    void setHeaderData(const ImageMetadata& meta);
    void setDecodedData(DecodedImage&& decoded);
    void setAnimationFrames(std::vector<AnimationFrame>&& frames);
    void setError(const std::string& message);

private:
    std::string url_;
    ImageLoadState state_ = ImageLoadState::Idle;
    ImageMetadata metadata_;
    DecodedImage decoded_;
    std::vector<AnimationFrame> frames_;
    uint32_t textureId_ = 0;
    size_t currentFrame_ = 0;
    float frameTimer_ = 0;
    std::string errorMessage_;

    LoadCallback onLoad_;
    LoadCallback onError_;
};

// ==================================================================
// Image cache
// ==================================================================

class ImageCache {
public:
    static ImageCache& instance();

    // Lookup or create
    std::shared_ptr<ImageResource> get(const std::string& url);
    void put(const std::string& url, std::shared_ptr<ImageResource> resource);
    void remove(const std::string& url);
    void clear();

    // Memory management
    size_t memorySizeBytes() const;
    void setMaxMemory(size_t bytes) { maxMemory_ = bytes; }
    void evictLRU();

    // Stats
    size_t hitCount() const { return hits_; }
    size_t missCount() const { return misses_; }
    size_t entryCount() const;

private:
    ImageCache() = default;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<ImageResource>> cache_;
    std::vector<std::string> lruOrder_;
    size_t maxMemory_ = 256 * 1024 * 1024; // 256MB default
    size_t hits_ = 0, misses_ = 0;
};

// ==================================================================
// Srcset / sizes resolver
// ==================================================================

struct SrcsetCandidate {
    std::string url;
    float widthDescriptor = 0;   // w descriptor
    float pixelDensity = 0;      // x descriptor
};

class SrcsetResolver {
public:
    static std::vector<SrcsetCandidate> parse(const std::string& srcset);

    static std::string selectBest(const std::vector<SrcsetCandidate>& candidates,
                                    int layoutWidth, float devicePixelRatio);

    // Parse sizes attribute
    static int parseSizes(const std::string& sizes, int viewportWidth);
};

// ==================================================================
// Image format detection
// ==================================================================

class ImageFormatDetector {
public:
    static ImageFormat detect(const uint8_t* data, size_t size);
    static std::string mimeType(ImageFormat format);
    static ImageFormat fromMimeType(const std::string& mime);
    static ImageFormat fromExtension(const std::string& ext);
};

// ==================================================================
// Image resize / transform
// ==================================================================

class ImageTransform {
public:
    // Resize with quality selection
    enum class Filter { Nearest, Bilinear, Bicubic, Lanczos };
    static DecodedImage resize(const DecodedImage& src, int dstWidth, int dstHeight,
                                 Filter filter = Filter::Bilinear);

    // Crop
    static DecodedImage crop(const DecodedImage& src, int x, int y, int w, int h);

    // Apply EXIF orientation
    static DecodedImage applyOrientation(const DecodedImage& src, int orientation);

    // Premultiply alpha
    static void premultiplyAlpha(DecodedImage& img);
    static void unpremultiplyAlpha(DecodedImage& img);
};

} // namespace NXRender
