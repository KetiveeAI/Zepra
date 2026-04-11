// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "linux_wayland.h"
#include <cstring>
#include <algorithm>

namespace NXRender {

// ==================================================================
// WaylandPlatform — core lifecycle
// ==================================================================

WaylandPlatform::WaylandPlatform() {}

WaylandPlatform::~WaylandPlatform() {
    shutdown();
}

bool WaylandPlatform::init(int width, int height, const std::string& title) {
    if (initialized_) return true;

    width_ = width;
    height_ = height;

    // Connect to Wayland display
    // display_ = wl_display_connect(NULL);
    display_ = nullptr; // Stub: would call wl_display_connect
    if (!display_) return false;

    // Get registry and bind globals
    // registry_ = wl_display_get_registry(display_);
    // wl_registry_add_listener(registry_, &registryListener, this);
    // wl_display_roundtrip(display_);

    // Create surface
    // surface_ = wl_compositor_create_surface(compositor_);
    // xdgSurface_ = xdg_wm_base_get_xdg_surface(xdgWmBase_, surface_);
    // xdgToplevel_ = xdg_surface_get_toplevel(xdgSurface_);

    if (!initEGL()) return false;

    initialized_ = true;
    return true;
}

void WaylandPlatform::shutdown() {
    if (!initialized_) return;

    cleanupEGL();

    // Destroy Wayland objects in reverse order
    // if (xdgToplevel_) xdg_toplevel_destroy(xdgToplevel_);
    // if (xdgSurface_) xdg_surface_destroy(xdgSurface_);
    // if (surface_) wl_surface_destroy(surface_);
    // if (registry_) wl_registry_destroy(registry_);
    // if (display_) wl_display_disconnect(display_);

    xdgToplevel_ = nullptr;
    xdgSurface_ = nullptr;
    surface_ = nullptr;
    registry_ = nullptr;
    display_ = nullptr;
    initialized_ = false;
}

bool WaylandPlatform::initEGL() {
    // EGL initialization for Wayland
    // eglDisplay_ = eglGetDisplay(display_);
    // eglInitialize(eglDisplay_, ...);
    // Choose config with EGL_SURFACE_TYPE = EGL_WINDOW_BIT
    // Create context with EGL_CONTEXT_CLIENT_VERSION = 3
    // eglWindow_ = wl_egl_window_create(surface_, width_, height_);
    // eglSurface_ = eglCreateWindowSurface(eglDisplay_, eglConfig_, eglWindow_, NULL);
    // eglMakeCurrent(eglDisplay_, eglSurface_, eglSurface_, eglContext_);
    return true;
}

void WaylandPlatform::cleanupEGL() {
    // eglDestroySurface(eglDisplay_, eglSurface_);
    // wl_egl_window_destroy(eglWindow_);
    // eglDestroyContext(eglDisplay_, eglContext_);
    // eglTerminate(eglDisplay_);
    eglSurface_ = nullptr;
    eglWindow_ = nullptr;
    eglContext_ = nullptr;
    eglDisplay_ = nullptr;
}

void WaylandPlatform::pollEvents() {
    if (!display_) return;
    // wl_display_dispatch_pending(display_);
    // wl_display_flush(display_);
}

void WaylandPlatform::swapBuffers() {
    if (!eglDisplay_ || !eglSurface_) return;
    // eglSwapBuffers(eglDisplay_, eglSurface_);

    // Commit damage
    commitDamage();
}

void WaylandPlatform::setTitle(const std::string& title) {
    if (!xdgToplevel_) return;
    // xdg_toplevel_set_title(xdgToplevel_, title.c_str());
    (void)title;
}

void WaylandPlatform::setWindowSize(int w, int h) {
    width_ = w;
    height_ = h;
    if (eglWindow_) {
        // wl_egl_window_resize(eglWindow_, w, h, 0, 0);
    }
}

void WaylandPlatform::setCursorVisible(bool visible) {
    if (!pointer_) return;
    if (!visible) {
        // wl_pointer_set_cursor(pointer_, pointerSerial_, NULL, 0, 0);
    }
    (void)visible;
}

void WaylandPlatform::setCursor(CursorType type) {
    if (!pointer_) return;
    // Load cursor from theme and set
    // wl_cursor_theme_get_cursor(cursorTheme_, shapeName);
    // wl_pointer_set_cursor(pointer_, pointerSerial_, cursorSurface, hotX, hotY);
    (void)type;
}

std::string WaylandPlatform::clipboardText() const {
    return clipboardText_;
}

void WaylandPlatform::setClipboardText(const std::string& text) {
    clipboardText_ = text;
    // Create wl_data_source, set offer for text/plain, set selection
}

void WaylandPlatform::addDamageRect(int x, int y, int w, int h) {
    damageRects_.push_back({x, y, w, h});
}

void WaylandPlatform::commitDamage() {
    if (!surface_) return;
    if (damageRects_.empty()) {
        // Full surface damage
        // wl_surface_damage_buffer(surface_, 0, 0, width_ * scale_, height_ * scale_);
    } else {
        for (const auto& r : damageRects_) {
            // wl_surface_damage_buffer(surface_, r.x, r.y, r.w, r.h);
            (void)r;
        }
        damageRects_.clear();
    }
    // wl_surface_commit(surface_);
}

// ==================================================================
// Wayland listener callbacks — file-local statics
// These are registered as C callbacks via Wayland listener structs.
// ==================================================================

namespace {

static void wlRegistryGlobal(void* data, void* /*registry*/, uint32_t /*name*/,
                              const char* interface, uint32_t /*version*/) {
    auto* self = static_cast<WaylandPlatform*>(data);
    if (!self) return;

    if (std::strcmp(interface, "wl_compositor") == 0) {
        // self->compositor_ = wl_registry_bind(...)
    } else if (std::strcmp(interface, "xdg_wm_base") == 0) {
        // self->xdgWmBase_ = wl_registry_bind(...)
    } else if (std::strcmp(interface, "wl_seat") == 0) {
        // self->seat_ = wl_registry_bind(...)
    } else if (std::strcmp(interface, "wl_output") == 0) {
        // Bind output for scale factor
    } else if (std::strcmp(interface, "wp_fractional_scale_manager_v1") == 0) {
        // Fractional scaling
    } else if (std::strcmp(interface, "wp_viewporter") == 0) {
        // Viewport
    } else if (std::strcmp(interface, "zxdg_decoration_manager_v1") == 0) {
        // Server-side decorations
    } else if (std::strcmp(interface, "wp_idle_inhibit_manager_v1") == 0) {
        // Idle inhibit
    } else if (std::strcmp(interface, "wp_content_type_manager_v1") == 0) {
        // Content type hints
    }
}

static void wlRegistryGlobalRemove(void* /*data*/, void* /*registry*/, uint32_t /*name*/) {
}

static void wlXdgSurfaceConfigure(void* /*data*/, void* /*surface*/, uint32_t /*serial*/) {
    // xdg_surface_ack_configure(surface, serial);
}

static void wlXdgToplevelConfigure(void* data, void* /*toplevel*/,
                                     int32_t width, int32_t height, void* /*states*/) {
    auto* self = static_cast<WaylandPlatform*>(data);
    if (!self) return;
    (void)width; (void)height;
}

static void wlXdgToplevelClose(void* /*data*/, void* /*toplevel*/) {
}

static void wlPointerEnter(void* data, void* /*pointer*/, uint32_t /*serial*/,
                            void* /*surface*/, float /*x*/, float /*y*/) {
    (void)data;
}

static void wlPointerLeave(void* /*data*/, void* /*pointer*/,
                            uint32_t /*serial*/, void* /*surface*/) {}

static void wlPointerMotion(void* /*data*/, void* /*pointer*/, uint32_t /*time*/,
                              float /*x*/, float /*y*/) {}

static void wlPointerButton(void* /*data*/, void* /*pointer*/, uint32_t /*serial*/,
                              uint32_t /*time*/, uint32_t /*button*/, uint32_t /*state*/) {}

static void wlPointerAxis(void* /*data*/, void* /*pointer*/, uint32_t /*time*/,
                            uint32_t /*axis*/, float /*value*/) {}

static void wlKeyboardKeymap(void* /*data*/, void* /*keyboard*/,
                               uint32_t /*format*/, int32_t /*fd*/, uint32_t /*size*/) {}

static void wlKeyboardEnter(void* /*data*/, void* /*keyboard*/, uint32_t /*serial*/,
                              void* /*surface*/, void* /*keys*/) {}

static void wlKeyboardLeave(void* /*data*/, void* /*keyboard*/,
                              uint32_t /*serial*/, void* /*surface*/) {}

static void wlKeyboardKey(void* /*data*/, void* /*keyboard*/, uint32_t /*serial*/,
                            uint32_t /*time*/, uint32_t /*key*/, uint32_t /*state*/) {}

static void wlKeyboardModifiers(void* /*data*/, void* /*keyboard*/, uint32_t /*serial*/,
                                  uint32_t /*modsDepressed*/, uint32_t /*modsLatched*/,
                                  uint32_t /*modsLocked*/, uint32_t /*group*/) {}

static void wlTouchDown(void* /*data*/, void* /*touch*/, uint32_t /*serial*/,
                          uint32_t /*time*/, void* /*surface*/,
                          int32_t /*id*/, float /*x*/, float /*y*/) {}

static void wlTouchUp(void* /*data*/, void* /*touch*/, uint32_t /*serial*/,
                        uint32_t /*time*/, int32_t /*id*/) {}

static void wlTouchMotion(void* /*data*/, void* /*touch*/, uint32_t /*time*/,
                            int32_t /*id*/, float /*x*/, float /*y*/) {}

} // anonymous namespace

} // namespace NXRender

