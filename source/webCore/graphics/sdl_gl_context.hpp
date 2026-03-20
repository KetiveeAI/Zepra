/**
 * @file sdl_gl_context.hpp
 * @brief SDL2 + OpenGL context integration for GLRenderBackend
 */

#pragma once

#include "rendering/gl_render_backend.hpp"
#include "rendering/paint_context.hpp"
#include <SDL2/SDL.h>

#ifdef ZEPRA_HAS_OPENGL
#include <SDL2/SDL_opengl.h>
#endif

#include <memory>
#include <string>

namespace Zepra::WebCore {

/**
 * @brief Rendering mode selection
 */
enum class RenderMode {
    Software,   // CPU rendering via SDL_Renderer
    OpenGL,     // GPU rendering via OpenGL 3.3
    Auto        // Auto-detect best available
};

/**
 * @brief SDL + OpenGL context manager
 * 
 * Creates SDL window with OpenGL context and bridges to GLRenderBackend
 */
class SDLGLContext {
public:
    SDLGLContext();
    ~SDLGLContext();
    
    /**
     * @brief Initialize SDL and create window
     * @param title Window title
     * @param width Window width
     * @param height Window height
     * @param mode Rendering mode preference
     * @return true on success
     */
    bool initialize(const std::string& title, int width, int height, 
                    RenderMode mode = RenderMode::Auto);
    
    /**
     * @brief Get the render backend for this context
     */
    RenderBackend* renderBackend() { return renderBackend_.get(); }
    
    /**
     * @brief Begin a frame (clear, setup)
     */
    void beginFrame();
    
    /**
     * @brief End a frame (present)
     */
    void endFrame();
    
    /**
     * @brief Resize the rendering surface
     */
    void resize(int width, int height);
    
    /**
     * @brief Get current window size
     */
    void getSize(int& width, int& height) const;
    
    /**
     * @brief Get SDL window handle
     */
    SDL_Window* window() { return window_; }
    
    /**
     * @brief Get current render mode
     */
    RenderMode mode() const { return currentMode_; }
    
    /**
     * @brief Check if using hardware acceleration
     */
    bool isHardwareAccelerated() const { return currentMode_ == RenderMode::OpenGL; }
    
    /**
     * @brief Create a DisplayList for rendering
     */
    std::unique_ptr<DisplayList> createDisplayList();
    
private:
    bool initOpenGL();
    bool initSoftware();
    void cleanup();
    
    SDL_Window* window_ = nullptr;
    SDL_GLContext glContext_ = nullptr;
    SDL_Renderer* sdlRenderer_ = nullptr;  // Fallback for software mode
    
    std::unique_ptr<RenderBackend> renderBackend_;
    RenderMode currentMode_ = RenderMode::Software;
    
    int width_ = 0;
    int height_ = 0;
};

/**
 * @brief SDL backend that wraps SDL_Renderer for software rendering
 */
class SDLSoftwareBackend : public RenderBackend {
public:
    explicit SDLSoftwareBackend(SDL_Renderer* renderer);
    
    // RenderBackend interface
    void executeDisplayList(const DisplayList& list) override;
    void present() override;
    void resize(int width, int height) override;
    void* createTexture(int width, int height) override;
    void destroyTexture(void* texture) override;
    
private:
    SDL_Renderer* renderer_ = nullptr;
    std::vector<SDL_Rect> clipStack_;
    int width_ = 0;
    int height_ = 0;
};

} // namespace Zepra::WebCore
