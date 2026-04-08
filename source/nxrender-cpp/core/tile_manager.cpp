// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "tile_manager.h"
#include <cmath>
#include <algorithm>

namespace NXRender {

TileManager::TileManager(int tileSize) : tileSize_(tileSize) {}

TileManager::~TileManager() {}

void TileManager::setBounds(int width, int height) {
    if (width_ == width && height_ == height) return;
    
    width_ = width;
    height_ = height;

    cols_ = std::ceil((float)width / tileSize_);
    rows_ = std::ceil((float)height / tileSize_);
    
    evictOffscreenTiles();
    allocateVisibleTiles();
}

void TileManager::setViewportOffset(int x, int y) {
    if (viewerX_ == x && viewerY_ == y) return;
    viewerX_ = x;
    viewerY_ = y;
    
    evictOffscreenTiles();
    allocateVisibleTiles();
}

void TileManager::evictOffscreenTiles() {
    // Keep tiles strictly inside a padded viewport (e.g., 2 tiles padding)
    int startCol = std::max(0, (viewerX_ / tileSize_) - 2);
    int startRow = std::max(0, (viewerY_ / tileSize_) - 2);
    // Use dynamic viewport bounds (width_, height_) instead of hardcoded 1080p
    int endCol = std::min(cols_ - 1, ((viewerX_ + width_) / tileSize_) + 2);
    int endRow = std::min(rows_ - 1, ((viewerY_ + height_) / tileSize_) + 2);

    auto it = tiles_.begin();
    while (it != tiles_.end()) {
        uint64_t key = it->first;
        int tx = static_cast<int>(key & 0xFFFFFFFF);
        int ty = static_cast<int>(key >> 32);
        
        if (tx < startCol || tx > endCol || ty < startRow || ty > endRow) {
            it = tiles_.erase(it);
        } else {
            ++it;
        }
    }
}

void TileManager::allocateVisibleTiles() {
    int startCol = std::max(0, (viewerX_ / tileSize_) - 1);
    int startRow = std::max(0, (viewerY_ / tileSize_) - 1);
    // Use dynamic viewport bounds (width_, height_) corresponding to window
    int endCol = std::min(cols_ - 1, ((viewerX_ + width_) / tileSize_) + 1);
    int endRow = std::min(rows_ - 1, ((viewerY_ + height_) / tileSize_) + 1);

    for (int y = startRow; y <= endRow; ++y) {
        for (int x = startCol; x <= endCol; ++x) {
            uint64_t key = (static_cast<uint64_t>(y) << 32) | static_cast<uint32_t>(x);
            if (tiles_.find(key) == tiles_.end()) {
                tiles_[key] = std::make_unique<RenderTile>(
                    x * tileSize_, y * tileSize_, 
                    tileSize_, tileSize_
                );
            }
        }
    }
}

std::vector<RenderTile*> TileManager::getTilesIntersecting(const Rect& rect) {
    std::vector<RenderTile*> result;
    if (rect.width <= 0 || rect.height <= 0) return result;

    int startCol = std::max(0, static_cast<int>(rect.x / tileSize_));
    int startRow = std::max(0, static_cast<int>(rect.y / tileSize_));
    int endCol = std::min(cols_ - 1, static_cast<int>((rect.x + rect.width) / tileSize_));
    int endRow = std::min(rows_ - 1, static_cast<int>((rect.y + rect.height) / tileSize_));

    for (int y = startRow; y <= endRow; ++y) {
        for (int x = startCol; x <= endCol; ++x) {
            uint64_t key = (static_cast<uint64_t>(y) << 32) | static_cast<uint32_t>(x);
            if (tiles_.find(key) == tiles_.end()) {
                // Ensure tile is lazily allocated if needed for damage region
                tiles_[key] = std::make_unique<RenderTile>(
                    x * tileSize_, y * tileSize_, 
                    tileSize_, tileSize_
                );
            }
            result.push_back(tiles_[key].get());
        }
    }

    return result;
}

void TileManager::invalidateRect(const Rect& rect) {
    auto tiles = getTilesIntersecting(rect);
    for (auto* tile : tiles) {
        tile->invalidate();
    }
}

void TileManager::composite(GpuContext* ctx) const {
    for (const auto& pair : tiles_) {
        // Only draws if isReady() is true inside draw()
        pair.second->draw(ctx, pair.second->bounds());
    }
}

} // namespace NXRender
