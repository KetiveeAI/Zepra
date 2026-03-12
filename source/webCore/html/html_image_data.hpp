/**
 * @file html_image_data.hpp
 * @brief Image data and bitmap interfaces
 */

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief Image data for canvas operations
 */
class ImageData {
public:
    ImageData(uint32_t width, uint32_t height);
    ImageData(uint32_t width, uint32_t height, const std::vector<uint8_t>& data);
    ~ImageData() = default;
    
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    
    const std::vector<uint8_t>& data() const { return data_; }
    std::vector<uint8_t>& data() { return data_; }
    
    // Pixel access (RGBA)
    uint32_t getPixel(uint32_t x, uint32_t y) const;
    void setPixel(uint32_t x, uint32_t y, uint32_t rgba);
    
    void getPixelComponents(uint32_t x, uint32_t y, 
                            uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const;
    void setPixelComponents(uint32_t x, uint32_t y,
                            uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    
private:
    uint32_t width_;
    uint32_t height_;
    std::vector<uint8_t> data_;  // RGBA format
};

/**
 * @brief Image bitmap for efficient image handling
 */
class ImageBitmap {
public:
    ImageBitmap(uint32_t width, uint32_t height);
    ImageBitmap(const ImageData& imageData);
    ~ImageBitmap() = default;
    
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    
    void close();
    bool isClosed() const { return closed_; }
    
    const std::vector<uint8_t>& data() const { return data_; }
    
private:
    uint32_t width_;
    uint32_t height_;
    std::vector<uint8_t> data_;
    bool closed_ = false;
};

/**
 * @brief Canvas rendering context 2D (interface only)
 */
class CanvasRenderingContext2DInterface {
public:
    virtual ~CanvasRenderingContext2DInterface() = default;
    
    // State
    virtual void save() = 0;
    virtual void restore() = 0;
    
    // Transformations
    virtual void scale(double x, double y) = 0;
    virtual void rotate(double angle) = 0;
    virtual void translate(double x, double y) = 0;
    virtual void transform(double a, double b, double c, double d, double e, double f) = 0;
    virtual void setTransform(double a, double b, double c, double d, double e, double f) = 0;
    virtual void resetTransform() = 0;
    
    // Compositing
    virtual double globalAlpha() const = 0;
    virtual void setGlobalAlpha(double alpha) = 0;
    virtual std::string globalCompositeOperation() const = 0;
    virtual void setGlobalCompositeOperation(const std::string& op) = 0;
    
    // Drawing rectangles
    virtual void clearRect(double x, double y, double w, double h) = 0;
    virtual void fillRect(double x, double y, double w, double h) = 0;
    virtual void strokeRect(double x, double y, double w, double h) = 0;
    
    // Text
    virtual void fillText(const std::string& text, double x, double y) = 0;
    virtual void strokeText(const std::string& text, double x, double y) = 0;
    
    // Paths
    virtual void beginPath() = 0;
    virtual void closePath() = 0;
    virtual void moveTo(double x, double y) = 0;
    virtual void lineTo(double x, double y) = 0;
    virtual void bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y) = 0;
    virtual void quadraticCurveTo(double cpx, double cpy, double x, double y) = 0;
    virtual void arc(double x, double y, double radius, double startAngle, double endAngle, bool anticlockwise = false) = 0;
    virtual void arcTo(double x1, double y1, double x2, double y2, double radius) = 0;
    virtual void ellipse(double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle, bool anticlockwise = false) = 0;
    virtual void rect(double x, double y, double w, double h) = 0;
    
    // Drawing
    virtual void fill() = 0;
    virtual void stroke() = 0;
    virtual void clip() = 0;
    
    // Image data
    virtual std::unique_ptr<ImageData> getImageData(int x, int y, int w, int h) = 0;
    virtual void putImageData(const ImageData& data, int x, int y) = 0;
    virtual std::unique_ptr<ImageData> createImageData(int w, int h) = 0;
};

} // namespace Zepra::WebCore
