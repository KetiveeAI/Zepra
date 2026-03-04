/*
 * NXRender FFI Stubs
 * 
 * Stub implementations for NXRender FFI functions.
 * These allow the REOX browser to compile and run without
 * the full NXRender Rust library.
 * 
 * When NXRender is available, replace these with actual FFI bindings.
 */

#ifndef NXRENDER_FFI_STUBS_H
#define NXRENDER_FFI_STUBS_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Types
 * ============================================================================ */

typedef struct { uint8_t r, g, b, a; } NxColor;
typedef struct { float x, y, width, height; } NxRect;
typedef struct { float x, y; } NxPoint;

/* Opaque handles */
typedef void* NxGpuContext;
typedef void* NxTheme;
typedef void* NxMouseState;
typedef void* NxKeyboardState;

typedef enum { NX_MOUSE_LEFT = 0, NX_MOUSE_RIGHT = 1, NX_MOUSE_MIDDLE = 2 } NxMouseButton;

/* ============================================================================
 * GPU Context Stubs
 * ============================================================================ */

/* Internal GPU state */
typedef struct {
    uint32_t width;
    uint32_t height;
    NxColor clear_color;
    /* In real impl: OpenGL context, framebuffer, etc. */
} NxGpuContextImpl;

static inline NxGpuContext nx_gpu_create(void) {
    NxGpuContextImpl* ctx = (NxGpuContextImpl*)calloc(1, sizeof(NxGpuContextImpl));
    ctx->width = 1280;
    ctx->height = 800;
    ctx->clear_color = (NxColor){13, 17, 23, 255}; /* Dark theme bg */
    printf("[NXRender] GPU context created (stub)\n");
    return (NxGpuContext)ctx;
}

static inline NxGpuContext nx_gpu_create_with_size(uint32_t width, uint32_t height) {
    NxGpuContextImpl* ctx = (NxGpuContextImpl*)calloc(1, sizeof(NxGpuContextImpl));
    ctx->width = width;
    ctx->height = height;
    ctx->clear_color = (NxColor){13, 17, 23, 255};
    printf("[NXRender] GPU context created %dx%d (stub)\n", width, height);
    return (NxGpuContext)ctx;
}

static inline void nx_gpu_destroy(NxGpuContext ctx) {
    if (ctx) free(ctx);
    printf("[NXRender] GPU context destroyed (stub)\n");
}

static inline void nx_gpu_present(NxGpuContext ctx) {
    (void)ctx;
    /* Swap buffers - in real impl: glXSwapBuffers or eglSwapBuffers */
}

static inline void nx_gpu_resize(NxGpuContext ctx, uint32_t width, uint32_t height) {
    if (!ctx) return;
    NxGpuContextImpl* impl = (NxGpuContextImpl*)ctx;
    impl->width = width;
    impl->height = height;
    printf("[NXRender] GPU resized to %dx%d (stub)\n", width, height);
}

static inline void nx_gpu_clear(NxGpuContext ctx, NxColor color) {
    if (!ctx) return;
    NxGpuContextImpl* impl = (NxGpuContextImpl*)ctx;
    impl->clear_color = color;
    /* In real impl: glClearColor + glClear */
}

static inline void nx_gpu_fill_rect(NxGpuContext ctx, NxRect rect, NxColor color) {
    (void)ctx; (void)rect; (void)color;
    /* In real impl: glBegin(GL_QUADS) or vertex buffer */
}

static inline void nx_gpu_fill_rounded_rect(NxGpuContext ctx, NxRect rect, NxColor color, float radius) {
    (void)ctx; (void)rect; (void)color; (void)radius;
    /* In real impl: draw with rounded corners using triangles */
}

static inline void nx_gpu_fill_circle(NxGpuContext ctx, float x, float y, float radius, NxColor color) {
    (void)ctx; (void)x; (void)y; (void)radius; (void)color;
    /* In real impl: GL_TRIANGLE_FAN */
}

