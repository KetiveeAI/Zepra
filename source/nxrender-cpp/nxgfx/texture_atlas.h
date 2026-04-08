// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file texture_atlas.h
 * @brief Runtime texture packing using shelf-first-fit bin packing.
 *
 * A TextureAtlas manages a single large GPU texture (e.g. 2048x2048) and
 * packs smaller sub-images into it. Used by the glyph cache and small
 * icon/image caching to reduce texture switching and draw calls.
 */

#pragma once

#include "../nxgfx/primitives.h"
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>

namespace NXRender {

class GpuContext;

/**
 * @brief UV coordinates for a region packed into the atlas.
 */
struct AtlasRegion {
    float u0 = 0, v0 = 0;  // Top-left UV
    float u1 = 0, v1 = 0;  // Bottom-right UV
    int x = 0, y = 0;      // Pixel position in atlas
    int width = 0, height = 0;
    int atlasIndex = 0;     // Which atlas page (for multi-page)
    bool valid = false;
};

/**
 * @brief Runtime texture atlas using shelf-first-fit bin packing.
 *
 * Shelf packing works by maintaining horizontal "shelves" (rows).
 * When a new sub-image is inserted, it's placed on the first shelf
 * that has enough width remaining. If no shelf fits, a new shelf
 * is created. If the atlas is full, it returns an invalid region.
 */
class TextureAtlas {
public:
    static constexpr int kDefaultSize = 2048;
    static constexpr int kPadding = 1; // 1px padding to prevent bleeding

    TextureAtlas();
    ~TextureAtlas();

    TextureAtlas(const TextureAtlas&) = delete;
    TextureAtlas& operator=(const TextureAtlas&) = delete;

    /**
     * @brief Initialize the atlas with a given size.
     * @param ctx GPU context for texture creation.
     * @param width Atlas texture width.
     * @param height Atlas texture height.
     * @param channels 1 = R8 (glyphs), 4 = RGBA8 (icons/images).
     * @return true on success.
     */
    bool init(GpuContext* ctx, int width = kDefaultSize, int height = kDefaultSize, int channels = 4);

    /**
     * @brief Destroy the atlas texture.
     */
    void shutdown();

    /**
     * @brief Insert a sub-image into the atlas.
     * @param width Sub-image width.
     * @param height Sub-image height.
     * @param pixels Pixel data (must match atlas channel count).
     * @param key Optional string key for lookup.
     * @return AtlasRegion with UV coordinates. Check .valid for success.
     */
    AtlasRegion insert(int width, int height, const uint8_t* pixels, const std::string& key = "");

    /**
     * @brief Look up a previously inserted region by key.
     * @return AtlasRegion (check .valid).
     */
    AtlasRegion find(const std::string& key) const;

    /**
     * @brief Remove a region by key (marks space as reusable on next defrag).
     */
    void remove(const std::string& key);

    /**
     * @brief Clear all regions and reset the atlas.
     */
    void clear();

    /**
     * @brief Upload any pending texture data to the GPU.
     */
    void flush();

    // Accessors
    uint32_t textureId() const { return textureId_; }
    int width() const { return width_; }
    int height() const { return height_; }
    int channels() const { return channels_; }
    float occupancy() const; // 0.0 - 1.0

    /**
     * @brief Number of regions in the atlas.
     */
    size_t regionCount() const { return regions_.size(); }

    /**
     * @brief Remaining vertical space in the atlas.
     */
    int remainingHeight() const;

private:
    struct Shelf {
        int y;       // Y position of this shelf
        int height;  // Height of this shelf (tallest item)
        int width;   // Current X cursor (next insertion point)
    };

    GpuContext* ctx_ = nullptr;
    uint32_t textureId_ = 0;
    int width_ = 0;
    int height_ = 0;
    int channels_ = 4;

    std::vector<Shelf> shelves_;
    std::unordered_map<std::string, AtlasRegion> regions_;
    size_t usedPixels_ = 0;

    // Pending uploads (batched to reduce texture upload calls)
    struct PendingUpload {
        int x, y, w, h;
        std::vector<uint8_t> pixels;
    };
    std::vector<PendingUpload> pendingUploads_;
};

} // namespace NXRender
