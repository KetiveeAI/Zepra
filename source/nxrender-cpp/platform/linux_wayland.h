// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "platform/platform.h"
#include "nxgfx/primitives.h"
#include <string>
#include <functional>
#include <vector>
#include <cstdint>

namespace NXRender {

// ==================================================================
// Wayland platform backend
// ==================================================================

class WaylandPlatform : public Platform {
public:
    WaylandPlatform();
    ~WaylandPlatform() override;

    // Platform interface
    bool init(int width, int height, const std::string& title) override;
    void shutdown() override;
    void pollEvents() override;
    void swapBuffers() override;
    void setTitle(const std::string& title) override;
    void setCursor(CursorType type) override;

    // Extended API
    bool isInitialized() const { return initialized_; }
    int windowWidth() const { return width_; }
    int windowHeight() const { return height_; }
    float devicePixelRatio() const { return scale_; }
    void setWindowSize(int w, int h);
    void setCursorVisible(bool visible);

    // Clipboard
    std::string clipboardText() const;
    void setClipboardText(const std::string& text);

    // Wayland native handles
    void* display() const { return display_; }
    void* surface() const { return surface_; }
    void* eglSurface() const { return eglSurface_; }

    // Fractional scaling
    void setScale(float scale) { scale_ = scale; }

    // Damage tracking
    void addDamageRect(int x, int y, int w, int h);
    void commitDamage();

    // Surface roles
    enum class Role { Toplevel, Popup, Subsurface };
    void setRole(Role role) { role_ = role; }

    // XDG decoration
    void setServerSideDecorations(bool enabled) { serverSideDecor_ = enabled; }
    bool hasServerSideDecorations() const { return serverSideDecor_; }

    // Content type hint
    enum class ContentType { None, Photo, Video, Game };
    void setContentType(ContentType type) { contentType_ = type; }

    // Idle inhibit
    void setIdleInhibit(bool inhibit) { idleInhibit_ = inhibit; }
    bool idleInhibit() const { return idleInhibit_; }

private:
    bool initialized_ = false;
    void* display_ = nullptr;
    void* registry_ = nullptr;
    void* compositor_ = nullptr;
    void* surface_ = nullptr;
    void* xdgWmBase_ = nullptr;
    void* xdgSurface_ = nullptr;
    void* xdgToplevel_ = nullptr;
    void* seat_ = nullptr;
    void* keyboard_ = nullptr;
    void* pointer_ = nullptr;
    void* touch_ = nullptr;
    void* eglDisplay_ = nullptr;
    void* eglContext_ = nullptr;
    void* eglSurface_ = nullptr;
    void* eglConfig_ = nullptr;
    void* eglWindow_ = nullptr;

    int width_ = 800, height_ = 600;
    float scale_ = 1.0f;
    Role role_ = Role::Toplevel;
    bool serverSideDecor_ = false;
    ContentType contentType_ = ContentType::None;
    bool idleInhibit_ = false;
    std::string clipboardText_;

    struct DamageRect { int x, y, w, h; };
    std::vector<DamageRect> damageRects_;

    float pointerX_ = 0, pointerY_ = 0;
    uint32_t pointerSerial_ = 0;
    uint32_t keyboardSerial_ = 0;

    bool initEGL();
    void cleanupEGL();
};

} // namespace NXRender