static inline void nx_gpu_draw_text(NxGpuContext ctx, const char* text, float x, float y, NxColor color) {
    (void)ctx; (void)text; (void)x; (void)y; (void)color;
    /* In real impl: FreeType + texture atlas */
}

static inline void nx_gpu_draw_line(NxGpuContext ctx, float x1, float y1, float x2, float y2, NxColor color, float width) {
    (void)ctx; (void)x1; (void)y1; (void)x2; (void)y2; (void)color; (void)width;
}

static inline void nx_gpu_set_clip(NxGpuContext ctx, NxRect rect) {
    (void)ctx; (void)rect;
    /* glScissor */
}

static inline void nx_gpu_clear_clip(NxGpuContext ctx) {
    (void)ctx;
    /* glDisable(GL_SCISSOR_TEST) */
}

/* ============================================================================
 * Theme Stubs
 * ============================================================================ */

typedef struct {
    NxColor primary;
    NxColor background;
    NxColor surface;
    NxColor text;
    bool is_dark;
} NxThemeImpl;

static inline NxTheme nx_theme_dark(void) {
    NxThemeImpl* theme = (NxThemeImpl*)calloc(1, sizeof(NxThemeImpl));
    theme->primary = (NxColor){88, 166, 255, 255};    /* #58A6FF */
    theme->background = (NxColor){13, 17, 23, 255};   /* #0D1117 */
    theme->surface = (NxColor){22, 27, 34, 255};      /* #161B22 */
    theme->text = (NxColor){240, 246, 252, 255};      /* #F0F6FC */
    theme->is_dark = true;
    printf("[NXRender] Dark theme created (stub)\n");
    return (NxTheme)theme;
}

static inline NxTheme nx_theme_light(void) {
    NxThemeImpl* theme = (NxThemeImpl*)calloc(1, sizeof(NxThemeImpl));
    theme->primary = (NxColor){9, 105, 218, 255};     /* #0969DA */
    theme->background = (NxColor){255, 255, 255, 255};
    theme->surface = (NxColor){246, 248, 250, 255};
    theme->text = (NxColor){31, 35, 40, 255};
    theme->is_dark = false;
    printf("[NXRender] Light theme created (stub)\n");
    return (NxTheme)theme;
}

static inline void nx_theme_destroy(NxTheme theme) {
    if (theme) free(theme);
}

static inline NxColor nx_theme_get_primary_color(NxTheme theme) {
    if (!theme) return (NxColor){88, 166, 255, 255};
    return ((NxThemeImpl*)theme)->primary;
}

static inline NxColor nx_theme_get_background_color(NxTheme theme) {
    if (!theme) return (NxColor){13, 17, 23, 255};
    return ((NxThemeImpl*)theme)->background;
}

static inline NxColor nx_theme_get_surface_color(NxTheme theme) {
    if (!theme) return (NxColor){22, 27, 34, 255};
    return ((NxThemeImpl*)theme)->surface;
}

static inline NxColor nx_theme_get_text_color(NxTheme theme) {
    if (!theme) return (NxColor){240, 246, 252, 255};
    return ((NxThemeImpl*)theme)->text;
}

/* ============================================================================
 * Mouse State Stubs
 * ============================================================================ */

typedef struct {
    float x, y;
    bool buttons[3];
} NxMouseStateImpl;

static inline NxMouseState nx_mouse_create(void) {
    NxMouseStateImpl* mouse = (NxMouseStateImpl*)calloc(1, sizeof(NxMouseStateImpl));
    return (NxMouseState)mouse;
}

static inline void nx_mouse_destroy(NxMouseState mouse) {
    if (mouse) free(mouse);
}

static inline void nx_mouse_get_position(NxMouseState mouse, float* x, float* y) {
    if (!mouse) { *x = *y = 0; return; }
    NxMouseStateImpl* impl = (NxMouseStateImpl*)mouse;
    *x = impl->x;
    *y = impl->y;
}

static inline bool nx_mouse_is_button_down(NxMouseState mouse, NxMouseButton button) {
    if (!mouse || button > 2) return false;
    return ((NxMouseStateImpl*)mouse)->buttons[button];
}

