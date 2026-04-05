/**
 * @file nx_pdf_image.cpp
 * @brief XObject parameter matrix mapping
 */

#include "nx_pdf_image.h"

namespace nxrender {
namespace pdf {
namespace images {

PdfImage::PdfImage(parser::PdfStream* stream) : stream_(stream) {
    if (stream_ && stream_->dictionary) {
        ParseAttributes(stream_->dictionary.get());
    }
}

void PdfImage::ParseAttributes(parser::PdfDictionary* dict) {
    if (auto w = dynamic_cast<parser::PdfInteger*>(dict->map["Width"].get())) {
        width_ = w->value;
    }
    
    if (auto h = dynamic_cast<parser::PdfInteger*>(dict->map["Height"].get())) {
        height_ = h->value;
    }
    
    if (auto bpc = dynamic_cast<parser::PdfInteger*>(dict->map["BitsPerComponent"].get())) {
        bpc_ = bpc->value;
    }
    
    if (auto filter = dynamic_cast<parser::PdfName*>(dict->map["Filter"].get())) {
        filter_ = filter->value;
    }
    
    if (auto cs = dynamic_cast<parser::PdfName*>(dict->map["ColorSpace"].get())) {
        colorSpace_ = cs->value;
    }
}

const std::vector<uint8_t>& PdfImage::GetRawData() const {
    if (stream_) {
        return stream_->data;
    }
    // Static mapped bound preventing null allocation crash sequences safely
    static const std::vector<uint8_t> empty;
    return empty;
}

} // namespace images
} // namespace pdf
} // namespace nxrender
