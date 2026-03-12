/**
 * @file nxgfx_context.hpp
 * @brief NXGFX context integration for rendering - SDL replacement
 * 
 * This replaces SDLGLContext with our native NXGFX backend.
 * No SDL dependency.
 */

#pragma once

#include "webcore/rendering/gl_render_backend.hpp"
#include "rendering/paint_context.hpp"

// NXRender C++ API
#include "nxgfx/context.h"
// If input is still C API, keep nxrender.h for input structs
extern "C" {
#include "nxrender.h"
}

#include <memory>
#include <string>

namespace Zepra::WebCore {

/**
 * @brief Rendering mode selection
 */
enum class NXRenderMode {
    Software,   // CPU rendering (fallback)
    GPU,        // GPU rendering via NXGFX
    Auto        // Auto-detect best available
};

/**
 * @brief NXGFX context manager - replaces SDLGLContext
 * 
 * Creates native window with GPU context via NXGFX (Rust)
 */
class NXGFXContext {
public:
    NXGFXContext();
    ~NXGFXContext();
    
    /**
     * @brief Initialize NXGFX and create window
     * @param title Window title
     * @param width Window width
     * @param height Window height
     * @param mode Rendering mode preference
     * @return true on success
     */
    bool initialize(const std::string& title, int width, int height, 
                    NXRenderMode mode = NXRenderMode::Auto);
    
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
     * @brief Get current render mode
     */
    NXRenderMode mode() const { return currentMode_; }
    
    /**
     * @brief Check if using hardware acceleration
     */
    bool isHardwareAccelerated() const { return currentMode_ == NXRenderMode::GPU; }
    
    /**
     * @brief Create a DisplayList for rendering
     */
    std::unique_ptr<DisplayList> createDisplayList();
    
    // Input state (replaces SDL_Event polling)
    NxMouseState* mouse() { return mouse_; }
    NxKeyboardState* keyboard() { return keyboard_; }
    NxTouchState* touch() { return touch_; }
    NxTheme* theme() { return theme_; }
    
private:
    bool initGPU();
    void cleanup();
    // NXGfx state
    NXRender::GpuContext* gpu_ = nullptr;
    NxMouseState* mouse_ = nullptr;
    NxKeyboardState* keyboard_ = nullptr;
    NxTouchState* touch_ = nullptr;
    NxTheme* theme_ = nullptr;
    
    std::unique_ptr<RenderBackend> renderBackend_;
    NXRenderMode currentMode_ = NXRenderMode::Software;
    
    int width_ = 0;
    int height_ = 0;
};

/**
 * @brief NXGFX backend that implements RenderBackend interface
 */
class NXGFXRenderBackend : public RenderBackend {
public:
    explicit NXGFXRenderBackend(NxGpuContext* gpu);
    
    NXGFXRenderBackend(NXRender::GpuContext* gpu);
    
    // RenderBackend interface
    void executeDisplayList(const DisplayList& list) override;
    void present() override;
    void resize(int width, int height) override;
    
    // Texture management
    void* createTexture(int width, int height) override;
    void destroyTexture(void* texture) override;
    
private:
    NXRender::GpuContext* gpu_; 
    int width_ = 0;
    int height_ = 0;
};

} // namespace Zepra::WebCore
