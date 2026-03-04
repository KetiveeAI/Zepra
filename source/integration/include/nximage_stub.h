#pragma once

#include <vector>
#include <cstdint>
#include <string>

// Simple Image Structure
struct NxImageResult {
    bool success;
    int width;
    int height;
    std::vector<uint8_t> data; // RGBA
};

// Stub decoder for when stb_image is not available
// Generates a placeholder pattern (checkerboard)
inline NxImageResult nx_decode_image_stub(const std::vector<uint8_t>& rawData) {
    NxImageResult img;
    img.width = 100;
    img.height = 100;
    img.success = false;
    
    if (rawData.empty()) {
        return img;
    }
    
    // In a real implementation, we would call stbi_load_from_memory here
    // For now, we generate a 100x100 checkerboard pattern
    // Color determined by data size to "vary" slightly based on input
    
    img.data.resize(img.width * img.height * 4);
    
    uint8_t baseR = rawData.size() % 255;
    uint8_t baseG = (rawData.size() * 2) % 255;
    uint8_t baseB = (rawData.size() * 3) % 255;
    
    for (int y = 0; y < img.height; y++) {
        for (int x = 0; x < img.width; x++) {
            int index = (y * img.width + x) * 4;
            bool dark = ((x / 10) + (y / 10)) % 2 == 0;
            
            if (dark) {
                img.data[index + 0] = baseR;
                img.data[index + 1] = baseG;
                img.data[index + 2] = baseB;
                img.data[index + 3] = 255; // Alpha
            } else {
                img.data[index + 0] = 200;
                img.data[index + 1] = 200;
                img.data[index + 2] = 200;
                img.data[index + 3] = 255; // Alpha
            }
        }
    }
    
    img.success = true;
    return img;
}
