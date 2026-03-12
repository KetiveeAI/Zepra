/**
 * @file nxrender_c_bridge.h
 * @brief Bridge header for C/C++ interoperability
 * 
 * Allows ZepraBrowser's C++ code to call into libnxrender.a (C library)
 * and vice versa. Provides thin C++ wrappers around C functions.
 * 
 * Usage in C++:
 *   #include "nxrender_c_bridge.h"
 *   auto* ctx = NXRender::C::createContext(fb, w, h, pitch);
 *   NXRender::C::fillRect(ctx, {0, 0, 100, 50}, {255, 0, 0, 255});
 * 
 * @copyright 2025 KetiveeAI
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations of C types from libnxrender.a
typedef struct nx_context nx_context_t;
typedef struct nx_compositor nx_compositor_t;
typedef struct nx_window_manager nx_window_manager_t;

// Bridge functions with unique names (avoid conflicts with actual C library)
// These translate flattened parameters to the struct-based C API

// Context
nx_context_t* nxgfx_bridge_init(void* framebuffer, uint32_t width, uint32_t height, uint32_t pitch);
void nxgfx_bridge_shutdown(nx_context_t* ctx);

// Drawing primitives (flattened params for C++ convenience)
void nxgfx_bridge_fill_rect(nx_context_t* ctx, int32_t x, int32_t y, uint32_t w, uint32_t h, 
                            uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void nxgfx_bridge_fill_rounded_rect(nx_context_t* ctx, int32_t x, int32_t y, uint32_t w, uint32_t h,
                                    uint32_t radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void nxgfx_bridge_fill_circle(nx_context_t* ctx, int32_t cx, int32_t cy, uint32_t radius,
                              uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void nxgfx_bridge_draw_line(nx_context_t* ctx, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                            uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint32_t thickness);
void nxgfx_bridge_draw_text(nx_context_t* ctx, const char* text, int32_t x, int32_t y,
                            uint8_t r, uint8_t g, uint8_t b, uint8_t a);

// Clipping
void nxgfx_bridge_set_clip_rect(nx_context_t* ctx, int32_t x, int32_t y, uint32_t w, uint32_t h);
void nxgfx_bridge_clear_clip_rect(nx_context_t* ctx);

// Compositor
nx_compositor_t* nx_compositor_bridge_create(nx_context_t* ctx);
void nx_compositor_bridge_destroy(nx_compositor_t* comp);
void nx_compositor_bridge_begin_frame(nx_compositor_t* comp);
void nx_compositor_bridge_end_frame(nx_compositor_t* comp);
void nx_compositor_bridge_composite(nx_compositor_t* comp);

#ifdef __cplusplus
}

// C++ wrapper namespace for convenient usage
namespace NXRender {
namespace C {

struct Color {
    uint8_t r, g, b, a;
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
};

struct Rect {
    int32_t x, y;
    uint32_t width, height;
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(int32_t x, int32_t y, uint32_t w, uint32_t h) : x(x), y(y), width(w), height(h) {}
};

inline nx_context_t* createContext(void* fb, uint32_t w, uint32_t h, uint32_t pitch) {
    return nxgfx_bridge_init(fb, w, h, pitch);
}

inline void destroyContext(nx_context_t* ctx) {
    nxgfx_bridge_shutdown(ctx);
}

inline void fillRect(nx_context_t* ctx, const Rect& r, const Color& c) {
    nxgfx_bridge_fill_rect(ctx, r.x, r.y, r.width, r.height, c.r, c.g, c.b, c.a);
}

inline void fillRoundedRect(nx_context_t* ctx, const Rect& r, uint32_t radius, const Color& c) {
    nxgfx_bridge_fill_rounded_rect(ctx, r.x, r.y, r.width, r.height, radius, c.r, c.g, c.b, c.a);
}

inline void fillCircle(nx_context_t* ctx, int32_t cx, int32_t cy, uint32_t radius, const Color& c) {
    nxgfx_bridge_fill_circle(ctx, cx, cy, radius, c.r, c.g, c.b, c.a);
}

inline void drawLine(nx_context_t* ctx, int32_t x1, int32_t y1, int32_t x2, int32_t y2, 
                     const Color& c, uint32_t thickness = 1) {
    nxgfx_bridge_draw_line(ctx, x1, y1, x2, y2, c.r, c.g, c.b, c.a, thickness);
}

inline void drawText(nx_context_t* ctx, const char* text, int32_t x, int32_t y, const Color& c) {
    nxgfx_bridge_draw_text(ctx, text, x, y, c.r, c.g, c.b, c.a);
}

inline void setClip(nx_context_t* ctx, const Rect& r) {
    nxgfx_bridge_set_clip_rect(ctx, r.x, r.y, r.width, r.height);
}

inline void clearClip(nx_context_t* ctx) {
    nxgfx_bridge_clear_clip_rect(ctx);
}

// Scoped clip guard for RAII-style clipping
class ClipGuard {
public:
    ClipGuard(nx_context_t* ctx, const Rect& r) : ctx_(ctx) { setClip(ctx, r); }
    ~ClipGuard() { clearClip(ctx_); }
private:
    nx_context_t* ctx_;
};

} // namespace C
} // namespace NXRender

#endif // __cplusplus
