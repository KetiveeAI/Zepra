/**
 * @file nx_pdf_font.cpp
 * @brief Base Font definitions internally mapped
 */

#include "nx_pdf_font.h"

namespace nxrender {
namespace pdf {
namespace fonts {

PdfFont::PdfFont(parser::PdfDictionary* dict) {
    if (dict) {
        ParseBaseDescriptors(dict);
    }
}

void PdfFont::ParseBaseDescriptors(parser::PdfDictionary* dict) {
    if (auto subtype = dynamic_cast<parser::PdfName*>(dict->map["Subtype"].get())) {
        if (subtype->value == "Type1") type_ = FontType::Type1;
        else if (subtype->value == "TrueType") type_ = FontType::TrueType;
        else if (subtype->value == "Type0") type_ = FontType::Type0;
    }
    
    if (auto baseFont = dynamic_cast<parser::PdfName*>(dict->map["BaseFont"].get())) {
        baseFont_ = baseFont->value;
    }
    
    if (auto firstC = dynamic_cast<parser::PdfInteger*>(dict->map["FirstChar"].get())) {
        firstChar_ = firstC->value;
    }
    
    if (auto lastC = dynamic_cast<parser::PdfInteger*>(dict->map["LastChar"].get())) {
        lastChar_ = lastC->value;
    }
    
    // Layout Widths explicitly bounding 
    if (auto widthsArr = dynamic_cast<parser::PdfArray*>(dict->map["Widths"].get())) {
        int currentChar = firstChar_;
        for (const auto& wObj : widthsArr->items) {
            if (auto wReal = dynamic_cast<parser::PdfReal*>(wObj.get())) {
                widths_[currentChar] = wReal->value;
            } else if (auto wInt = dynamic_cast<parser::PdfInteger*>(wObj.get())) {
                widths_[currentChar] = static_cast<double>(wInt->value);
            }
            currentChar++;
        }
    }
    
    if (auto fdRef = dynamic_cast<parser::PdfDictionary*>(dict->map["FontDescriptor"].get())) {
        if (fdRef->map.count("FontFile2") || fdRef->map.count("FontFile3")) {
            hasEmbedded_ = true;
        }
    }
}

double PdfFont::GetWidth(int charCode) const {
    auto it = widths_.find(charCode);
    if (it != widths_.end()) return it->second;
    return 1000.0; // Base generic width parameter
}

} // namespace fonts
} // namespace pdf
} // namespace nxrender
