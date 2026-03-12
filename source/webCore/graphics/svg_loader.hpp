/**
 * @file svg_loader.hpp
 * @brief SVG icon loader using custom NxSVG (No third-party dependencies)
 */

#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

namespace Zepra::WebCore {

struct SVGTexture {
    uint32_t textureId = 0;
    int width = 0;
    int height = 0;
};

/**
 * @brief SVG icon loader that parses SVG files using custom NxSVG
 * 
 * Uses the custom nxsvg.h parser - no third-party dependencies.
 */
class SVGLoader {
public:
    static SVGLoader& instance();
    
    /// Load SVG and return texture info (cached)
    SVGTexture loadSVG(const std::string& path, int targetWidth, int targetHeight);
    
    /// Draw SVG directly using OpenGL (preferred method)
    /// @param path Path to SVG file
    /// @param x X position to draw at
    /// @param y Y position to draw at
    /// @param size Size to render the icon at
    /// @param r Red color component (0-255)
    /// @param g Green color component (0-255)
    /// @param b Blue color component (0-255)
    void drawSVG(const std::string& path, float x, float y, float size,
                 uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);
    
    /// Clear texture cache
    void clearCache();
    
private:
    SVGLoader() = default;
    ~SVGLoader();
    
    std::unordered_map<std::string, SVGTexture> cache_;
};

} // namespace Zepra::WebCore
