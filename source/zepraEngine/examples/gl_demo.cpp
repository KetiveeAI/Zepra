/**
 * @file gl_demo.cpp
 * @brief Demonstration of ZepraBrowser OpenGL rendering capabilities
 * 
 * This demo shows:
 * - SDLGLContext initialization with auto-fallback
 * - DisplayList rendering with PaintCommands
 * - Animated rectangles and color gradients
 */

#include "webcore/sdl_gl_context.hpp"
#include "webcore/paint_context.hpp"
#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>
#include <chrono>

using namespace Zepra::WebCore;

/**
 * @brief Convert float (0-1) to uint8_t (0-255)
 */
inline uint8_t toU8(float f) {
    return static_cast<uint8_t>(std::clamp(f, 0.0f, 1.0f) * 255);
}

/**
 * @brief Create Color from float values (0-1 range)
 */
inline Color colorF(float r, float g, float b, float a = 1.0f) {
    return {toU8(r), toU8(g), toU8(b), toU8(a)};
}

/**
 * @brief Convert HSV to RGB Color
 */
Color hueToRGB(float hue) {
    float r, g, b;
    int i = static_cast<int>(hue * 6);
    float f = hue * 6 - i;
    float q = 1 - f;
    
    switch (i % 6) {
        case 0: r = 1; g = f; b = 0; break;
        case 1: r = q; g = 1; b = 0; break;
        case 2: r = 0; g = 1; b = f; break;
        case 3: r = 0; g = q; b = 1; break;
        case 4: r = f; g = 0; b = 1; break;
        case 5: r = 1; g = 0; b = q; break;
        default: r = g = b = 0;
    }
    
    return colorF(r, g, b);
}

/**
 * @brief Animated demo scene
 */
class GLDemo {
public:
    bool init() {
        // Try to create OpenGL context (falls back to software if unavailable)
        if (!context_.initialize("ZepraBrowser - OpenGL Demo", 1024, 768, RenderMode::Auto)) {
            std::cerr << "Failed to initialize rendering context" << std::endl;
            return false;
        }
        
        std::cout << "Render mode: " 
                  << (context_.isHardwareAccelerated() ? "OpenGL (GPU)" : "Software (CPU)") 
                  << std::endl;
        
        return true;
    }
    
    void run() {
        bool running = true;
        SDL_Event event;
        auto startTime = std::chrono::steady_clock::now();
        
        while (running) {
            // Handle events
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    context_.resize(event.window.data1, event.window.data2);
                }
            }
            
            // Calculate animation time
            auto now = std::chrono::steady_clock::now();
            float t = std::chrono::duration<float>(now - startTime).count();
            
            // Render frame
            render(t);
            
            SDL_Delay(16); // ~60 FPS
        }
    }
    
private:
    void render(float time) {
        int width, height;
        context_.getSize(width, height);
        
        // Create display list for this frame
        auto displayList = context_.createDisplayList();
        PaintContext ctx(*displayList);
        
        // Clear background with gradient-like effect
        float bgShade = 0.1f + 0.05f * std::sin(time * 0.5f);
        ctx.fillRect({0, 0, static_cast<float>(width), static_cast<float>(height)}, 
                     colorF(bgShade, bgShade, bgShade + 0.1f));
        
        // Draw animated header bar
        ctx.fillRect({0, 0, static_cast<float>(width), 60}, colorF(0.15f, 0.15f, 0.18f));
        
        // Animated logo area
        float logoX = width / 2.0f - 100 + std::sin(time) * 5;
        ctx.fillRect({logoX, 15, 200, 30}, 
                     colorF(0.4f + 0.1f * std::sin(time * 2), 
                            0.6f + 0.1f * std::sin(time * 2.5f), 
                            1.0f));
        
        // Bouncing rectangles
        for (int i = 0; i < 5; i++) {
            float phase = time + i * 0.8f;
            float x = 100 + i * 180 + std::sin(phase) * 20;
            float y = 150 + std::abs(std::sin(phase * 1.5f)) * 100;
            float size = 80 + std::sin(phase * 2) * 20;
            
            // Rainbow colors
            float hue = std::fmod(time * 0.3f + i * 0.2f, 1.0f);
            Color color = hueToRGB(hue);
            
            ctx.fillRect({x, y, size, size}, color);
            
            // Border
            ctx.strokeRect({x - 2, y - 2, size + 4, size + 4}, colorF(1.0f, 1.0f, 1.0f, 0.5f), 2.0f);
        }
        
        // Animated wave at bottom
        float waveY = height - 100.0f;
        for (int i = 0; i < width; i += 20) {
            float h = 30 + std::sin(time * 2 + i * 0.05f) * 20;
            ctx.fillRect({static_cast<float>(i), waveY + 50 - h, 18, h}, colorF(0.2f, 0.5f, 0.8f, 0.7f));
        }
        
        // FPS counter area
        ctx.fillRect({width - 120.0f, 10, 110, 40}, colorF(0, 0, 0, 0.7f));
        ctx.strokeRect({width - 120.0f, 10, 110, 40}, colorF(0.5f, 0.5f, 0.5f), 1.0f);
        
        // Render mode indicator
        float indicatorY = height - 30.0f;
        Color modeColor = context_.isHardwareAccelerated() 
                        ? colorF(0.2f, 0.8f, 0.2f) 
                        : colorF(0.8f, 0.6f, 0.2f);
        ctx.fillRect({10, indicatorY, 20, 20}, modeColor);
        
        // Execute the display list
        context_.beginFrame();
        if (context_.renderBackend()) {
            context_.renderBackend()->executeDisplayList(*displayList);
        }
        context_.endFrame();
    }
    
    SDLGLContext context_;
};

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    std::cout << "=== ZepraBrowser OpenGL Rendering Demo ===" << std::endl;
    std::cout << "Press ESC or close the window to exit." << std::endl;
    std::cout << std::endl;
    
    GLDemo demo;
    if (demo.init()) {
        demo.run();
        std::cout << "Demo finished successfully!" << std::endl;
    } else {
        std::cerr << "Failed to start demo" << std::endl;
        return 1;
    }
    
    SDL_Quit();
    return 0;
}
