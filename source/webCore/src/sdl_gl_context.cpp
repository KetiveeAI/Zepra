/**
 * @file sdl_gl_context.cpp
 * @brief SDL2 + OpenGL context integration implementation
 */

#include "webcore/sdl_gl_context.hpp"
#include <iostream>

namespace Zepra::WebCore {

// =============================================================================
// SDLGLContext Implementation
// =============================================================================

SDLGLContext::SDLGLContext() = default;

SDLGLContext::~SDLGLContext() {
    cleanup();
}

bool SDLGLContext::initialize(const std::string& title, int width, int height,
                               RenderMode mode) {
    width_ = width;
    height_ = height;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Try OpenGL first if requested
    bool useGL = (mode == RenderMode::OpenGL || mode == RenderMode::Auto);
    
#ifdef ZEPRA_HAS_OPENGL
    if (useGL) {
        // Set OpenGL attributes for 3.3 core
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        
        window_ = SDL_CreateWindow(
            title.c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width, height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
        );
        
        if (window_) {
            if (initOpenGL()) {
                currentMode_ = RenderMode::OpenGL;
                return true;
            }
            // OpenGL failed, fallback to software
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
    }
#endif
    
    // Fallback to software rendering
    if (mode != RenderMode::OpenGL) {
        window_ = SDL_CreateWindow(
            title.c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width, height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );
        
        if (window_ && initSoftware()) {
            currentMode_ = RenderMode::Software;
            return true;
        }
    }
    
    return false;
}

bool SDLGLContext::initOpenGL() {
#ifdef ZEPRA_HAS_OPENGL
    glContext_ = SDL_GL_CreateContext(window_);
    if (!glContext_) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Make context current
    SDL_GL_MakeCurrent(window_, glContext_);
    
    // Enable VSync
    SDL_GL_SetSwapInterval(1);
    
    // Create GLRenderBackend
    auto glBackend = std::make_unique<GLRenderBackend>();
    if (!glBackend->initialize()) {
        std::cerr << "Failed to initialize OpenGL backend" << std::endl;
        return false;
    }
    glBackend->resize(width_, height_);
    renderBackend_ = std::move(glBackend);
    
    std::cout << "Using OpenGL hardware acceleration" << std::endl;
    return true;
#else
    return false;
#endif
}

bool SDLGLContext::initSoftware() {
    sdlRenderer_ = SDL_CreateRenderer(window_, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!sdlRenderer_) {
        // Try software renderer
        sdlRenderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }
    
    if (!sdlRenderer_) {
        return false;
    }
    
    // Create SDL software backend
    auto softBackend = std::make_unique<SDLSoftwareBackend>(sdlRenderer_);
    softBackend->resize(width_, height_);
    renderBackend_ = std::move(softBackend);
    
    std::cout << "Using software rendering" << std::endl;
    return true;
}

void SDLGLContext::cleanup() {
    renderBackend_.reset();
    
#ifdef ZEPRA_HAS_OPENGL
    if (glContext_) {
        SDL_GL_DeleteContext(glContext_);
        glContext_ = nullptr;
    }
#endif
    
    if (sdlRenderer_) {
        SDL_DestroyRenderer(sdlRenderer_);
        sdlRenderer_ = nullptr;
    }
    
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
}

void SDLGLContext::beginFrame() {
    // Clear the screen at start of frame
    if (currentMode_ == RenderMode::Software && sdlRenderer_) {
        SDL_SetRenderDrawColor(sdlRenderer_, 255, 255, 255, 255);
        SDL_RenderClear(sdlRenderer_);
    }
}

void SDLGLContext::endFrame() {
    // Present to screen
    if (renderBackend_) {
        renderBackend_->present();
    }
    
#ifdef ZEPRA_HAS_OPENGL
    if (currentMode_ == RenderMode::OpenGL && glContext_) {
        SDL_GL_SwapWindow(window_);
    }
#endif
}

void SDLGLContext::resize(int width, int height) {
    width_ = width;
    height_ = height;
    
    if (renderBackend_) {
        renderBackend_->resize(width, height);
    }
}

void SDLGLContext::getSize(int& width, int& height) const {
    width = width_;
    height = height_;
}

std::unique_ptr<DisplayList> SDLGLContext::createDisplayList() {
    return std::make_unique<DisplayList>();
}

// =============================================================================
// SDLSoftwareBackend Implementation
// =============================================================================

SDLSoftwareBackend::SDLSoftwareBackend(SDL_Renderer* renderer)
    : renderer_(renderer) {}

void SDLSoftwareBackend::executeDisplayList(const DisplayList& list) {
    // Execute each draw command from the display list
    for (const auto& cmd : list.commands()) {
        switch (cmd.type) {
            case PaintCommandType::FillRect:
                SDL_SetRenderDrawColor(renderer_,
                    static_cast<uint8_t>(cmd.color.r * 255),
                    static_cast<uint8_t>(cmd.color.g * 255),
                    static_cast<uint8_t>(cmd.color.b * 255),
                    static_cast<uint8_t>(cmd.color.a * 255));
                {
                    SDL_Rect sdlRect = {
                        static_cast<int>(cmd.rect.x),
                        static_cast<int>(cmd.rect.y),
                        static_cast<int>(cmd.rect.width),
                        static_cast<int>(cmd.rect.height)
                    };
                    SDL_RenderFillRect(renderer_, &sdlRect);
                }
                break;
                
            case PaintCommandType::StrokeRect:
                SDL_SetRenderDrawColor(renderer_,
                    static_cast<uint8_t>(cmd.color.r * 255),
                    static_cast<uint8_t>(cmd.color.g * 255),
                    static_cast<uint8_t>(cmd.color.b * 255),
                    static_cast<uint8_t>(cmd.color.a * 255));
                {
                    SDL_Rect sdlRect = {
                        static_cast<int>(cmd.rect.x),
                        static_cast<int>(cmd.rect.y),
                        static_cast<int>(cmd.rect.width),
                        static_cast<int>(cmd.rect.height)
                    };
                    SDL_RenderDrawRect(renderer_, &sdlRect);
                }
                break;
                
            case PaintCommandType::DrawText:
                // Text rendering placeholder - draws colored box
                SDL_SetRenderDrawColor(renderer_,
                    static_cast<uint8_t>(cmd.color.r * 255),
                    static_cast<uint8_t>(cmd.color.g * 255),
                    static_cast<uint8_t>(cmd.color.b * 255),
                    static_cast<uint8_t>(cmd.color.a * 255));
                {
                    float textWidth = cmd.text.length() * cmd.number * 0.6f;
                    SDL_Rect textRect = {
                        static_cast<int>(cmd.rect.x),
                        static_cast<int>(cmd.rect.y),
                        static_cast<int>(textWidth),
                        static_cast<int>(cmd.number)
                    };
                    SDL_RenderFillRect(renderer_, &textRect);
                }
                break;
                
            case PaintCommandType::PushClip:
                {
                    SDL_Rect sdlRect = {
                        static_cast<int>(cmd.rect.x),
                        static_cast<int>(cmd.rect.y),
                        static_cast<int>(cmd.rect.width),
                        static_cast<int>(cmd.rect.height)
                    };
                    clipStack_.push_back(sdlRect);
                    SDL_RenderSetClipRect(renderer_, &sdlRect);
                }
                break;
                
            case PaintCommandType::PopClip:
                if (!clipStack_.empty()) {
                    clipStack_.pop_back();
                }
                if (clipStack_.empty()) {
                    SDL_RenderSetClipRect(renderer_, nullptr);
                } else {
                    SDL_RenderSetClipRect(renderer_, &clipStack_.back());
                }
                break;
                
            default:
                break;
        }
    }
}

void SDLSoftwareBackend::present() {
    SDL_RenderPresent(renderer_);
}

void SDLSoftwareBackend::resize(int width, int height) {
    width_ = width;
    height_ = height;
}

void* SDLSoftwareBackend::createTexture(int width, int height) {
    SDL_Texture* texture = SDL_CreateTexture(renderer_,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STATIC,
        width, height);
    
    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    }
    
    return texture;
}

void SDLSoftwareBackend::destroyTexture(void* texture) {
    if (texture) {
        SDL_DestroyTexture(static_cast<SDL_Texture*>(texture));
    }
}

} // namespace Zepra::WebCore

