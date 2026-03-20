// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "../../source/zepraEngine/include/engine/video_player.h"
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
#include <SDL2/SDL.h>
#include <GL/gl.h>
#ifdef __linux__
#include <dlfcn.h>
#endif

namespace zepra {

VideoPlayer::VideoPlayer() : gpuEnabled(true), cudaEnabled(false), backend(VideoBackend::OpenGL) {}
VideoPlayer::~VideoPlayer() {}

bool VideoPlayer::load(const std::string& filePath) {
    std::cout << "[VideoPlayer] Loading video: " << filePath << std::endl;
    // TODO: Implement video file loading and decoding
    return true;
}

bool VideoPlayer::play() {
    std::cout << "[VideoPlayer] Play" << std::endl;
    // TODO: Implement playback
    return true;
}

bool VideoPlayer::pause() {
    std::cout << "[VideoPlayer] Pause" << std::endl;
    // TODO: Implement pause
    return true;
}

bool VideoPlayer::stop() {
    std::cout << "[VideoPlayer] Stop" << std::endl;
    // TODO: Implement stop
    return true;
}

bool VideoPlayer::seek(double seconds) {
    std::cout << "[VideoPlayer] Seek to " << seconds << "s" << std::endl;
    // TODO: Implement seek
    return true;
}

double VideoPlayer::getDuration() const { return 0.0; }
double VideoPlayer::getCurrentTime() const { return 0.0; }
bool VideoPlayer::isPlaying() const { return false; }

bool VideoPlayer::enableGpuAcceleration(bool enable) {
    gpuEnabled = enable;
    backend = enable ? VideoBackend::OpenGL : VideoBackend::CPU;
    std::cout << (enable ? "[VideoPlayer] GPU acceleration enabled" : "[VideoPlayer] GPU acceleration disabled") << std::endl;
    return true;
}

bool VideoPlayer::isGpuAccelerationEnabled() const { return gpuEnabled; }

std::string VideoPlayer::getGpuInfo() const {
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

// CUDA/NVDEC support (stub)
bool VideoPlayer::isCudaAvailable() const {
#ifdef __linux__
    void* handle = dlopen("libcuda.so", RTLD_LAZY);
    bool available = (handle != nullptr);
    if (handle) dlclose(handle);
    return available;
#elif defined(_WIN32)
    HMODULE h = LoadLibraryA("nvcuda.dll");
    bool available = (h != nullptr);
    if (h) FreeLibrary(h);
    return available;
#else
    return false;
#endif
}

bool VideoPlayer::enableCuda(bool enable) {
    if (enable && isCudaAvailable()) {
        cudaEnabled = true;
        backend = VideoBackend::CUDA;
        std::cout << "[VideoPlayer] CUDA/NVDEC acceleration enabled" << std::endl;
        // TODO: Integrate with real CUDA/NVDEC decode
    } else {
        cudaEnabled = false;
        backend = gpuEnabled ? VideoBackend::OpenGL : VideoBackend::CPU;
        std::cout << "[VideoPlayer] CUDA/NVDEC acceleration disabled" << std::endl;
    }
    return cudaEnabled;
}

bool VideoPlayer::isCudaEnabled() const { return cudaEnabled; }

VideoBackend VideoPlayer::getActiveBackend() const { return backend; }

void VideoPlayer::setOnFrameCallback(std::function<void()> cb) { onFrameCallback = cb; }
void VideoPlayer::setOnEndCallback(std::function<void()> cb) { onEndCallback = cb; }

} // namespace zepra 