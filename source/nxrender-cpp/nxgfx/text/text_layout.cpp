// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "nxgfx/text/text_layout.h"
#include "nxgfx/context.h"
#include <algorithm>
#include <cmath>

namespace NXRender {

TextLayout::TextLayout() {}
TextLayout::~TextLayout() {}

void TextLayout::addRun(const TextRun& run) {
    runs_.push_back(run);
}

void TextLayout::setText(const std::string& text, float fontSize, const Color& color) {
    clearRuns();
    runs_.emplace_back(text, fontSize, color);
}

void TextLayout::clearRuns() {
    runs_.clear();
}

float TextLayout::measureWidth(const std::string& text, float fontSize) const {
    // Approximate: use average character width from font metrics
    // Real implementation should query FreeType via glyph_cache
    float avgCharWidth = fontSize * 0.55f;
    return static_cast<float>(text.size()) * avgCharWidth;
}

float TextLayout::measureHeight(const std::string& text, float fontSize, float maxWidth) const {
    if (text.empty()) return 0;
    float lineHeight = fontSize * 1.4f;
    if (maxWidth <= 0) return lineHeight;

    float textWidth = measureWidth(text, fontSize);
    int lineCount = std::max(1, static_cast<int>(std::ceil(textWidth / maxWidth)));
    return static_cast<float>(lineCount) * lineHeight;
}

std::vector<TextLayout::WordSegment> TextLayout::segmentRuns() const {
    std::vector<WordSegment> segments;

    for (size_t r = 0; r < runs_.size(); r++) {
        const auto& run = runs_[r];
        const std::string& text = run.text;
        size_t i = 0;

        while (i < text.size()) {
            // Newline
            if (text[i] == '\n') {
                WordSegment seg;
                seg.runIndex = r;
                seg.startChar = i;
                seg.endChar = i + 1;
                seg.width = 0;
                seg.isWhitespace = true;
                seg.isNewline = true;
                segments.push_back(seg);
                i++;
                continue;
            }

            // Whitespace
            if (text[i] == ' ' || text[i] == '\t') {
                size_t start = i;
                while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) i++;
                WordSegment seg;
                seg.runIndex = r;
                seg.startChar = start;
                seg.endChar = i;
                seg.width = static_cast<float>(i - start) * run.fontSize * 0.55f;
                if (text[start] == '\t') seg.width = run.fontSize * 4.0f; // Tab = 4 spaces
                seg.isWhitespace = true;
                seg.isNewline = false;
                segments.push_back(seg);
                continue;
            }

            // Word
            size_t start = i;
            while (i < text.size() && text[i] != ' ' && text[i] != '\t' && text[i] != '\n') {
                i++;
            }
            WordSegment seg;
            seg.runIndex = r;
            seg.startChar = start;
            seg.endChar = i;
            seg.width = measureWidth(text.substr(start, i - start), run.fontSize);
            seg.isWhitespace = false;
            seg.isNewline = false;
            segments.push_back(seg);
        }
    }

    return segments;
}

