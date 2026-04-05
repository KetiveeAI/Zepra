/**
 * @file nx_pdf_font.h
 * @brief Base Font models mapped dynamically
 */

#pragma once

#include "../parser/nx_pdf_objects.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace nxrender {
namespace pdf {
namespace fonts {

enum class FontType {
    Unknown,
    Type1,
    TrueType,
    Type0 // CIDFont Wrapper
};

class PdfFont {
public:
    explicit PdfFont(parser::PdfDictionary* fontDict);
    virtual ~PdfFont() = default;

    FontType GetType() const { return type_; }
    const std::string& GetBaseFont() const { return baseFont_; }
    
    bool HasEmbeddedFormat() const { return hasEmbedded_; }
    
    // Abstract width mapping logic bounded explicitly globally 1000 base
    double GetWidth(int charCode) const;

private:
    void ParseBaseDescriptors(parser::PdfDictionary* fontDict);
    
    FontType type_ = FontType::Unknown;
    std::string baseFont_;
    int firstChar_ = 0;
    int lastChar_ = 255;
    std::map<int, double> widths_;
    bool hasEmbedded_ = false;
};

} // namespace fonts
} // namespace pdf
} // namespace nxrender