static inline void nx_mouse_move(NxMouseState mouse, float x, float y) {
    if (!mouse) return;
    NxMouseStateImpl* impl = (NxMouseStateImpl*)mouse;
    impl->x = x;
    impl->y = y;
}

static inline void nx_mouse_button_down(NxMouseState mouse, float x, float y, NxMouseButton button) {
    if (!mouse || button > 2) return;
    NxMouseStateImpl* impl = (NxMouseStateImpl*)mouse;
    impl->x = x;
    impl->y = y;
    impl->buttons[button] = true;
}

static inline void nx_mouse_button_up(NxMouseState mouse, float x, float y, NxMouseButton button) {
    if (!mouse || button > 2) return;
    NxMouseStateImpl* impl = (NxMouseStateImpl*)mouse;
    impl->x = x;
    impl->y = y;
    impl->buttons[button] = false;
}

/* ============================================================================
 * Keyboard State Stubs
 * ============================================================================ */

typedef struct {
    bool ctrl;
    bool shift;
    bool alt;
    bool meta;
} NxKeyboardStateImpl;

static inline NxKeyboardState nx_keyboard_create(void) {
    NxKeyboardStateImpl* kb = (NxKeyboardStateImpl*)calloc(1, sizeof(NxKeyboardStateImpl));
    return (NxKeyboardState)kb;
}

static inline void nx_keyboard_destroy(NxKeyboardState kb) {
    if (kb) free(kb);
}

static inline bool nx_keyboard_is_ctrl(NxKeyboardState kb) {
    return kb ? ((NxKeyboardStateImpl*)kb)->ctrl : false;
}

static inline bool nx_keyboard_is_shift(NxKeyboardState kb) {
    return kb ? ((NxKeyboardStateImpl*)kb)->shift : false;
}

static inline bool nx_keyboard_is_alt(NxKeyboardState kb) {
    return kb ? ((NxKeyboardStateImpl*)kb)->alt : false;
}

static inline void nx_keyboard_set_ctrl(NxKeyboardState kb, bool down) {
    if (kb) ((NxKeyboardStateImpl*)kb)->ctrl = down;
}

static inline void nx_keyboard_set_shift(NxKeyboardState kb, bool down) {
    if (kb) ((NxKeyboardStateImpl*)kb)->shift = down;
}

static inline void nx_keyboard_set_alt(NxKeyboardState kb, bool down) {
    if (kb) ((NxKeyboardStateImpl*)kb)->alt = down;
}

/* ============================================================================
 * Text Measurement (Stub)
 * ============================================================================ */

static inline int nx_text_width(const char* text, int font_size) {
    if (!text) return 0;
    /* Approximate: 0.6 * font_size per character */
    return (int)(strlen(text) * font_size * 0.6);
}

static inline int nx_text_height(int font_size) {
    return (int)(font_size * 1.2);
}

/* ============================================================================
 * Image Loading (Stub)
 * ============================================================================ */

typedef void* NxImage;

static inline NxImage nx_image_load(const char* path) {
    printf("[NXRender] Image load: %s (stub)\n", path);
    return NULL; /* Stub */
}

static inline void nx_image_destroy(NxImage img) {
    (void)img;
}

static inline void nx_gpu_draw_image(NxGpuContext ctx, NxImage img, NxRect rect) {
    (void)ctx; (void)img; (void)rect;
}

/* ============================================================================
 * Gradient (Stub)
 * ============================================================================ */

static inline void nx_gpu_fill_gradient_v(NxGpuContext ctx, NxRect rect, NxColor top, NxColor bottom) {
    (void)ctx; (void)rect; (void)top; (void)bottom;
    /* Vertical gradient */
}

static inline void nx_gpu_fill_gradient_h(NxGpuContext ctx, NxRect rect, NxColor left, NxColor right) {
    (void)ctx; (void)rect; (void)left; (void)right;
    /* Horizontal gradient */
}

#ifdef __cplusplus
}
#endif

#endif /* NXRENDER_FFI_STUBS_H */
