// NXRENDER C Header - Full API
// Native rendering engine for ZepraBrowser

#ifndef NXRENDER_H
#define NXRENDER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============= TYPES =============

typedef struct NxGpuContext NxGpuContext;
typedef struct NxTheme NxTheme;
typedef struct NxMouseState NxMouseState;
typedef struct NxTouchState NxTouchState;
typedef struct NxGestureRecognizer NxGestureRecognizer;
typedef struct NxGestureTranslator NxGestureTranslator;
typedef struct NxKeyboardState NxKeyboardState;

typedef struct { uint8_t r, g, b, a; } NxColor;
typedef struct { float x, y, width, height; } NxRect;
typedef struct { float x, y; } NxPoint;

typedef struct {
    char* gpu_name;
    char* gpu_vendor;
    char* gpu_driver;
    uint32_t display_width;
    uint32_t display_height;
    uint32_t display_refresh;
    char* display_name;
} NxSystemInfo;

typedef enum { NX_MOUSE_LEFT = 0, NX_MOUSE_RIGHT = 1, NX_MOUSE_MIDDLE = 2 } NxMouseButton;

typedef enum {
    NX_GESTURE_NONE = 0,
    NX_GESTURE_TAP = 1,
    NX_GESTURE_DOUBLE_TAP = 2,
    NX_GESTURE_LONG_PRESS = 3,
    NX_GESTURE_PAN = 4,
    NX_GESTURE_SWIPE_LEFT = 5,
    NX_GESTURE_SWIPE_RIGHT = 6,
    NX_GESTURE_SWIPE_UP = 7,
    NX_GESTURE_SWIPE_DOWN = 8,
    NX_GESTURE_PINCH_IN = 9,
    NX_GESTURE_PINCH_OUT = 10,
    NX_GESTURE_ROTATE = 11,
} NxGestureType;

typedef struct {
    NxGestureType gesture_type;
    float x, y;
    float scale;
    float rotation;
    float velocity_x, velocity_y;
} NxGestureResult;

typedef enum {
    NX_ACTION_NONE = 0,
    NX_ACTION_BACK = 1,
    NX_ACTION_FORWARD = 2,
    NX_ACTION_REFRESH = 3,
    NX_ACTION_ZOOM_IN = 4,
    NX_ACTION_ZOOM_OUT = 5,
    NX_ACTION_ZOOM_RESET = 6,
    NX_ACTION_NEXT_TAB = 7,
    NX_ACTION_PREV_TAB = 8,
    NX_ACTION_CLOSE_TAB = 9,
    NX_ACTION_FULLSCREEN = 10,
} NxGestureAction;

// ============= HELPERS =============

static inline NxColor nx_rgb(uint8_t r, uint8_t g, uint8_t b) { return (NxColor){r, g, b, 255}; }
static inline NxColor nx_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return (NxColor){r, g, b, a}; }
static inline NxRect nx_rect(float x, float y, float w, float h) { return (NxRect){x, y, w, h}; }
static inline NxPoint nx_point(float x, float y) { return (NxPoint){x, y}; }

// ============= SYSTEM DETECTION =============

NxSystemInfo* nx_detect_system(void);
void nx_free_system_info(NxSystemInfo* info);

// ============= GPU CONTEXT =============

NxGpuContext* nx_gpu_create(void);
NxGpuContext* nx_gpu_create_with_size(uint32_t width, uint32_t height);
void nx_gpu_destroy(NxGpuContext* ctx);
void nx_gpu_fill_rect(NxGpuContext* ctx, NxRect rect, NxColor color);
void nx_gpu_fill_rounded_rect(NxGpuContext* ctx, NxRect rect, NxColor color, float radius);
void nx_gpu_fill_circle(NxGpuContext* ctx, float x, float y, float radius, NxColor color);
void nx_gpu_draw_text(NxGpuContext* ctx, const char* text, float x, float y, NxColor color);
void nx_gpu_draw_line(NxGpuContext* ctx, float x1, float y1, float x2, float y2, NxColor color, float thickness);
void nx_gpu_present(NxGpuContext* ctx);
void nx_gpu_clear(NxGpuContext* ctx, NxColor color);
void nx_gpu_resize(NxGpuContext* ctx, uint32_t width, uint32_t height);

// ============= THEME =============

NxTheme* nx_theme_light(void);
NxTheme* nx_theme_dark(void);
void nx_theme_destroy(NxTheme* theme);
NxColor nx_theme_get_primary_color(const NxTheme* theme);
NxColor nx_theme_get_background_color(const NxTheme* theme);
NxColor nx_theme_get_surface_color(const NxTheme* theme);
NxColor nx_theme_get_text_color(const NxTheme* theme);

// ============= MOUSE =============

NxMouseState* nx_mouse_create(void);
void nx_mouse_destroy(NxMouseState* mouse);
void nx_mouse_get_position(const NxMouseState* mouse, float* x, float* y);
bool nx_mouse_is_button_down(const NxMouseState* mouse, NxMouseButton button);
void nx_mouse_move(NxMouseState* mouse, float x, float y);
void nx_mouse_button_down(NxMouseState* mouse, float x, float y, NxMouseButton button);
void nx_mouse_button_up(NxMouseState* mouse, float x, float y, NxMouseButton button);

// ============= TOUCH =============

NxTouchState* nx_touch_create(void);
void nx_touch_destroy(NxTouchState* touch);
uint32_t nx_touch_count(const NxTouchState* touch);
void nx_touch_start(NxTouchState* touch, uint64_t id, float x, float y);
void nx_touch_move(NxTouchState* touch, uint64_t id, float x, float y);
void nx_touch_end(NxTouchState* touch, uint64_t id, float x, float y);
bool nx_touch_get_center(const NxTouchState* touch, float* x, float* y);

// ============= GESTURE RECOGNITION =============

NxGestureRecognizer* nx_gesture_create(void);
void nx_gesture_destroy(NxGestureRecognizer* gesture);
NxGestureResult nx_gesture_detect(const NxGestureRecognizer* gesture, const NxTouchState* touch);

// ============= GESTURE TRANSLATION =============

NxGestureTranslator* nx_translator_create(void);
void nx_translator_destroy(NxGestureTranslator* tr);
void nx_translator_set_screen_width(NxGestureTranslator* tr, float width);
NxGestureAction nx_translator_translate(const NxGestureTranslator* tr, NxGestureType gesture, uint8_t fingers, float edge_x);

// ============= KEYBOARD =============

NxKeyboardState* nx_keyboard_create(void);
void nx_keyboard_destroy(NxKeyboardState* kb);
bool nx_keyboard_is_ctrl(const NxKeyboardState* kb);
bool nx_keyboard_is_shift(const NxKeyboardState* kb);
bool nx_keyboard_is_alt(const NxKeyboardState* kb);

// ============= VERSION =============

const char* nx_version(void);

#ifdef __cplusplus
}
#endif

#endif // NXRENDER_H
