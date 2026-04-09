// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "nxgfx/text/text_shaper.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <ft2build.h>
#include FT_FREETYPE_H

struct hb_font_t;

namespace NXRender {
namespace Text {

class FontFallbackManager {
public:
    static FontFallbackManager& instance();

    bool initialize();
    void shutdown();

    bool registerFont(const std::string& family, const std::string& path, bool isBold, bool isItalic);

    FT_Face getFace(const std::string& family, bool isBold, bool isItalic);
    hb_font_t* getHbFont(const std::string& family, bool isBold, bool isItalic, float size);

    // Codepoint coverage
    bool hasGlyph(const std::string& family, bool isBold, bool isItalic, uint32_t codepoint);
    std::string findFontForCodepoint(uint32_t codepoint);

    // Statistics
    size_t registeredFamilyCount() const;
    size_t cachedFaceCount() const;

private:
    FontFallbackManager();
    ~FontFallbackManager();

    void discoverSystemFonts();
    void scanFontDirectory(const std::string& dirPath);
    FT_Face tryFamilyAlias(const std::string& family, bool isBold, bool isItalic);

    struct FontRegistration {
        std::string family;
        std::string path;
        bool isBold;
        bool isItalic;
    };

    FT_Library ftLibrary_ = nullptr;
    std::unordered_map<std::string, std::vector<FontRegistration>> registry_;
    std::unordered_map<std::string, FT_Face> faceCache_;
    std::unordered_map<std::string, hb_font_t*> hbFontCache_;

    std::string buildKey(const std::string& family, bool isBold, bool isItalic, float size) const;
};

} // namespace Text
} // namespace NXRender
