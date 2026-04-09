// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "text_shaper.h"
#include "font_fallback.h"
#include <cstring>
#include <algorithm>

namespace NXRender {
namespace Text {

TextShaper::TextShaper() {
    hbBuffer_ = hb_buffer_create();
}

TextShaper::~TextShaper() {
    if (hbBuffer_) {
        hb_buffer_destroy(hbBuffer_);
        hbBuffer_ = nullptr;
    }
}

bool TextShaper::init() {
    return FontFallbackManager::instance().initialize();
}

// ==================================================================
// Script and direction detection
// ==================================================================

static hb_script_t detectScript(const std::string& text) {
    // Heuristic: sample the first non-ASCII codepoint
    for (size_t i = 0; i < text.size(); ) {
        uint8_t c = static_cast<uint8_t>(text[i]);
        uint32_t codepoint = 0;

        if (c < 0x80) {
            codepoint = c;
            i++;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < text.size()) {
            codepoint = ((c & 0x1F) << 6) | (text[i+1] & 0x3F);
            i += 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < text.size()) {
            codepoint = ((c & 0x0F) << 12) | ((text[i+1] & 0x3F) << 6)
                        | (text[i+2] & 0x3F);
            i += 3;
        } else if ((c & 0xF8) == 0xF0 && i + 3 < text.size()) {
            codepoint = ((c & 0x07) << 18) | ((text[i+1] & 0x3F) << 12)
                        | ((text[i+2] & 0x3F) << 6) | (text[i+3] & 0x3F);
            i += 4;
        } else {
            i++;
            continue;
        }

        // Arabic range
        if (codepoint >= 0x0600 && codepoint <= 0x06FF) return HB_SCRIPT_ARABIC;
        // Hebrew range
        if (codepoint >= 0x0590 && codepoint <= 0x05FF) return HB_SCRIPT_HEBREW;
        // Devanagari range
        if (codepoint >= 0x0900 && codepoint <= 0x097F) return HB_SCRIPT_DEVANAGARI;
        // Bengali
        if (codepoint >= 0x0980 && codepoint <= 0x09FF) return HB_SCRIPT_BENGALI;
        // Tamil
        if (codepoint >= 0x0B80 && codepoint <= 0x0BFF) return HB_SCRIPT_TAMIL;
        // Thai
        if (codepoint >= 0x0E00 && codepoint <= 0x0E7F) return HB_SCRIPT_THAI;
        // CJK Unified Ideographs
        if (codepoint >= 0x4E00 && codepoint <= 0x9FFF) return HB_SCRIPT_HAN;
        // Hangul
        if (codepoint >= 0xAC00 && codepoint <= 0xD7AF) return HB_SCRIPT_HANGUL;
        // Hiragana
        if (codepoint >= 0x3040 && codepoint <= 0x309F) return HB_SCRIPT_HIRAGANA;
        // Katakana
        if (codepoint >= 0x30A0 && codepoint <= 0x30FF) return HB_SCRIPT_KATAKANA;
    }

    return HB_SCRIPT_LATIN;
}

static bool isRTLScript(hb_script_t script) {
    return script == HB_SCRIPT_ARABIC ||
           script == HB_SCRIPT_HEBREW ||
           script == HB_SCRIPT_SYRIAC ||
           script == HB_SCRIPT_THAANA;
}

// ==================================================================
// Core shaping
// ==================================================================

TextRun TextShaper::shape(const std::string& text, const std::string& fontName,
                          bool isBold, bool isItalic, float fontSize,
                          const std::string& lang, bool isRTL) {
    TextRun run;
    run.text = text;
    run.language = lang;
    run.isRTL = isRTL;

    if (text.empty()) return run;

    hb_font_t* useFont = FontFallbackManager::instance().getHbFont(
        fontName, isBold, isItalic, fontSize);
    if (!useFont) {
        useFont = FontFallbackManager::instance().getHbFont(
            "Arial", false, false, fontSize);
    }
    if (!useFont) return run;

    hb_buffer_clear_contents(hbBuffer_);
    hb_buffer_add_utf8(hbBuffer_, text.c_str(), -1, 0, -1);

    // Auto-detect script if not specified
    hb_script_t script = detectScript(text);
    hb_buffer_set_script(hbBuffer_, script);

    // Direction
    if (isRTL || isRTLScript(script)) {
        hb_buffer_set_direction(hbBuffer_, HB_DIRECTION_RTL);
        run.isRTL = true;
    } else {
        hb_buffer_set_direction(hbBuffer_, HB_DIRECTION_LTR);
    }

    // Language
    if (!lang.empty()) {
        hb_buffer_set_language(hbBuffer_, hb_language_from_string(lang.c_str(), -1));
    }

    // Guess segment properties for anything unset
    hb_buffer_guess_segment_properties(hbBuffer_);

    // Optional OpenType features
    hb_feature_t features[4];
    unsigned int featureCount = 0;

    // Enable kerning
    features[featureCount].tag = HB_TAG('k','e','r','n');
    features[featureCount].value = 1;
    features[featureCount].start = HB_FEATURE_GLOBAL_START;
    features[featureCount].end = HB_FEATURE_GLOBAL_END;
    featureCount++;

    // Enable standard ligatures
    features[featureCount].tag = HB_TAG('l','i','g','a');
    features[featureCount].value = 1;
    features[featureCount].start = HB_FEATURE_GLOBAL_START;
    features[featureCount].end = HB_FEATURE_GLOBAL_END;
    featureCount++;

    // Execute shaping
    hb_shape(useFont, hbBuffer_, features, featureCount);

    unsigned int glyph_count;
    hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(hbBuffer_, &glyph_count);
    hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(hbBuffer_, &glyph_count);

    run.glyphs.reserve(glyph_count);

    const float HB_SCALE = 1.0f / 64.0f;

    for (unsigned int i = 0; i < glyph_count; ++i) {
        ShapedGlyph g;
        g.glyphIndex = glyph_info[i].codepoint;
        g.clusterIndex = glyph_info[i].cluster;
        g.xAdvance = glyph_pos[i].x_advance * HB_SCALE;
        g.yAdvance = glyph_pos[i].y_advance * HB_SCALE;
        g.xOffset = glyph_pos[i].x_offset * HB_SCALE;
        g.yOffset = glyph_pos[i].y_offset * HB_SCALE;
        run.glyphs.push_back(g);
    }

    return run;
}

// ==================================================================
// Itemization — split text into runs by script
// ==================================================================

std::vector<TextRun> TextShaper::itemize(const std::string& text,
                                          const std::string& fontName,
                                          bool isBold, bool isItalic,
                                          float fontSize,
                                          const std::string& lang) {
    std::vector<TextRun> runs;
    if (text.empty()) return runs;

    // Simple itemizer: split on script boundaries
    size_t start = 0;
    hb_script_t currentScript = HB_SCRIPT_COMMON;
    bool firstChar = true;

    size_t i = 0;
    while (i < text.size()) {
        uint8_t c = static_cast<uint8_t>(text[i]);
        uint32_t codepoint = 0;
        size_t charLen = 1;

        if (c < 0x80) {
            codepoint = c;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < text.size()) {
            codepoint = ((c & 0x1F) << 6) | (text[i+1] & 0x3F);
            charLen = 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < text.size()) {
            codepoint = ((c & 0x0F) << 12) | ((text[i+1] & 0x3F) << 6)
                        | (text[i+2] & 0x3F);
            charLen = 3;
        } else if ((c & 0xF8) == 0xF0 && i + 3 < text.size()) {
            codepoint = ((c & 0x07) << 18) | ((text[i+1] & 0x3F) << 12)
                        | ((text[i+2] & 0x3F) << 6) | (text[i+3] & 0x3F);
            charLen = 4;
        }

        hb_script_t charScript = hb_unicode_script(
            hb_unicode_funcs_get_default(), codepoint);

        // COMMON and INHERITED inherit from surrounding
        if (charScript != HB_SCRIPT_COMMON && charScript != HB_SCRIPT_INHERITED) {
            if (firstChar) {
                currentScript = charScript;
                firstChar = false;
            } else if (charScript != currentScript) {
                // Script boundary — emit run
                std::string segment = text.substr(start, i - start);
                bool rtl = isRTLScript(currentScript);
                runs.push_back(shape(segment, fontName, isBold, isItalic,
                                     fontSize, lang, rtl));
                start = i;
                currentScript = charScript;
            }
        }

        i += charLen;
    }

    // Emit final run
    if (start < text.size()) {
        std::string segment = text.substr(start);
        bool rtl = isRTLScript(currentScript);
        runs.push_back(shape(segment, fontName, isBold, isItalic,
                             fontSize, lang, rtl));
    }

    return runs;
}

// ==================================================================
// Measurement
// ==================================================================

float TextShaper::measureWidth(const TextRun& run) {
    float width = 0;
    for (const auto& g : run.glyphs) {
        width += g.xAdvance;
    }
    return width;
}

size_t TextShaper::hitTestPosition(const TextRun& run, float x) {
    if (run.glyphs.empty()) return 0;

    float cursor = 0;
    for (size_t i = 0; i < run.glyphs.size(); i++) {
        float mid = cursor + run.glyphs[i].xAdvance * 0.5f;
        if (x < mid) return run.glyphs[i].clusterIndex;
        cursor += run.glyphs[i].xAdvance;
    }
    return run.glyphs.back().clusterIndex + 1;
}

float TextShaper::caretOffset(const TextRun& run, size_t clusterIndex) {
    float offset = 0;
    for (const auto& g : run.glyphs) {
        if (g.clusterIndex >= clusterIndex) break;
        offset += g.xAdvance;
    }
    return offset;
}

} // namespace Text
} // namespace NXRender