TextLayoutResult TextLayout::compute() {
    TextLayoutResult result;
    if (runs_.empty()) return result;

    auto segments = segmentRuns();
    if (segments.empty()) return result;

    float curX = indent_;
    float curY = 0;
    float lineWidth = 0;
    float lineHeight = 0;
    float lineAscent = 0;
    float lineDescent = 0;
    size_t lineStartGlyph = 0;
    size_t lineGlyphCount = 0;
    int lineCount = 0;
    bool firstLine = true;

    auto finishLine = [&](bool isWrapped) {
        if (lineGlyphCount == 0 && lineHeight <= 0) {
            // Empty line from newline
            lineHeight = runs_.empty() ? 20.0f : runs_[0].fontSize * 1.4f;
        }

        TextLine line;
        line.x = 0;
        line.y = curY;
        line.width = lineWidth;
        line.height = lineHeight;
        line.baseline = lineAscent;
        line.ascent = lineAscent;
        line.descent = lineDescent;
        line.startGlyph = lineStartGlyph;
        line.glyphCount = lineGlyphCount;
        line.isLastLine = false;
        line.isWrapped = isWrapped;
        result.lines.push_back(line);

        result.totalWidth = std::max(result.totalWidth, lineWidth);
        curY += lineHeight + lineSpacing_;
        lineCount++;

        curX = 0;
        lineWidth = 0;
        lineHeight = 0;
        lineAscent = 0;
        lineDescent = 0;
        lineStartGlyph = result.glyphs.size();
        lineGlyphCount = 0;
        firstLine = false;
    };

    for (size_t si = 0; si < segments.size(); si++) {
        const auto& seg = segments[si];
        const auto& run = runs_[seg.runIndex];

        // Check max lines
        if (maxLines_ > 0 && lineCount >= maxLines_) break;

        // Check max height
        if (maxHeight_ > 0 && curY + run.fontSize * 1.4f > maxHeight_) break;

        // Newline
        if (seg.isNewline) {
            finishLine(false);
            continue;
        }

        // Check if this segment fits on the current line
        bool fits = (maxWidth_ <= 0) || (curX + seg.width <= maxWidth_);

        if (!fits && lineGlyphCount > 0) {
            // Wrap: finish current line, put this segment on a new line
            finishLine(true);
        }

        // If it still doesn't fit (single word wider than maxWidth), handle per-char
        if (maxWidth_ > 0 && seg.width > maxWidth_ && breakMode_ != LineBreakMode::Word) {
            const std::string& text = run.text;
            for (size_t c = seg.startChar; c < seg.endChar; c++) {
                float charW = run.fontSize * 0.55f;
                if (maxWidth_ > 0 && curX + charW > maxWidth_ && lineGlyphCount > 0) {
                    finishLine(true);
                }

                PositionedGlyph glyph;
                glyph.glyphId = static_cast<uint32_t>(text[c]);
                glyph.x = curX;
                glyph.y = curY;
                glyph.advance = charW;
                glyph.width = charW;
                glyph.height = run.fontSize;
                glyph.runIndex = seg.runIndex;
                glyph.charIndex = c;
                result.glyphs.push_back(glyph);

                curX += charW + run.letterSpacing;
                lineWidth = curX;
                lineGlyphCount++;
                result.characterCount++;

                float lh = run.fontSize * run.lineHeightMultiplier;
                lineHeight = std::max(lineHeight, lh);
                lineAscent = std::max(lineAscent, run.fontSize * 0.8f);
                lineDescent = std::max(lineDescent, run.fontSize * 0.2f);
            }
            continue;
        }

        // Place the segment
        if (!seg.isWhitespace) {
            const std::string& text = run.text;
            for (size_t c = seg.startChar; c < seg.endChar; c++) {
                float charW = run.fontSize * 0.55f;

                PositionedGlyph glyph;
                glyph.glyphId = static_cast<uint32_t>(text[c]);
                glyph.x = curX;
                glyph.y = curY;
                glyph.advance = charW;
                glyph.width = charW;
                glyph.height = run.fontSize;
                glyph.runIndex = seg.runIndex;
                glyph.charIndex = c;
                result.glyphs.push_back(glyph);

                curX += charW + run.letterSpacing;
                lineGlyphCount++;
                result.characterCount++;
            }
        } else {
            curX += seg.width;
        }

        lineWidth = curX;
        float lh = run.fontSize * run.lineHeightMultiplier;
        lineHeight = std::max(lineHeight, lh);
        lineAscent = std::max(lineAscent, run.fontSize * 0.8f);
        lineDescent = std::max(lineDescent, run.fontSize * 0.2f);
    }

    // Finish the last line
    if (lineGlyphCount > 0 || result.lines.empty()) {
        finishLine(false);
    }

    if (!result.lines.empty()) {
        result.lines.back().isLastLine = true;
    }

    result.totalHeight = curY;

    // Apply alignment
    applyAlignment(result);

    // Apply overflow
    applyOverflow(result);

    return result;
}

