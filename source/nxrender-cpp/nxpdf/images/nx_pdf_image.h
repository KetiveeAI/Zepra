/**
 * @file nx_pdf_image.h
 * @brief XObject Image data abstraction
 */

#pragma once

#include "../parser/nx_pdf_objects.h"
#include <string>
#include <vector>

namespace nxrender {
namespace pdf {
namespace images {

class PdfImage {
public:
    explicit PdfImage(parser::PdfStream* stream);
    ~PdfImage() = default;

    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    int GetBitsPerComponent() const { return bpc_; }
    
    // Defines exact compression states avoiding speculative buffer logic natively
    const std::string& GetFilter() const { return filter_; }
    const std::string& GetColorSpace() const { return colorSpace_; }
    
    // Bounds strictly avoiding duplicates returning original memory span
    const std::vector<uint8_t>& GetRawData() const;

private:
    void ParseAttributes(parser::PdfDictionary* dict);

    parser::PdfStream* stream_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    int bpc_ = 8;
    std::string colorSpace_ = "DeviceRGB"; // standard fallback
    std::string filter_;
};

} // namespace images
} // namespace pdf
} // namespace nxrender
