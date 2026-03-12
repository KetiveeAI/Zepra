// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file line_layout.h
 * @brief Inline box model and line layout
 * 
 * Handles inline formatting context:
 * - Text flow
 * - Inline boxes
 * - Baseline alignment
 * - Line breaking
 */

#pragma once

#include "writing_mode.h"
#include "float_manager.h"
#include "../nxgfx/primitives.h"
#include <vector>
#include <string>
#include <algorithm>

namespace NXRender {

/**
 * @brief Baseline type
 */
enum class BaselineType {
    Alphabetic,     // Default for text
    Ideographic,    // CJK bottom
    Central,        // Central baseline
    Mathematical,   // Math baseline
    Hanging,        // Top hanging baseline
    TextTop,        // Top of text
    TextBottom      // Bottom of text
};

/**
 * @brief Vertical alignment
 */
enum class VerticalAlign {
    Baseline,
    Sub,
    Super,
    TextTop,
    TextBottom,
    Middle,
    Top,
    Bottom,
    // Explicit length handled separately
};

/**
 * @brief Text alignment
 */
enum class TextAlign {
    Start,
    End,
    Left,
    Right,
    Center,
    Justify
};

/**
 * @brief A fragment of inline content
 */
struct InlineFragment {
    float width = 0;
    float height = 0;
    float ascent = 0;      // Distance from baseline to top
    float descent = 0;     // Distance from baseline to bottom
    float xOffset = 0;     // Position in line
    float yOffset = 0;     // Vertical offset from baseline
    
    VerticalAlign verticalAlign = VerticalAlign::Baseline;
    
    // Content reference (text range or inline box)
    int contentStart = 0;
    int contentEnd = 0;
    bool isText = true;
    
    float baseline() const { return ascent; }
    float lineHeight() const { return ascent + descent; }
};

/**
 * @brief A line of inline content
 */
struct LineBox {
    float x = 0;           // Line start x
    float y = 0;           // Line top y
    float width = 0;       // Line width
    float height = 0;      // Line height
    float baseline = 0;    // Baseline position from top
    
    std::vector<InlineFragment> fragments;
    
    float top() const { return y; }
    float bottom() const { return y + height; }
    float left() const { return x; }
    float right() const { return x + width; }
    
    bool isEmpty() const { return fragments.empty(); }
};

/**
 * @brief Line layout engine
 */
class LineLayout {
public:
    LineLayout() = default;
    
    /**
     * @brief Begin a new line layout
     * @param availableWidth Width available for content
     * @param floatManager Optional float manager for float intrusion
     */
    void beginLayout(float availableWidth, FloatManager* floatManager = nullptr) {
        availableWidth_ = availableWidth;
        floatManager_ = floatManager;
        lines_.clear();
        currentLine_ = LineBox{};
        currentX_ = 0;
        currentY_ = 0;
    }
    
    /**
     * @brief Begin a new line
     * @param y Starting y position
     */
    void beginLine(float y) {
        if (!currentLine_.isEmpty()) {
            finishLine();
        }
        
        currentLine_ = LineBox{};
        currentLine_.y = y;
        currentY_ = y;
        currentX_ = 0;
        
        // Account for floats
        if (floatManager_) {
            currentX_ = floatManager_->getLeftEdge(y);
            float rightEdge = floatManager_->getRightEdge(y);
            lineAvailableWidth_ = rightEdge - currentX_;
        } else {
            lineAvailableWidth_ = availableWidth_;
        }
    }
    
    /**
     * @brief Place an inline fragment
     * @param frag Fragment to place
     * @return true if placed on current line, false if line break needed
     */
    bool placeFragment(const InlineFragment& frag) {
        if (currentX_ + frag.width > lineAvailableWidth_ && !currentLine_.isEmpty()) {
            return false;  // Need line break
        }
        
        InlineFragment placed = frag;
        placed.xOffset = currentX_;
        
        currentLine_.fragments.push_back(placed);
        currentX_ += frag.width;
        
        return true;
    }
    
    /**
     * @brief Finish current line and compute positions
     */
    void finishLine() {
        if (currentLine_.isEmpty()) return;
        
        // Compute line metrics
        computeLineMetrics();
        
        // Apply text alignment
        applyAlignment();
        
        lines_.push_back(currentLine_);
        currentY_ += currentLine_.height;
    }
    
