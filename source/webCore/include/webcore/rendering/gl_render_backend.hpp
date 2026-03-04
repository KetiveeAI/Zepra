/**
 * @file gl_render_backend.hpp
 * @brief OpenGL GPU render backend for hardware-accelerated painting
 */

#pragma once

#include "paint_context.hpp"
#include <memory>
#include <unordered_map>
#include <SDL2/SDL_ttf.h>

namespace Zepra::WebCore {

/**
 * @brief OpenGL shader program wrapper
 */
struct ShaderProgram {
    unsigned int id = 0;
    int posLocation = -1;
    int colorLocation = -1;
    int texCoordLocation = -1;
    int projectionLocation = -1;
    int textureLocation = -1;
};

/**
 * @brief OpenGL texture info
 */
struct GLTexture {
    unsigned int id = 0;
    int width = 0;
    int height = 0;
};

/**
 * @brief OpenGL GPU rendering backend
 * 
 * Implements hardware-accelerated 2D rendering using OpenGL 3.3 Core Profile.
 * Renders DisplayList commands using batched draw calls for performance.
 */
class GLRenderBackend : public RenderBackend {
public:
    GLRenderBackend();
    ~GLRenderBackend() override;
    
    /**
     * @brief Initialize OpenGL context (call after GL context is created)
     */
    bool initialize();
    
    /**
     * @brief Shutdown and cleanup resources
     */
    void shutdown();
    
    // RenderBackend interface
    void executeDisplayList(const DisplayList& list) override;
    void present() override;
    void resize(int width, int height) override;
    void* createTexture(int width, int height) override;
    void destroyTexture(void* texture) override;
    
    // Additional GPU features
    void setClearColor(const Color& color);
    void clear();
    
    // Compositing layers (for hardware acceleration)
    void beginLayer(float opacity = 1.0f);
    void endLayer();
    
    // Stats
    int drawCallCount() const { return drawCallCount_; }
    int triangleCount() const { return triangleCount_; }
    
private:
    bool createShaders();
    void destroyShaders();
    void createBuffers();
    void destroyBuffers();
    
    // Batch rendering
    void flushBatch();
    void addQuad(const Rect& rect, const Color& color, const Rect* texCoords = nullptr);
    
    // Command execution
    void executeFillRect(const PaintCommand& cmd);
    void executeStrokeRect(const PaintCommand& cmd);
    void executeDrawText(const PaintCommand& cmd);
    void executeDrawImage(const PaintCommand& cmd);
    void executeDrawTexture(const PaintCommand& cmd);
    void executePushClip(const PaintCommand& cmd);
    void executePopClip();
    
    bool initialized_ = false;
    int viewportWidth_ = 800;
    int viewportHeight_ = 600;
    
    // Shaders
    ShaderProgram solidShader_;
    ShaderProgram textureShader_;
    
    // Buffers
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int ebo_ = 0;

    // Texture buffers
    unsigned int textureVao_ = 0;
    unsigned int textureVbo_ = 0;
    unsigned int textureEbo_ = 0;
    
    // Batching
    static constexpr int MAX_QUADS = 10000;
    std::vector<float> vertices_;
    std::vector<unsigned int> indices_;
    int quadCount_ = 0;
    
    // Texture cache
    std::unordered_map<std::string, GLTexture> textureCache_;
    
    // Clip stack
    std::vector<Rect> clipStack_;
    
    // Stats
    int drawCallCount_ = 0;
    int triangleCount_ = 0;
    
    // Clear color
    Color clearColor_ = Color::white();
    
    // System font
    TTF_Font* font_ = nullptr;
    
    // Compositor layers
    std::vector<unsigned int> layerFramebuffers_;
    float currentLayerOpacity_ = 1.0f;
};

/**
 * @brief Software fallback render backend (for systems without GPU)
 */
class SoftwareRenderBackend : public RenderBackend {
public:
    SoftwareRenderBackend(int width, int height);
    ~SoftwareRenderBackend() override;
    
    void executeDisplayList(const DisplayList& list) override;
    void present() override;
    void resize(int width, int height) override;
    void* createTexture(int width, int height) override;
    void destroyTexture(void* texture) override;
    
    // Access pixel buffer
    const std::vector<uint32_t>& pixels() const { return pixels_; }
    int width() const { return width_; }
    int height() const { return height_; }
    
private:
    void fillRect(const Rect& rect, const Color& color);
    void strokeRect(const Rect& rect, const Color& color, float width);
    void drawText(const std::string& text, float x, float y, const Color& color, float fontSize);
    
    std::vector<uint32_t> pixels_;
    int width_;
    int height_;
    std::vector<Rect> clipStack_;
};

} // namespace Zepra::WebCore
