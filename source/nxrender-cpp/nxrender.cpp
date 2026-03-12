// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file nxrender.cpp
 * @brief NXRender main framework implementation
 */

#include "nxrender_cpp.h"
#include "platform/platform.h" // Platform interface
#include <memory>
#include <iostream>

namespace NXRender {

static std::unique_ptr<GpuContext> g_gpu;
static std::unique_ptr<Compositor> g_compositor;
static std::unique_ptr<Platform> g_platform;
static EventCallback g_eventHandler;
static std::function<void()> g_renderCallback;
static int g_width = 0;
static int g_height = 0;
static bool g_running = false;

// Internal dispatcher called by Platform
void dispatchEvent(const Event& event) {
    if (event.type == EventType::Close) {
        g_running = false;
        // Don't forward close event, we handle it
        return;
    }
    
    // Pass to handler
    if (g_eventHandler) {
        g_eventHandler(event);
    }
    
    // Pass to widgets/compositor if needed (future)
}

bool init(int width, int height) {
    g_width = width;
    g_height = height;
    
    // Create Platform Window
    g_platform.reset(createPlatform());
    if (!g_platform->init(width, height, "Zepra Browser")) {
        return false;
    }
    
    // Create GPU context
    g_gpu = std::make_unique<GpuContext>();
    if (!g_gpu->init(width, height)) {
        return false;
    }
    
    // Create compositor
    g_compositor = std::make_unique<Compositor>();
    if (!g_compositor->init(g_gpu.get())) {
        return false;
    }
    
    // Initialize default theme
    setTheme(Theme::light());
    
    g_running = true;
    return true;
}

void shutdown() {
    g_running = false;
    g_compositor.reset();
    g_gpu.reset();
    if (g_platform) {
        g_platform->shutdown();
        g_platform.reset();
    }
}

Compositor* compositor() {
    return g_compositor.get();
}

Theme* theme() {
    return currentTheme();
}

void setEventHandler(EventCallback handler) {
    g_eventHandler = handler;
}

void setRenderCallback(std::function<void()> callback) {
    g_renderCallback = callback;
}

void processEvents() {
    if (g_platform) {
        g_platform->pollEvents();
    }
}

void render() {
    if (!g_compositor) return;
    
    g_compositor->beginFrame();
    g_compositor->render(); 
    
    // User callback
    if (g_renderCallback) {
        g_renderCallback();
    }
                            
    g_compositor->endFrame();
    
    if (g_platform) {
        g_platform->swapBuffers();
    }
}

void run() {
    while (g_running) {
        processEvents();
        render();
    }
}

} // namespace NXRender
