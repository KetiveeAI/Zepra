// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "nxgfx/texture_atlas.h"
#include "nxgfx/context.h"
#include <GL/gl.h>
#include <cstring>
#include <algorithm>
#include <iostream>

namespace NXRender {

TextureAtlas::TextureAtlas() {}

TextureAtlas::~TextureAtlas() {
    shutdown();
}

bool TextureAtlas::init(GpuContext* ctx, int width, int height, int channels) {
    if (!ctx || width <= 0 || height <= 0) return false;
    if (channels != 1 && channels != 4) return false;

    ctx_ = ctx;
    width_ = width;
    height_ = height;
    channels_ = channels;
    usedPixels_ = 0;
    shelves_.clear();
    regions_.clear();
    pendingUploads_.clear();

    // Create the atlas texture
    GLenum format = (channels == 1) ? GL_ALPHA : GL_RGBA;
    GLenum internalFormat = (channels == 1) ? GL_ALPHA : GL_RGBA;

    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    // Allocate empty texture
    std::vector<uint8_t> empty(static_cast<size_t>(width) * height * channels, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format,
                 GL_UNSIGNED_BYTE, empty.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    textureId_ = texId;

    std::cout << "[TextureAtlas] Created " << width << "x" << height
              << " (ch=" << channels << ", texId=" << textureId_ << ")" << std::endl;

    return true;
}

void TextureAtlas::shutdown() {
    if (textureId_ != 0) {
        GLuint texId = textureId_;
        glDeleteTextures(1, &texId);
        textureId_ = 0;
    }
    shelves_.clear();
    regions_.clear();
    pendingUploads_.clear();
    usedPixels_ = 0;
    ctx_ = nullptr;
}

AtlasRegion TextureAtlas::insert(int w, int h, const uint8_t* pixels, const std::string& key) {
    if (w <= 0 || h <= 0 || !textureId_) return AtlasRegion{};

    // Check key cache first
    if (!key.empty()) {
        auto it = regions_.find(key);
        if (it != regions_.end()) return it->second;
    }

    int paddedW = w + kPadding * 2;
    int paddedH = h + kPadding * 2;

    // Can't fit at all
    if (paddedW > width_ || paddedH > height_) return AtlasRegion{};

    // Try to fit in an existing shelf
    Shelf* bestShelf = nullptr;
    int bestWaste = INT32_MAX;

    for (auto& shelf : shelves_) {
        if (shelf.width + paddedW <= width_ && shelf.height >= paddedH) {
            int waste = shelf.height - paddedH;
            if (waste < bestWaste) {
                bestWaste = waste;
                bestShelf = &shelf;
            }
        }
    }

    // Create a new shelf if nothing fits
    if (!bestShelf) {
        int shelfY = 0;
        if (!shelves_.empty()) {
            const auto& last = shelves_.back();
            shelfY = last.y + last.height;
        }

        if (shelfY + paddedH > height_) {
            // Atlas is full
            return AtlasRegion{};
        }

        shelves_.push_back({shelfY, paddedH, 0});
        bestShelf = &shelves_.back();
    }

    // Place in shelf
    int px = bestShelf->width + kPadding;
    int py = bestShelf->y + kPadding;
    bestShelf->width += paddedW;

    // Queue pixel upload
    if (pixels) {
        PendingUpload upload;
        upload.x = px;
        upload.y = py;
        upload.w = w;
        upload.h = h;
        upload.pixels.assign(pixels, pixels + static_cast<size_t>(w) * h * channels_);
        pendingUploads_.push_back(std::move(upload));
    }

    // Create region
    AtlasRegion region;
    region.x = px;
    region.y = py;
    region.width = w;
    region.height = h;
    region.u0 = static_cast<float>(px) / static_cast<float>(width_);
    region.v0 = static_cast<float>(py) / static_cast<float>(height_);
    region.u1 = static_cast<float>(px + w) / static_cast<float>(width_);
    region.v1 = static_cast<float>(py + h) / static_cast<float>(height_);
    region.atlasIndex = 0;
    region.valid = true;

    usedPixels_ += static_cast<size_t>(w) * h;

    if (!key.empty()) {
        regions_[key] = region;
    }

    return region;
}

AtlasRegion TextureAtlas::find(const std::string& key) const {
    auto it = regions_.find(key);
    if (it != regions_.end()) return it->second;
    return AtlasRegion{};
}

void TextureAtlas::remove(const std::string& key) {
    auto it = regions_.find(key);
    if (it != regions_.end()) {
        usedPixels_ -= static_cast<size_t>(it->second.width) * it->second.height;
        regions_.erase(it);
    }
}

void TextureAtlas::clear() {
    shelves_.clear();
    regions_.clear();
    pendingUploads_.clear();
    usedPixels_ = 0;

    // Clear the GPU texture
    if (textureId_) {
        GLenum format = (channels_ == 1) ? GL_ALPHA : GL_RGBA;
        std::vector<uint8_t> empty(static_cast<size_t>(width_) * height_ * channels_, 0);
        glBindTexture(GL_TEXTURE_2D, textureId_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, format,
                        GL_UNSIGNED_BYTE, empty.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void TextureAtlas::flush() {
    if (pendingUploads_.empty() || !textureId_) return;

    GLenum format = (channels_ == 1) ? GL_ALPHA : GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureId_);

    // Set pixel alignment for single-channel
    if (channels_ == 1) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    for (const auto& upload : pendingUploads_) {
        glTexSubImage2D(GL_TEXTURE_2D, 0,
                        upload.x, upload.y, upload.w, upload.h,
                        format, GL_UNSIGNED_BYTE,
                        upload.pixels.data());
    }

    if (channels_ == 1) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    pendingUploads_.clear();
}

float TextureAtlas::occupancy() const {
    size_t totalPixels = static_cast<size_t>(width_) * height_;
    if (totalPixels == 0) return 0.0f;
    return static_cast<float>(usedPixels_) / static_cast<float>(totalPixels);
}

int TextureAtlas::remainingHeight() const {
    if (shelves_.empty()) return height_;
    const auto& last = shelves_.back();
    return height_ - (last.y + last.height);
}

} // namespace NXRender
