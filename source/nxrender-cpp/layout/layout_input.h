// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file layout_input.h
 * @brief Layout constraint system (inspired by Gecko's ReflowInput)
 * 
 * Provides constraint propagation for layout, handling:
 * - Available size
 * - Computed size with min/max constraints
 * - Margin/padding computation
 * - Parent layout chain
 */

#pragma once

#include "writing_mode.h"
#include "../nxgfx/primitives.h"
#include <algorithm>
#include <optional>

namespace NXRender {

class Widget;

/**
 * @brief Constrained size value (may be unconstrained)
 */
constexpr float UNCONSTRAINED_SIZE = 1e9f;

inline bool isConstrained(float size) {
    return size < UNCONSTRAINED_SIZE * 0.5f;
}

/**
 * @brief Box sizing mode
 */
enum class BoxSizing {
    ContentBox,   // Width/height applies to content
    BorderBox     // Width/height includes padding and border
};

/**
 * @brief Layout flags
 */
struct LayoutFlags {
    bool isIResize : 1 = false;      // Inline size is changing
    bool isBResize : 1 = false;      // Block size is changing
    bool shrinkWrap : 1 = false;     // Use minimum content size
    bool stretchInline : 1 = false;  // Stretch to fill inline axis
    bool stretchBlock : 1 = false;   // Stretch to fill block axis
    bool isReplaced : 1 = false;     // Replaced element (img, video)
    
    LayoutFlags() = default;
};

/**
 * @brief Layout input/constraints for a widget
 */
class LayoutInput {
public:
    // The widget being laid out
    Widget* widget = nullptr;
    
    // Writing mode info
    WritingModeInfo writingMode;
    
    // Parent layout input (for constraint inheritance)
    const LayoutInput* parent = nullptr;
    
    // ========================================================================
    // Available Space
    // ========================================================================
    
    LogicalSize availableSize() const { return availableSize_; }
    
    float availableISize() const { return availableSize_.iSize; }
    float availableBSize() const { return availableSize_.bSize; }
    
    void setAvailableSize(const LogicalSize& size) { availableSize_ = size; }
    void setAvailableISize(float size) { availableSize_.iSize = size; }
    void setAvailableBSize(float size) { availableSize_.bSize = size; }
    
    // ========================================================================
    // Computed Size (after constraints applied)
    // ========================================================================
    
    LogicalSize computedSize() const { return computedSize_; }
    
    float computedISize() const { return computedSize_.iSize; }
    float computedBSize() const { return computedSize_.bSize; }
    
    void setComputedISize(float size) { computedSize_.iSize = size; }
    void setComputedBSize(float size) { computedSize_.bSize = size; }
    
    // ========================================================================
    // Min/Max Constraints
    // ========================================================================
    
    float computedMinISize() const { return minSize_.iSize; }
    float computedMaxISize() const { return maxSize_.iSize; }
    float computedMinBSize() const { return minSize_.bSize; }
    float computedMaxBSize() const { return maxSize_.bSize; }
    
    void setMinISize(float size) { minSize_.iSize = size; }
    void setMaxISize(float size) { maxSize_.iSize = size; }
    void setMinBSize(float size) { minSize_.bSize = size; }
    void setMaxBSize(float size) { maxSize_.bSize = size; }
    
    /**
     * @brief Apply min/max constraints to inline size
     * @param size Input size
     * @return Clamped size
     */
    float applyMinMaxISize(float size) const {
        // CSS says: if max < min, min wins
        float result = size;
        if (isConstrained(maxSize_.iSize)) {
            result = std::min(result, maxSize_.iSize);
        }
        result = std::max(result, minSize_.iSize);
        return result;
    }
    
    /**
     * @brief Apply min/max constraints to block size
     * @param size Input size
     * @param consumed Amount already consumed by prev fragments
     * @return Clamped size
     */
    float applyMinMaxBSize(float size, float consumed = 0) const {
        size += consumed;
        
        if (isConstrained(maxSize_.bSize)) {
            size = std::min(size, maxSize_.bSize);
        }
        size = std::max(size, minSize_.bSize);
        
        return size - consumed;
    }
    
    // ========================================================================
    // Margin & Padding
    // ========================================================================
    
    LogicalMargin computedMargin() const { return margin_; }
    LogicalMargin computedPadding() const { return padding_; }
    LogicalMargin computedBorderPadding() const { return borderPadding_; }
    
    void setComputedMargin(const LogicalMargin& m) { margin_ = m; }
    void setComputedPadding(const LogicalMargin& p) { padding_ = p; }
    void setComputedBorderPadding(const LogicalMargin& bp) { borderPadding_ = bp; }
    
    // ========================================================================
    // Containing Block
    // ========================================================================
    
    LogicalSize containingBlockSize() const { return containingBlockSize_; }
    void setContainingBlockSize(const LogicalSize& size) { containingBlockSize_ = size; }
    
    // ========================================================================
    // Flags
    // ========================================================================
    
    LayoutFlags flags;
    
    // ========================================================================
    // Convenience Methods
    // ========================================================================
    
    /**
     * @brief Get content box size (computed size minus padding/border)
     */
    LogicalSize contentBoxSize() const {
        return {
            computedSize_.iSize - borderPadding_.inlineSum(),
            computedSize_.bSize - borderPadding_.blockSum()
        };
    }
    
    /**
     * @brief Get border box size (same as computed for border-box sizing)
     */
    LogicalSize borderBoxSize() const {
        return computedSize_;
    }
    
    /**
     * @brief Get margin box size
     */
    LogicalSize marginBoxSize() const {
        return {
            computedSize_.iSize + margin_.inlineSum(),
            computedSize_.bSize + margin_.blockSum()
        };
    }
    
    /**
     * @brief Resolve percentage value against containing block
     * @param percent Percentage (0-100)
     * @param inlineAxis True for inline axis, false for block
     */
    float resolvePercentage(float percent, bool inlineAxis) const {
        float base = inlineAxis ? containingBlockSize_.iSize : containingBlockSize_.bSize;
        if (!isConstrained(base)) return 0;
        return base * percent / 100.0f;
    }
    
    /**
     * @brief Create child layout input
     */
    LayoutInput forChild(Widget* child) const {
        LayoutInput result;
        result.widget = child;
        result.parent = this;
        result.writingMode = writingMode;  // Inherit by default
        result.containingBlockSize_ = contentBoxSize();
        result.availableSize_ = contentBoxSize();
        return result;
    }
    
private:
    LogicalSize availableSize_;
    LogicalSize computedSize_;
    LogicalSize minSize_;
    LogicalSize maxSize_ = {UNCONSTRAINED_SIZE, UNCONSTRAINED_SIZE};
    LogicalMargin margin_;
    LogicalMargin padding_;
    LogicalMargin borderPadding_;
    LogicalSize containingBlockSize_;
};

/**
 * @brief Layout output/result
 */
struct LayoutOutput {
    LogicalSize size;           // Final measured size
    float ascent = 0;           // Baseline from top (for inline alignment)
    float descent = 0;          // Below baseline
    bool hasOverflow = false;   // Content overflows box
    
    // For fragmentation (pagination/columns)
    bool isComplete = true;     // All content laid out
    float consumedBSize = 0;    // Block size consumed so far
};

} // namespace NXRender
