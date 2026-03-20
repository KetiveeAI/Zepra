// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "../../source/zepraEngine/include/engine/gpu_manager.h"
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
#include <SDL2/SDL.h>
#include <GL/gl.h>

namespace zepra {

GpuManager::GpuManager() {}
GpuManager::~GpuManager() {}

bool GpuManager::isGpuAvailable() const {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;
    SDL_Window* window = SDL_CreateWindow("GPU Check", 0, 0, 1, 1, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window) return false;
    SDL_GLContext context = SDL_GL_CreateContext(window);
    bool available = (context != nullptr);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return available;
}

bool GpuManager::enableGpuAcceleration(bool enable) {
    // In real browser, this would toggle GPU pipeline. Here, just log.
    std::cout << (enable ? "[GPU] Acceleration enabled" : "[GPU] Acceleration disabled") << std::endl;
    return true;
}

bool GpuManager::isGpuAccelerationEnabled() const {
    // Stub: always true for now
    return true;
}

std::string GpuManager::getGpuInfo() const {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return "No GPU";
    SDL_Window* window = SDL_CreateWindow("GPU Info", 0, 0, 1, 1, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window) return "No GPU";
    SDL_GLContext context = SDL_GL_CreateContext(window);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    std::string info = "Vendor: ";
    info += vendor ? reinterpret_cast<const char*>(vendor) : "?";
    info += ", Renderer: ";
    info += renderer ? reinterpret_cast<const char*>(renderer) : "?";
    info += ", Version: ";
    info += version ? reinterpret_cast<const char*>(version) : "?";
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return info;
}

// Video decode support (stub)
bool GpuManager::isVideoDecodeSupported() const {
    // TODO: Integrate with platform-specific video decode APIs
    return true;
}

} // namespace zepra 