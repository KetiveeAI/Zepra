// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "render_tile.h"
#include "nxgfx/context.h"

namespace NXRender {

RenderTile::RenderTile(int x, int y, int width, int height)
    : bounds_(x, y, width, height) {
    if (gpu()) {
        renderTarget_ = gpu()->createRenderTarget(width, height);
    }
}

RenderTile::~RenderTile() {
    if (gpu() && renderTarget_) {
        gpu()->destroyRenderTarget(renderTarget_);
    }
}

void RenderTile::beginRecord(GpuContext* ctx) {
    if (!renderTarget_) return;
    
    isReady_ = false;
    
    // Bind the abstract Render Target
    ctx->setRenderTarget(renderTarget_);
    
    // We clear the tile completely transparent
    ctx->clear(Color::transparent());
    
    // Translate the GPU context so elements draw correctly within this tile's regional coordinates
    ctx->pushTransform();
    ctx->translate(-bounds_.x, -bounds_.y);
}

void RenderTile::endRecord(GpuContext* ctx) {
    if (!renderTarget_) return;
    
    ctx->popTransform();
    
    // Restore default framebuffer (system window)
    ctx->setRenderTarget(0);
    
    isReady_ = true;
}

void RenderTile::draw(GpuContext* ctx, const Rect& targetRect) const {
    if (!isReady_ || !renderTarget_) return;
    
    // Pure abstraction! No OpenGL legacy glBegin calls!
    ctx->drawTexture(renderTarget_, targetRect);
}

} // namespace NXRender