    /**
     * @brief End layout and return all lines
     */
    const std::vector<LineBox>& endLayout() {
        if (!currentLine_.isEmpty()) {
            finishLine();
        }
        return lines_;
    }
    
    /**
     * @brief Get baseline position for a line
     */
    float getBaseline(int lineIndex) const {
        if (lineIndex >= 0 && lineIndex < static_cast<int>(lines_.size())) {
            return lines_[lineIndex].y + lines_[lineIndex].baseline;
        }
        return 0;
    }
    
    /**
     * @brief Set text alignment
     */
    void setTextAlign(TextAlign align) { textAlign_ = align; }
    
    /**
     * @brief Set explicit line height
     */
    void setLineHeight(float height) { explicitLineHeight_ = height; }
    
    /**
     * @brief Get total height of all lines
     */
    float totalHeight() const {
        if (lines_.empty()) return 0;
        return lines_.back().bottom();
    }
    
private:
    float availableWidth_ = 0;
    float lineAvailableWidth_ = 0;
    FloatManager* floatManager_ = nullptr;
    
    std::vector<LineBox> lines_;
    LineBox currentLine_;
    float currentX_ = 0;
    float currentY_ = 0;
    
    TextAlign textAlign_ = TextAlign::Start;
    float explicitLineHeight_ = 0;
    
    void computeLineMetrics() {
        if (currentLine_.isEmpty()) return;
        
        float maxAscent = 0;
        float maxDescent = 0;
        
        for (auto& frag : currentLine_.fragments) {
            maxAscent = std::max(maxAscent, frag.ascent);
            maxDescent = std::max(maxDescent, frag.descent);
        }
        
        currentLine_.baseline = maxAscent;
        currentLine_.height = maxAscent + maxDescent;
        
        if (explicitLineHeight_ > 0 && explicitLineHeight_ > currentLine_.height) {
            float halfLeading = (explicitLineHeight_ - currentLine_.height) / 2;
            currentLine_.baseline += halfLeading;
            currentLine_.height = explicitLineHeight_;
        }
        
        // Position fragments vertically
        for (auto& frag : currentLine_.fragments) {
            switch (frag.verticalAlign) {
                case VerticalAlign::Baseline:
                    frag.yOffset = currentLine_.baseline - frag.ascent;
                    break;
                case VerticalAlign::Top:
                    frag.yOffset = 0;
                    break;
                case VerticalAlign::Bottom:
                    frag.yOffset = currentLine_.height - frag.lineHeight();
                    break;
                case VerticalAlign::Middle:
                    frag.yOffset = (currentLine_.height - frag.lineHeight()) / 2;
                    break;
                case VerticalAlign::TextTop:
                    frag.yOffset = currentLine_.baseline - maxAscent;
                    break;
                case VerticalAlign::TextBottom:
                    frag.yOffset = currentLine_.baseline + maxDescent - frag.lineHeight();
                    break;
                case VerticalAlign::Sub:
                    frag.yOffset = currentLine_.baseline - frag.ascent + frag.lineHeight() * 0.3f;
                    break;
                case VerticalAlign::Super:
                    frag.yOffset = currentLine_.baseline - frag.ascent - frag.lineHeight() * 0.3f;
                    break;
            }
        }
        
        // Compute line width
        currentLine_.width = currentX_;
    }
    
    void applyAlignment() {
        if (currentLine_.isEmpty()) return;
        
        float usedWidth = currentLine_.width;
        float extraSpace = lineAvailableWidth_ - usedWidth;
        
        if (extraSpace <= 0) return;
        
        float offset = 0;
        
        switch (textAlign_) {
            case TextAlign::Start:
            case TextAlign::Left:
                offset = 0;
                break;
            case TextAlign::End:
            case TextAlign::Right:
                offset = extraSpace;
                break;
            case TextAlign::Center:
                offset = extraSpace / 2;
                break;
            case TextAlign::Justify:
                // Distribute space between fragments
                if (currentLine_.fragments.size() > 1) {
                    float gap = extraSpace / (currentLine_.fragments.size() - 1);
                    float accumOffset = 0;
                    for (auto& frag : currentLine_.fragments) {
                        frag.xOffset += accumOffset;
                        accumOffset += gap;
                    }
                }
                return;
        }
        
        // Apply offset to all fragments
        for (auto& frag : currentLine_.fragments) {
            frag.xOffset += offset;
        }
        
        currentLine_.x = offset;
    }
};

} // namespace NXRender
