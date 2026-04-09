// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "nx_pdf_image.h"
#include <algorithm>
#include <cstring>
#include <cstdint>

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

    // Check for Mask
    if (dict->map.find("SMask") != dict->map.end()) {
        hasSoftMask_ = true;
    }

    // ImageMask
    if (auto mask = dynamic_cast<parser::PdfBool*>(dict->map["ImageMask"].get())) {
        isImageMask_ = mask->value;
        if (isImageMask_) bpc_ = 1;
    }

    // Decode array
    if (auto decode = dynamic_cast<parser::PdfArray*>(dict->map["Decode"].get())) {
        for (const auto& item : decode->items) {
            if (auto r = dynamic_cast<parser::PdfReal*>(item.get())) {
                decodeArray_.push_back(r->value);
            } else if (auto i = dynamic_cast<parser::PdfInteger*>(item.get())) {
                decodeArray_.push_back(static_cast<double>(i->value));
            }
        }
    }

    // Intent
    if (auto intent = dynamic_cast<parser::PdfName*>(dict->map["Intent"].get())) {
        intent_ = intent->value;
    }

    // Interpolate
    if (auto interp = dynamic_cast<parser::PdfBool*>(dict->map["Interpolate"].get())) {
        interpolate_ = interp->value;
    }
}

const std::vector<uint8_t>& PdfImage::GetRawData() const {
    if (stream_) {
        return stream_->data;
    }
    static const std::vector<uint8_t> empty;
    return empty;
}

// ==================================================================
// Component count
// ==================================================================

int PdfImage::GetComponentCount() const {
    if (colorSpace_ == "DeviceRGB") return 3;
    if (colorSpace_ == "DeviceCMYK") return 4;
    if (colorSpace_ == "DeviceGray") return 1;
    if (isImageMask_) return 1;
    return 3; // Default assumption
}

// ==================================================================
// Pixel data stride
// ==================================================================

size_t PdfImage::GetStrideBytes() const {
    int components = GetComponentCount();
    size_t bitsPerPixel = components * bpc_;
    size_t bitsPerRow = width_ * bitsPerPixel;
    return (bitsPerRow + 7) / 8; // Round up to byte boundary
}

// ==================================================================
// Raw to RGBA conversion
// ==================================================================

std::vector<uint8_t> PdfImage::DecodeToRGBA() const {
    int components = GetComponentCount();
    size_t pixelCount = width_ * height_;
    std::vector<uint8_t> rgba(pixelCount * 4, 255);

    const auto& raw = GetRawData();
    if (raw.empty() || width_ <= 0 || height_ <= 0) return rgba;

    size_t stride = GetStrideBytes();

    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            size_t pixelOffset = y * stride;
            size_t outIdx = (y * width_ + x) * 4;

            if (bpc_ == 8) {
                size_t srcIdx = pixelOffset + x * components;
                if (srcIdx + components > raw.size()) continue;

                if (components == 3) {
                    // RGB
                    rgba[outIdx + 0] = raw[srcIdx + 0];
                    rgba[outIdx + 1] = raw[srcIdx + 1];
                    rgba[outIdx + 2] = raw[srcIdx + 2];
                    rgba[outIdx + 3] = 255;
                } else if (components == 1) {
                    // Grayscale
                    rgba[outIdx + 0] = raw[srcIdx];
                    rgba[outIdx + 1] = raw[srcIdx];
                    rgba[outIdx + 2] = raw[srcIdx];
                    rgba[outIdx + 3] = 255;
                } else if (components == 4) {
                    // CMYK -> RGB
                    double c = raw[srcIdx + 0] / 255.0;
                    double m = raw[srcIdx + 1] / 255.0;
                    double yk = raw[srcIdx + 2] / 255.0;
                    double k = raw[srcIdx + 3] / 255.0;
                    rgba[outIdx + 0] = static_cast<uint8_t>((1.0 - c) * (1.0 - k) * 255);
                    rgba[outIdx + 1] = static_cast<uint8_t>((1.0 - m) * (1.0 - k) * 255);
                    rgba[outIdx + 2] = static_cast<uint8_t>((1.0 - yk) * (1.0 - k) * 255);
                    rgba[outIdx + 3] = 255;
                }
            } else if (bpc_ == 1 && components == 1) {
                // 1-bit monochrome
                size_t byteIdx = pixelOffset + x / 8;
                if (byteIdx >= raw.size()) continue;
                int bitIdx = 7 - (x % 8);
                uint8_t val = (raw[byteIdx] >> bitIdx) & 1;
                uint8_t gray = val ? 255 : 0;
                rgba[outIdx + 0] = gray;
                rgba[outIdx + 1] = gray;
                rgba[outIdx + 2] = gray;
                rgba[outIdx + 3] = 255;
            }
        }
    }

    return rgba;
}

size_t PdfImage::GetRawDataSize() const {
    const auto& data = GetRawData();
    return data.size();
}

size_t PdfImage::GetDecodedSize() const {
    return static_cast<size_t>(width_) * height_ * 4;
}

} // namespace images
} // namespace pdf
} // namespace nxrender
