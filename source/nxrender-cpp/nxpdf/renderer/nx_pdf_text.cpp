// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "nx_pdf_graphics.h"
#include <string>
#include <cstdint>
#include <vector>
#include <cmath>

namespace nxrender {
namespace pdf {
namespace renderer {

// ==================================================================
// PDF Text State (BT/ET block parameters)
// ==================================================================

struct TextState {
    double fontSize = 12.0;
    double charSpacing = 0;
    double wordSpacing = 0;
    double horizontalScaling = 100.0; // percentage
    double leading = 0;
    double rise = 0;
    int renderMode = 0; // 0=fill, 1=stroke, 2=fill+stroke, 3=invisible
    Matrix textMatrix;
    Matrix textLineMatrix;
    std::string fontName;
    bool isKnockout = false;
};

// ==================================================================
// Glyph positioning
// ==================================================================

struct PositionedGlyph {
    uint32_t charCode;
    double x, y;
    double width;
    double fontSize;
    std::string fontName;
};

// ==================================================================
// Text rendering engine
// ==================================================================

class TextRenderer {
public:
    TextRenderer() = default;

    void beginText() {
        state_.textMatrix = Matrix();
        state_.textLineMatrix = Matrix();
        inTextBlock_ = true;
    }

    void endText() {
        inTextBlock_ = false;
    }

    void setFont(const std::string& name, double size) {
        state_.fontName = name;
        state_.fontSize = size;
    }

    void setCharSpacing(double spacing) { state_.charSpacing = spacing; }
    void setWordSpacing(double spacing) { state_.wordSpacing = spacing; }
    void setHorizontalScaling(double scale) { state_.horizontalScaling = scale; }
    void setLeading(double leading) { state_.leading = leading; }
    void setRise(double rise) { state_.rise = rise; }
    void setRenderMode(int mode) { state_.renderMode = mode; }

    void setTextMatrix(double a, double b, double c, double d,
                        double e, double f) {
        state_.textMatrix = {a, b, c, d, e, f};
        state_.textLineMatrix = state_.textMatrix;
    }

    // Td: Move to start of next text line
    void moveTextPosition(double tx, double ty) {
        Matrix translate;
        translate.e = tx;
        translate.f = ty;
        state_.textLineMatrix.Concat(translate);
        state_.textMatrix = state_.textLineMatrix;
    }

    // TD: same as Td but also sets leading
    void moveTextPositionAndSetLeading(double tx, double ty) {
        state_.leading = -ty;
        moveTextPosition(tx, ty);
    }

    // T*: Move to start of next line (uses leading)
    void nextLine() {
        moveTextPosition(0, -state_.leading);
    }

    // Tj: Show text string
    std::vector<PositionedGlyph> showText(const std::string& text,
                                            const GraphicsState& gs) {
        std::vector<PositionedGlyph> glyphs;
        if (!inTextBlock_) return glyphs;

        double horizontalScale = state_.horizontalScaling / 100.0;

        for (size_t i = 0; i < text.size(); i++) {
            uint32_t charCode = static_cast<uint8_t>(text[i]);

            // Apply text rendering matrix: Trm = Tm × CTM
            Matrix trm = state_.textMatrix;
            trm.Concat(gs.ctm);

            PositionedGlyph glyph;
            glyph.charCode = charCode;
            glyph.x = trm.e;
            glyph.y = trm.f + state_.rise;
            glyph.fontSize = state_.fontSize;
            glyph.fontName = state_.fontName;

            // Approximate glyph width (should come from font metrics)
            double glyphWidth = state_.fontSize * 0.6;
            glyph.width = glyphWidth;

            glyphs.push_back(glyph);

            // Advance text position
            double tx = (glyphWidth * horizontalScale + state_.charSpacing);
            if (charCode == ' ') {
                tx += state_.wordSpacing;
            }

            Matrix advance;
            advance.e = tx;
            state_.textMatrix.Concat(advance);
        }

        return glyphs;
    }

    // TJ: Show text with individual glyph positioning
    std::vector<PositionedGlyph> showTextArray(
            const std::vector<std::string>& strings,
            const std::vector<double>& adjustments,
            const GraphicsState& gs) {
        std::vector<PositionedGlyph> allGlyphs;

        size_t adjIdx = 0;
        for (size_t i = 0; i < strings.size(); i++) {
            // Apply adjustment before this string
            if (adjIdx < adjustments.size()) {
                double adj = adjustments[adjIdx++];
                // PDF TJ adjustments are in thousandths of a unit of text space
                Matrix adjust;
                adjust.e = -adj / 1000.0 * state_.fontSize;
                state_.textMatrix.Concat(adjust);
            }

            auto glyphs = showText(strings[i], gs);
            allGlyphs.insert(allGlyphs.end(), glyphs.begin(), glyphs.end());
        }

        return allGlyphs;
    }

    const TextState& textState() const { return state_; }

    bool inTextBlock() const { return inTextBlock_; }

    // Compute text width for a string
    double measureTextWidth(const std::string& text) const {
        double horizontalScale = state_.horizontalScaling / 100.0;
        double width = 0;
        for (size_t i = 0; i < text.size(); i++) {
            double glyphWidth = state_.fontSize * 0.6;
            width += glyphWidth * horizontalScale + state_.charSpacing;
            if (text[i] == ' ') width += state_.wordSpacing;
        }
        return width;
    }

private:
    TextState state_;
    bool inTextBlock_ = false;
};

} // namespace renderer
} // namespace pdf
} // namespace nxrender
