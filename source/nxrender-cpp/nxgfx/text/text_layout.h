// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file text_layout.h
 * @brief Multi-line text layout engine with word-wrap, line-breaking, and bidi support.
 */

#pragma once

#include "../primitives.h"
#include "../color.h"
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace NXRender {

class GpuContext;

enum class TextAlign : uint8_t {
    Left,
    Center,
    Right,
    Justify
};

enum class TextOverflow : uint8_t {
    Clip,
    Ellipsis,
    Fade
};

enum class LineBreakMode : uint8_t {
    Word,      // Break at word boundaries
    Char,      // Break at any character
    Anywhere   // Break anywhere (for CJK)
};

enum class TextDirection : uint8_t {
    LTR,
    RTL,
    Auto
};

/**
 * @brief A run of text with uniform styling.
 */
struct TextRun {
    std::string text;
    float fontSize = 14.0f;
    Color color = Color(0x212121);
    std::string fontFamily;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;
    float letterSpacing = 0;
    float lineHeightMultiplier = 1.4f;

    TextRun() = default;
    TextRun(const std::string& t) : text(t) {}
    TextRun(const std::string& t, float size, const Color& c)
        : text(t), fontSize(size), color(c) {}
};

/**
 * @brief A positioned glyph in the output.
 */
struct PositionedGlyph {
    uint32_t glyphId;
    float x, y;           // Top-left position
    float advance;         // Horizontal advance
    float width, height;   // Glyph bounding box
    size_t runIndex;       // Which TextRun this glyph belongs to
    size_t charIndex;      // Character index within the run
};

/**
 * @brief A visual line of text after layout.
 */
struct TextLine {
    float x, y;            // Line origin
    float width;           // Actual content width
    float height;          // Line height
    float baseline;        // Distance from y to baseline
    float ascent;
    float descent;
    size_t startGlyph;     // First glyph index in this line
    size_t glyphCount;     // Number of glyphs in this line
    bool isLastLine;       // Last line in paragraph
    bool isWrapped;        // Was wrapped (not explicit newline)
};

/**
 * @brief Result of a text layout computation.
 */
struct TextLayoutResult {
    std::vector<PositionedGlyph> glyphs;
    std::vector<TextLine> lines;
    float totalWidth = 0;   // Maximum line width
    float totalHeight = 0;  // Total height of all lines
    size_t characterCount = 0;

    bool empty() const { return lines.empty(); }
};

/**
 * @brief Hit-test result for text position queries.
 */
struct TextHitResult {
    size_t lineIndex = 0;
    size_t glyphIndex = 0;
    size_t charIndex = 0;
    size_t runIndex = 0;
    float insertX = 0;      // X position of insertion point
    bool isLeading = true;   // Hit on leading edge of glyph
    bool isValid = false;
};

/**
 * @brief Multi-line text layout engine.
 *
 * Usage:
 *   TextLayout layout;
 *   layout.setMaxWidth(400);
 *   layout.setAlignment(TextAlign::Left);
 *   layout.addRun(TextRun("Hello ", 16, Color(0)));
 *   layout.addRun(TextRun("world!", 16, Color(0xFF0000)));
 *   auto result = layout.compute();
 */
class TextLayout {
public:
    TextLayout();
    ~TextLayout();

    // Configuration
    void setMaxWidth(float width) { maxWidth_ = width; }
    void setMaxHeight(float height) { maxHeight_ = height; }
    void setMaxLines(int lines) { maxLines_ = lines; }
    void setAlignment(TextAlign align) { align_ = align; }
    void setOverflow(TextOverflow overflow) { overflow_ = overflow; }
    void setLineBreakMode(LineBreakMode mode) { breakMode_ = mode; }
    void setDirection(TextDirection dir) { direction_ = dir; }
    void setLineSpacing(float spacing) { lineSpacing_ = spacing; }
    void setIndent(float indent) { indent_ = indent; }

    // Content
    void addRun(const TextRun& run);
    void setText(const std::string& text, float fontSize = 14.0f,
                 const Color& color = Color(0x212121));
    void clearRuns();

    // Compute layout
    TextLayoutResult compute();

    // Hit testing
    TextHitResult hitTest(float x, float y, const TextLayoutResult& layout) const;

    // Selection
    Rect caretRect(size_t charIndex, const TextLayoutResult& layout) const;
    std::vector<Rect> selectionRects(size_t startChar, size_t endChar,
                                      const TextLayoutResult& layout) const;

    // Rendering
    void render(GpuContext* ctx, float originX, float originY,
                const TextLayoutResult& layout) const;

    // Metrics
    float measureWidth(const std::string& text, float fontSize) const;
    float measureHeight(const std::string& text, float fontSize, float maxWidth) const;

private:
    struct WordSegment {
        size_t runIndex;
        size_t startChar;
        size_t endChar;
        float width;
        bool isWhitespace;
        bool isNewline;
    };

    std::vector<WordSegment> segmentRuns() const;
    float measureSegment(const WordSegment& seg) const;
    void applyAlignment(TextLayoutResult& result) const;
    void applyOverflow(TextLayoutResult& result) const;

    std::vector<TextRun> runs_;
    float maxWidth_ = 0;       // 0 = no wrap
    float maxHeight_ = 0;      // 0 = no limit
    int maxLines_ = 0;         // 0 = no limit
    TextAlign align_ = TextAlign::Left;
    TextOverflow overflow_ = TextOverflow::Clip;
    LineBreakMode breakMode_ = LineBreakMode::Word;
    TextDirection direction_ = TextDirection::LTR;
    float lineSpacing_ = 0;    // Extra spacing between lines
    float indent_ = 0;         // First line indent
};

} // namespace NXRender
