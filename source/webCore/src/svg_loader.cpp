/**
 * @file svg_loader.cpp
 * @brief SVG icon loader implementation using nanosvg
 */

#define NANOSVG_IMPLEMENTATION
#include "nanosvg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/nanosvgrast.h"

#include "webcore/svg_loader.hpp"
#include <iostream>
#include <cstring>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace Zepra::WebCore {

SVGLoader& SVGLoader::instance() {
    static SVGLoader instance;
    return instance;
}

SVGLoader::~SVGLoader() {
    clearCache();
}

SVGTexture SVGLoader::loadSVG(const std::string& path, int targetWidth, int targetHeight) {
    // Check cache
    std::string key = path + "_" + std::to_string(targetWidth) + "x" + std::to_string(targetHeight);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return it->second;
    }
    
    // Parse SVG
    NSVGimage* image = nsvgParseFromFile(path.c_str(), "px", 96.0f);
    if (!image) {
        std::cerr << "SVGLoader: Failed to parse SVG: " << path << std::endl;
        return {};
    }
    
    // Calculate scale
    float scaleX = (float)targetWidth / image->width;
    float scaleY = (float)targetHeight / image->height;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;
    
    int width = (int)(image->width * scale);
    int height = (int)(image->height * scale);
    
    if (width <= 0 || height <= 0) {
        width = targetWidth;
        height = targetHeight;
        scale = 1.0f;
    }
    
    // Rasterize
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        std::cerr << "SVGLoader: Failed to create rasterizer" << std::endl;
        nsvgDelete(image);
        return {};
    }
    
    unsigned char* pixels = new unsigned char[width * height * 4];
    memset(pixels, 0, width * height * 4);
    
    nsvgRasterize(rast, image, 0, 0, scale, pixels, width, height, width * 4);
    
    // Create OpenGL texture
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    
    // Cleanup
    delete[] pixels;
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);
    
    // Cache and return
    SVGTexture tex = {textureId, width, height};
    cache_[key] = tex;
    
    std::cout << "SVGLoader: Loaded " << path << " (" << width << "x" << height << ")" << std::endl;
    
    return tex;
}

void SVGLoader::clearCache() {
    for (auto& pair : cache_) {
        if (pair.second.textureId) {
            glDeleteTextures(1, &pair.second.textureId);
        }
    }
    cache_.clear();
}

} // namespace Zepra::WebCore
