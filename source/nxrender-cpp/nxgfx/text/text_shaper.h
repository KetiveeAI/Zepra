// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "nxgfx/math/vector.h"
#include <hb.h>

namespace NXRender {
namespace Text {

struct ShapedGlyph {
    uint32_t glyphIndex;
    float xAdvance;
    float yAdvance;
    float xOffset;
    float yOffset;
    uint32_t clusterIndex;
};

struct TextRun {
    std::string text;
    std::string language;
    bool isRTL;
    std::vector<ShapedGlyph> glyphs;
};

class TextShaper {
public:
    TextShaper();
    ~TextShaper();

    bool init();

    // Shape a single run with HarfBuzz
    TextRun shape(const std::string& text, const std::string& fontName, bool isBold, bool isItalic, float fontSize, const std::string& lang, bool isRTL);

    // Split text into script-homogeneous runs and shape each
    std::vector<TextRun> itemize(const std::string& text, const std::string& fontName,
                                  bool isBold, bool isItalic, float fontSize,
                                  const std::string& lang);

    // Measurement
    static float measureWidth(const TextRun& run);
    static size_t hitTestPosition(const TextRun& run, float x);
    static float caretOffset(const TextRun& run, size_t clusterIndex);

private:
    hb_buffer_t* hbBuffer_ = nullptr;
};

} // namespace Text
} // namespace NXRender
