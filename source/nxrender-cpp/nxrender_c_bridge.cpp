/**
 * @file nxrender_c_bridge.cpp
 * @brief Implementation of C/C++ bridge functions
 * 
 * Links against libnxrender.a (C library) and provides C-compatible wrappers
 * that can be called from the browser's C++ code.
 * 
 * Note: This bridge provides a flattened API for easier C++ usage.
 * The actual C library uses structured types (nx_rect_t, nx_point_t, etc).
 * 
 * @copyright 2025 KetiveeAI
 */

#include "nxrender_c_bridge.h"

// Include the full C library headers
extern "C" {
#include <nxgfx/nxgfx.h>
#include <nxrender/compositor.h>
}

// Bridge implementations that translate between flattened C++ params and C struct types

extern "C" {

// Context management - direct passthrough
nx_context_t* nxgfx_bridge_init(void* framebuffer, uint32_t width, uint32_t height, uint32_t pitch) {
    return ::nxgfx_init(framebuffer, width, height, pitch);
}

void nxgfx_bridge_shutdown(nx_context_t* ctx) {
    ::nxgfx_destroy(ctx);
}

// Drawing with flattened params -> struct conversion
void nxgfx_bridge_fill_rect(nx_context_t* ctx, int32_t x, int32_t y, uint32_t w, uint32_t h,
                            uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    nx_rect_t rect = {x, y, w, h};
    nx_color_t color = {r, g, b, a};
    ::nxgfx_fill_rect(ctx, rect, color);
}

void nxgfx_bridge_fill_rounded_rect(nx_context_t* ctx, int32_t x, int32_t y, uint32_t w, uint32_t h,
                                    uint32_t radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    nx_rect_t rect = {x, y, w, h};
    nx_color_t color = {r, g, b, a};
    ::nxgfx_fill_rounded_rect(ctx, rect, color, radius);
}

void nxgfx_bridge_fill_circle(nx_context_t* ctx, int32_t cx, int32_t cy, uint32_t radius,
                              uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    nx_point_t center = {cx, cy};
    nx_color_t color = {r, g, b, a};
    ::nxgfx_fill_circle(ctx, center, radius, color);
}

void nxgfx_bridge_draw_line(nx_context_t* ctx, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                            uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint32_t thickness) {
    nx_point_t p1 = {x1, y1};
    nx_point_t p2 = {x2, y2};
    nx_color_t color = {r, g, b, a};
    ::nxgfx_draw_line(ctx, p1, p2, color, thickness);
}

void nxgfx_bridge_draw_text(nx_context_t* ctx, const char* text, int32_t x, int32_t y,
                            uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    nx_point_t pos = {x, y};
    nx_color_t color = {r, g, b, a};
    // Use default font - NULL means use default in the C lib
    ::nxgfx_draw_text(ctx, text, pos, NULL, color);
}

void nxgfx_bridge_set_clip_rect(nx_context_t* ctx, int32_t x, int32_t y, uint32_t w, uint32_t h) {
    nx_rect_t rect = {x, y, w, h};
    ::nxgfx_set_clip(ctx, rect);
}

void nxgfx_bridge_clear_clip_rect(nx_context_t* ctx) {
    ::nxgfx_clear_clip(ctx);
}

// Compositor bridge
nx_compositor_t* nx_compositor_bridge_create(nx_context_t* ctx) {
    return ::nx_compositor_create(ctx);
}

void nx_compositor_bridge_destroy(nx_compositor_t* comp) {
    ::nx_compositor_destroy(comp);
}

void nx_compositor_bridge_begin_frame(nx_compositor_t* comp) {
    ::nx_compositor_begin_frame(comp);
}

void nx_compositor_bridge_end_frame(nx_compositor_t* comp) {
    ::nx_compositor_end_frame(comp);
}

void nx_compositor_bridge_composite(nx_compositor_t* comp) {
    ::nx_compositor_composite(comp);
}

} // extern "C"
