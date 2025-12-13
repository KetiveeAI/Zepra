#include "webcore/simple_font.hpp"
#include <SDL2/SDL_ttf.h>
#include <iostream>

namespace Zepra::WebCore {

static TTF_Font* measurementFont = nullptr;

static void ensureMeasurementFont(int size) {
    if (!measurementFont) {
        if (!TTF_WasInit()) {
             // Should be init by backend (GLRenderBackend), but safety check
             // Note: Initializes SDL_ttf if not already initialized
             if (TTF_Init() == -1) {
                 std::cerr << "SimpleFont: SDL_ttf init failed: " << TTF_GetError() << std::endl;
             }
        }
        // Try common system fonts
        measurementFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", size);
        if (!measurementFont) {
             measurementFont = TTF_OpenFont("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", size);
        }
        if (!measurementFont) {
             std::cerr << "SimpleFont: Failed to load measurement font!" << std::endl;
        }
    }
}

void SimpleFont::drawText(PaintContext& ctx, const std::string& text, float x, float y, const Color& color, float size) {
    // Delegate to backend text rendering which uses textures
    ctx.drawText(text, x, y, color, size);
}

float SimpleFont::getTextWidth(const std::string& text, float size) {
    ensureMeasurementFont((int)size);
    if (!measurementFont) return text.length() * size * 0.6f; // Fallback estimate
    
    int w, h;
    TTF_SizeUTF8(measurementFont, text.c_str(), &w, &h);
    
    return (float)w;
}

float SimpleFont::getCharWidth(float size) {
    return getTextWidth("M", size);
}

float SimpleFont::getLineHeight(float size) {
    ensureMeasurementFont((int)size);
    if (!measurementFont) return size * 1.2f;
    return (float)TTF_FontLineSkip(measurementFont);
}

} // namespace Zepra::WebCore
