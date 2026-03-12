// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file writing_mode.h
 * @brief Writing mode support for logical coordinates
 * 
 * Provides logical coordinate system (inline/block) instead of 
 * physical (x/y) for proper RTL and vertical text support.
 * Inspired by CSS Writing Modes Level 4.
 */

#pragma once

#include "../nxgfx/primitives.h"
#include <cstdint>

namespace NXRender {

/**
 * @brief Text direction
 */
enum class TextDirection : uint8_t {
    LTR,    // Left-to-right (English, etc.)
    RTL     // Right-to-left (Arabic, Hebrew, etc.)
};

/**
 * @brief Writing mode (block flow direction)
 */
enum class WritingMode : uint8_t {
    HorizontalTB,   // Horizontal, top-to-bottom (default)
    VerticalRL,     // Vertical, right-to-left (traditional CJK)
    VerticalLR,     // Vertical, left-to-right (Mongolian)
    SidewaysRL,     // Sideways right-to-left
    SidewaysLR      // Sideways left-to-right
};

/**
 * @brief Combined writing mode info
 */
struct WritingModeInfo {
    WritingMode mode = WritingMode::HorizontalTB;
    TextDirection direction = TextDirection::LTR;
    
    bool isVertical() const {
        return mode != WritingMode::HorizontalTB;
    }
    
    bool isHorizontal() const {
        return mode == WritingMode::HorizontalTB;
    }
    
    bool isRTL() const {
        return direction == TextDirection::RTL;
    }
    
    bool isLTR() const {
        return direction == TextDirection::LTR;
    }
    
    // Check if block axis is reversed
    bool isBlockAxisReversed() const {
        return mode == WritingMode::VerticalRL || mode == WritingMode::SidewaysRL;
    }
};

/**
 * @brief Logical size (inline-size × block-size)
 * 
 * In horizontal-tb:
 *   - iSize = width
 *   - bSize = height
 * 
 * In vertical modes:
 *   - iSize = height
 *   - bSize = width
 */
struct LogicalSize {
    float iSize = 0;  // Inline size (follows text flow)
    float bSize = 0;  // Block size (perpendicular to text)
    
    LogicalSize() = default;
    LogicalSize(float i, float b) : iSize(i), bSize(b) {}
    
    // Convert to physical size
    Size toPhysical(WritingMode wm) const {
        if (wm == WritingMode::HorizontalTB) {
            return Size{iSize, bSize};
        }
        return Size{bSize, iSize};  // Swap for vertical modes
    }
    
    // Create from physical size
    static LogicalSize fromPhysical(const Size& s, WritingMode wm) {
        if (wm == WritingMode::HorizontalTB) {
            return {s.width, s.height};
        }
        return {s.height, s.width};
    }
    
    // Operators
    LogicalSize operator+(const LogicalSize& o) const {
        return {iSize + o.iSize, bSize + o.bSize};
    }
    
    LogicalSize operator-(const LogicalSize& o) const {
        return {iSize - o.iSize, bSize - o.bSize};
    }
    
    bool operator==(const LogicalSize& o) const {
        return iSize == o.iSize && bSize == o.bSize;
    }
};

/**
 * @brief Logical point (inline-start, block-start)
 */
struct LogicalPoint {
    float i = 0;  // Inline position
    float b = 0;  // Block position
    
    LogicalPoint() = default;
    LogicalPoint(float iPos, float bPos) : i(iPos), b(bPos) {}
    
    // Convert to physical point
    Point toPhysical(WritingMode wm, TextDirection dir, const Size& containerSize) const {
        if (wm == WritingMode::HorizontalTB) {
            float x = (dir == TextDirection::LTR) ? i : (containerSize.width - i);
            return Point{x, b};
        }
        // Vertical modes
        float x = (wm == WritingMode::VerticalRL) ? (containerSize.width - b) : b;
        float y = (dir == TextDirection::LTR) ? i : (containerSize.height - i);
        return Point{x, y};
    }
    