void TextLayout::applyAlignment(TextLayoutResult& result) const {
    if (maxWidth_ <= 0 || align_ == TextAlign::Left) return;

    for (auto& line : result.lines) {
        float slack = maxWidth_ - line.width;
        if (slack <= 0) continue;

        float offset = 0;
        switch (align_) {
            case TextAlign::Center:
                offset = slack * 0.5f;
                break;
            case TextAlign::Right:
                offset = slack;
                break;
            case TextAlign::Justify:
                if (!line.isLastLine && line.glyphCount > 1) {
                    // Distribute slack among glyphs
                    float perGlyph = slack / static_cast<float>(line.glyphCount - 1);
                    for (size_t g = line.startGlyph + 1;
                         g < line.startGlyph + line.glyphCount && g < result.glyphs.size(); g++) {
                        size_t idx = g - line.startGlyph;
                        result.glyphs[g].x += perGlyph * static_cast<float>(idx);
                    }
                }
                continue;
            default:
                continue;
        }

        for (size_t g = line.startGlyph;
             g < line.startGlyph + line.glyphCount && g < result.glyphs.size(); g++) {
            result.glyphs[g].x += offset;
        }
        line.x += offset;
    }
}

void TextLayout::applyOverflow(TextLayoutResult& result) const {
    if (overflow_ == TextOverflow::Clip) return;

    // Ellipsis: if last visible line overflows, truncate and append "…"
    if (overflow_ == TextOverflow::Ellipsis && !result.lines.empty()) {
        auto& lastLine = result.lines.back();
        if (maxWidth_ > 0 && lastLine.width > maxWidth_) {
            float ellipsisWidth = 12.0f; // Approximate width of "…"
            float targetWidth = maxWidth_ - ellipsisWidth;

            // Remove glyphs from the end until we fit
            while (lastLine.glyphCount > 0) {
                size_t lastIdx = lastLine.startGlyph + lastLine.glyphCount - 1;
                if (lastIdx >= result.glyphs.size()) break;

                float glyphEnd = result.glyphs[lastIdx].x + result.glyphs[lastIdx].advance;
                if (glyphEnd <= targetWidth) break;

                lastLine.glyphCount--;
                result.glyphs.pop_back();
            }

            // Add ellipsis glyph
            if (lastLine.glyphCount > 0) {
                size_t lastIdx = lastLine.startGlyph + lastLine.glyphCount - 1;
                float insertX = result.glyphs[lastIdx].x + result.glyphs[lastIdx].advance;

                PositionedGlyph ellipsis;
                ellipsis.glyphId = 0x2026; // '…'
                ellipsis.x = insertX;
                ellipsis.y = result.glyphs[lastIdx].y;
                ellipsis.advance = ellipsisWidth;
                ellipsis.width = ellipsisWidth;
                ellipsis.height = result.glyphs[lastIdx].height;
                ellipsis.runIndex = result.glyphs[lastIdx].runIndex;
                ellipsis.charIndex = result.characterCount;
                result.glyphs.push_back(ellipsis);
                lastLine.glyphCount++;
            }
        }
    }
}

TextHitResult TextLayout::hitTest(float x, float y, const TextLayoutResult& layout) const {
    TextHitResult hit;
    if (layout.lines.empty()) return hit;

    // Find the line
    size_t lineIdx = 0;
    for (size_t i = 0; i < layout.lines.size(); i++) {
        if (y >= layout.lines[i].y && y < layout.lines[i].y + layout.lines[i].height) {
            lineIdx = i;
            break;
        }
        if (i == layout.lines.size() - 1) lineIdx = i;
    }

    const auto& line = layout.lines[lineIdx];
    hit.lineIndex = lineIdx;
    hit.isValid = true;

    // Find the glyph within the line
    for (size_t g = line.startGlyph; g < line.startGlyph + line.glyphCount && g < layout.glyphs.size(); g++) {
        const auto& glyph = layout.glyphs[g];
        float mid = glyph.x + glyph.advance * 0.5f;

        if (x <= mid) {
            hit.glyphIndex = g;
            hit.charIndex = glyph.charIndex;
            hit.runIndex = glyph.runIndex;
            hit.insertX = glyph.x;
            hit.isLeading = true;
            return hit;
        }
    }

    // After last glyph
    if (line.glyphCount > 0) {
        size_t lastG = line.startGlyph + line.glyphCount - 1;
        if (lastG < layout.glyphs.size()) {
            hit.glyphIndex = lastG;
            hit.charIndex = layout.glyphs[lastG].charIndex + 1;
            hit.runIndex = layout.glyphs[lastG].runIndex;
            hit.insertX = layout.glyphs[lastG].x + layout.glyphs[lastG].advance;
            hit.isLeading = false;
        }
    }

    return hit;
}

