/**
 * @file svg_loader.hpp
 * @brief SVG icon loader using nanosvg
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
 * @brief SVG icon loader that parses SVG files and creates textures
 */
class SVGLoader {
public:
    static SVGLoader& instance();
    
    // Load SVG and return texture ID (cached)
    SVGTexture loadSVG(const std::string& path, int targetWidth, int targetHeight);
    
    // Clear cache
    void clearCache();
    
private:
    SVGLoader() = default;
    ~SVGLoader();
    
    std::unordered_map<std::string, SVGTexture> cache_;
};

} // namespace Zepra::WebCore
