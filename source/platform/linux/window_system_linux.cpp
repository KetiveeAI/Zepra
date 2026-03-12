// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file window_system_linux.cpp
 * @brief Linux window system implementation using SDL2
 */

#include "platform/window_system.hpp"
#include <SDL2/SDL.h>
#include <unordered_map>

namespace Zepra::Platform {

struct WindowHandle {
    SDL_Window* window = nullptr;
    int id = 0;
    
    WindowSystem::ResizeCallback onResize;
    WindowSystem::CloseCallback onClose;
    WindowSystem::FocusCallback onFocus;
};

static std::unordered_map<int, WindowHandle*> g_windows;
static int g_nextWindowId = 1;

std::unique_ptr<WindowSystem> WindowSystem::create() {
    return std::make_unique<WindowSystemLinux>();
}

WindowHandle* WindowSystemLinux::createWindow(const WindowConfig& config) {
    auto* handle = new WindowHandle();
    handle->id = g_nextWindowId++;
    
    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
    if (config.resizable) flags |= SDL_WINDOW_RESIZABLE;
    if (!config.decorated) flags |= SDL_WINDOW_BORDERLESS;
    if (config.maximized) flags |= SDL_WINDOW_MAXIMIZED;
    if (config.fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    
    int x = (config.x < 0) ? SDL_WINDOWPOS_CENTERED : config.x;
    int y = (config.y < 0) ? SDL_WINDOWPOS_CENTERED : config.y;
    
    handle->window = SDL_CreateWindow(
        config.title.c_str(),
        x, y,
        config.width, config.height,
        flags
    );
    
    if (!handle->window) {
        delete handle;
        return nullptr;
    }
    
    g_windows[handle->id] = handle;
    return handle;
}

void WindowSystemLinux::destroyWindow(WindowHandle* window) {
    if (!window) return;
    
    if (window->window) {
        SDL_DestroyWindow(window->window);
    }
    
    g_windows.erase(window->id);
    delete window;
}

void WindowSystemLinux::showWindow(WindowHandle* window) {
    if (window && window->window) {
        SDL_ShowWindow(window->window);
    }
}

void WindowSystemLinux::hideWindow(WindowHandle* window) {
    if (window && window->window) {
        SDL_HideWindow(window->window);
    }
}

void WindowSystemLinux::minimizeWindow(WindowHandle* window) {
    if (window && window->window) {
        SDL_MinimizeWindow(window->window);
    }
}

void WindowSystemLinux::maximizeWindow(WindowHandle* window) {
    if (window && window->window) {
        SDL_MaximizeWindow(window->window);
    }
}

void WindowSystemLinux::restoreWindow(WindowHandle* window) {
    if (window && window->window) {
        SDL_RestoreWindow(window->window);
    }
}

void WindowSystemLinux::setFullscreen(WindowHandle* window, bool fullscreen) {
    if (window && window->window) {
        SDL_SetWindowFullscreen(window->window, 
            fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    }
}

void WindowSystemLinux::setTitle(WindowHandle* window, const std::string& title) {
    if (window && window->window) {
        SDL_SetWindowTitle(window->window, title.c_str());
    }
}

void WindowSystemLinux::setSize(WindowHandle* window, int width, int height) {
    if (window && window->window) {
        SDL_SetWindowSize(window->window, width, height);
    }
}

void WindowSystemLinux::setPosition(WindowHandle* window, int x, int y) {
    if (window && window->window) {
        SDL_SetWindowPosition(window->window, x, y);
    }
}

void WindowSystemLinux::getSize(WindowHandle* window, int& width, int& height) {
    if (window && window->window) {
        SDL_GetWindowSize(window->window, &width, &height);
    }
}

void WindowSystemLinux::getPosition(WindowHandle* window, int& x, int& y) {
    if (window && window->window) {
        SDL_GetWindowPosition(window->window, &x, &y);
    }
}

void WindowSystemLinux::focusWindow(WindowHandle* window) {
    if (window && window->window) {
        SDL_RaiseWindow(window->window);
    }
}

bool WindowSystemLinux::isFocused(WindowHandle* window) {
    if (window && window->window) {
        Uint32 flags = SDL_GetWindowFlags(window->window);
        return (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
    }
    return false;
}

void* WindowSystemLinux::getNativeHandle(WindowHandle* window) {
    if (window) {
        return window->window;
    }
    return nullptr;
}

void WindowSystemLinux::setOnResize(WindowHandle* window, ResizeCallback callback) {
    if (window) {
        window->onResize = std::move(callback);
    }
}

void WindowSystemLinux::setOnClose(WindowHandle* window, CloseCallback callback) {
    if (window) {
        window->onClose = std::move(callback);
    }
}

void WindowSystemLinux::setOnFocus(WindowHandle* window, FocusCallback callback) {
    if (window) {
        window->onFocus = std::move(callback);
    }
}

} // namespace Zepra::Platform
