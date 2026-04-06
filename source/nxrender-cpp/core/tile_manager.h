// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "render_tile.h"
#include <vector>
#include <memory>

namespace NXRender {
class GpuContext;



class TileManager {
public:
    TileManager(int tileSize = 256);
    ~TileManager();

    // Resizes the virtual canvas, allocating or destroying tiles as needed
    void setBounds(int width, int height);

    // Returns all tiles intersecting the given rect
    std::vector<RenderTile*> getTilesIntersecting(const Rect& rect);

    // Invalidates tiles intersecting the given rect
    void invalidateRect(const Rect& rect);

    // Render all ready tiles
    void composite(GpuContext* ctx) const;

    int getTileSize() const { return tileSize_; }

private:
    int tileSize_;
    int width_ = 0;
    int height_ = 0;
    int cols_ = 0;
    int rows_ = 0;

    std::vector<std::unique_ptr<RenderTile>> tiles_;
};

} // namespace NXRender