Rect TextLayout::caretRect(size_t charIndex, const TextLayoutResult& layout) const {
    if (layout.glyphs.empty()) {
        if (layout.lines.empty()) return Rect(0, 0, 2, 20);
        return Rect(layout.lines[0].x, layout.lines[0].y, 2, layout.lines[0].height);
    }

    // Find the glyph for this char index
    for (size_t g = 0; g < layout.glyphs.size(); g++) {
        if (layout.glyphs[g].charIndex >= charIndex) {
            // Find which line this glyph is on
            for (const auto& line : layout.lines) {
                if (g >= line.startGlyph && g < line.startGlyph + line.glyphCount) {
                    return Rect(layout.glyphs[g].x, line.y, 2, line.height);
                }
            }
        }
    }

    // Past end: position after last glyph
    const auto& last = layout.glyphs.back();
    for (const auto& line : layout.lines) {
        size_t end = line.startGlyph + line.glyphCount;
        if (layout.glyphs.size() == end) {
            return Rect(last.x + last.advance, line.y, 2, line.height);
        }
    }

    return Rect(last.x + last.advance, last.y, 2, last.height);
}

std::vector<Rect> TextLayout::selectionRects(size_t startChar, size_t endChar,
                                              const TextLayoutResult& layout) const {
    std::vector<Rect> rects;
    if (layout.glyphs.empty() || startChar >= endChar) return rects;

    for (const auto& line : layout.lines) {
        float selStart = -1, selEnd = -1;

        for (size_t g = line.startGlyph;
             g < line.startGlyph + line.glyphCount && g < layout.glyphs.size(); g++) {
            size_t ci = layout.glyphs[g].charIndex;
            if (ci >= startChar && ci < endChar) {
                if (selStart < 0) selStart = layout.glyphs[g].x;
                selEnd = layout.glyphs[g].x + layout.glyphs[g].advance;
            }
        }

        if (selStart >= 0 && selEnd > selStart) {
            rects.emplace_back(selStart, line.y, selEnd - selStart, line.height);
        }
    }

    return rects;
}

void TextLayout::render(GpuContext* ctx, float originX, float originY,
                         const TextLayoutResult& layout) const {
    if (!ctx || layout.glyphs.empty()) return;

    for (const auto& glyph : layout.glyphs) {
        if (glyph.runIndex >= runs_.size()) continue;
        const auto& run = runs_[glyph.runIndex];

        char ch[2] = {static_cast<char>(glyph.glyphId & 0x7F), 0};
        ctx->drawText(std::string(ch), originX + glyph.x,
                      originY + glyph.y, run.color, run.fontSize);
    }

    // Draw underlines and strikethroughs
    for (size_t r = 0; r < runs_.size(); r++) {
        const auto& run = runs_[r];
        if (!run.underline && !run.strikethrough) continue;

        for (const auto& line : layout.lines) {
            float startX = -1, endX = -1;
            for (size_t g = line.startGlyph;
                 g < line.startGlyph + line.glyphCount && g < layout.glyphs.size(); g++) {
                if (layout.glyphs[g].runIndex == r) {
                    if (startX < 0) startX = layout.glyphs[g].x;
                    endX = layout.glyphs[g].x + layout.glyphs[g].advance;
                }
            }

            if (startX >= 0 && endX > startX) {
                if (run.underline) {
                    float y = originY + line.y + line.ascent + 2;
                    ctx->drawLine(originX + startX, y, originX + endX, y, run.color, 1.0f);
                }
                if (run.strikethrough) {
                    float y = originY + line.y + line.ascent * 0.5f;
                    ctx->drawLine(originX + startX, y, originX + endX, y, run.color, 1.0f);
                }
            }
        }
    }
}

} // namespace NXRender
