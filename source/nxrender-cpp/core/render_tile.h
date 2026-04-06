// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "nxgfx/primitives.h"
#include <vector>

namespace NXRender {
class GpuContext;



class RenderTile {
public:
    RenderTile(int x, int y, int width, int height);
    ~RenderTile();

    const Rect& bounds() const { return bounds_; }
    bool isReady() const { return isReady_; }
    
    // Binds this tile as the active render target (FBO)
    void beginRecord(GpuContext* ctx);
    
    // Unbinds FBO, generates mipmaps if necessary
    void endRecord(GpuContext* ctx);

    // Submits tile to the screen
    void draw(GpuContext* ctx, const Rect& targetRect) const;
    
    void invalidate() { isReady_ = false; }

private:
    Rect bounds_;
    bool isReady_ = false;
    
    // Native OpenGL handles
    unsigned int fbo_ = 0;
    unsigned int texture_ = 0;
    
    void initFbo();
    void releaseFbo();
};

} // namespace NXRender