    static LogicalPoint fromPhysical(const Point& p, WritingMode wm, 
                                     TextDirection dir, const Size& containerSize) {
        if (wm == WritingMode::HorizontalTB) {
            float i = (dir == TextDirection::LTR) ? p.x : (containerSize.width - p.x);
            return {i, p.y};
        }
        float b = (wm == WritingMode::VerticalRL) ? (containerSize.width - p.x) : p.x;
        float i = (dir == TextDirection::LTR) ? p.y : (containerSize.height - p.y);
        return {i, b};
    }
};

/**
 * @brief Logical margin/padding (inline-start, inline-end, block-start, block-end)
 */
struct LogicalMargin {
    float iStart = 0;   // Inline-start (left in horizontal-tb LTR)
    float iEnd = 0;     // Inline-end (right in horizontal-tb LTR)
    float bStart = 0;   // Block-start (top in horizontal-tb)
    float bEnd = 0;     // Block-end (bottom in horizontal-tb)
    
    LogicalMargin() = default;
    LogicalMargin(float is, float ie, float bs, float be)
        : iStart(is), iEnd(ie), bStart(bs), bEnd(be) {}
    
    // Create uniform margin
    static LogicalMargin uniform(float v) {
        return {v, v, v, v};
    }
    
    // Get inline extent
    float inlineSum() const { return iStart + iEnd; }
    
    // Get block extent
    float blockSum() const { return bStart + bEnd; }
    
    // Convert to physical EdgeInsets
    EdgeInsets toPhysical(WritingMode wm, TextDirection dir) const {
        if (wm == WritingMode::HorizontalTB) {
            if (dir == TextDirection::LTR) {
                return {bStart, iEnd, bEnd, iStart};  // top, right, bottom, left
            }
            return {bStart, iStart, bEnd, iEnd};  // Swap left/right for RTL
        }
        // Vertical modes
        if (wm == WritingMode::VerticalRL) {
            if (dir == TextDirection::LTR) {
                return {iStart, bStart, iEnd, bEnd};
            }
            return {iEnd, bStart, iStart, bEnd};
        }
        // VerticalLR
        if (dir == TextDirection::LTR) {
            return {iStart, bEnd, iEnd, bStart};
        }
        return {iEnd, bEnd, iStart, bStart};
    }
    
    static LogicalMargin fromPhysical(const EdgeInsets& e, WritingMode wm, TextDirection dir) {
        if (wm == WritingMode::HorizontalTB) {
            if (dir == TextDirection::LTR) {
                return {e.left, e.right, e.top, e.bottom};
            }
            return {e.right, e.left, e.top, e.bottom};
        }
        // Vertical modes - simplified
        return {e.top, e.bottom, e.left, e.right};
    }
    
    LogicalMargin operator+(const LogicalMargin& o) const {
        return {iStart + o.iStart, iEnd + o.iEnd, bStart + o.bStart, bEnd + o.bEnd};
    }
    
    LogicalMargin operator-(const LogicalMargin& o) const {
        return {iStart - o.iStart, iEnd - o.iEnd, bStart - o.bStart, bEnd - o.bEnd};
    }
};

/**
 * @brief Logical rect (position + size)
 */
struct LogicalRect {
    LogicalPoint origin;
    LogicalSize size;
    
    LogicalRect() = default;
    LogicalRect(const LogicalPoint& o, const LogicalSize& s) : origin(o), size(s) {}
    LogicalRect(float i, float b, float iSize, float bSize)
        : origin(i, b), size(iSize, bSize) {}
    
    float iStart() const { return origin.i; }
    float bStart() const { return origin.b; }
    float iEnd() const { return origin.i + size.iSize; }
    float bEnd() const { return origin.b + size.bSize; }
    
    Rect toPhysical(WritingMode wm, TextDirection dir, const Size& containerSize) const {
        Point physOrigin = origin.toPhysical(wm, dir, containerSize);
        Size physSize = size.toPhysical(wm);
        return Rect{physOrigin.x, physOrigin.y, physSize.width, physSize.height};
    }
};

} // namespace NXRender
