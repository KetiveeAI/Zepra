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

    int newCols = std::ceil((float)width / tileSize_);
    int newRows = std::ceil((float)height / tileSize_);

    if (newCols == cols_ && newRows == rows_) return;

    cols_ = newCols;
    rows_ = newRows;

    tiles_.clear();
    tiles_.reserve(cols_ * rows_);

    for (int y = 0; y < rows_; ++y) {
        for (int x = 0; x < cols_; ++x) {
            tiles_.push_back(std::make_unique<RenderTile>(
                x * tileSize_, y * tileSize_, 
                tileSize_, tileSize_
            ));
        }
    }
}

std::vector<RenderTile*> TileManager::getTilesIntersecting(const Rect& rect) {
    std::vector<RenderTile*> result;
    if (tiles_.empty() || rect.width <= 0 || rect.height <= 0) return result;

    int startCol = std::max(0, static_cast<int>(rect.x / tileSize_));
    int startRow = std::max(0, static_cast<int>(rect.y / tileSize_));
    int endCol = std::min(cols_ - 1, static_cast<int>((rect.x + rect.width) / tileSize_));
    int endRow = std::min(rows_ - 1, static_cast<int>((rect.y + rect.height) / tileSize_));

    for (int y = startRow; y <= endRow; ++y) {
        for (int x = startCol; x <= endCol; ++x) {
            result.push_back(tiles_[y * cols_ + x].get());
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
    for (const auto& tile : tiles_) {
        // Only draws if isReady() is true inside draw()
        tile->draw(ctx, tile->bounds());
    }
}

} // namespace NXRender
