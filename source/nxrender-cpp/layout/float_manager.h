// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file float_manager.h
 * @brief CSS float layout manager
 * 
 * Handles float placement and content flow around floats.
 */

#pragma once

#include "writing_mode.h"
#include "../nxgfx/primitives.h"
#include <vector>
#include <algorithm>

namespace NXRender {

/**
 * @brief Float side
 */
enum class FloatSide {
    Left,
    Right,
    InlineStart,
    InlineEnd
};

/**
 * @brief A floating element
 */
struct FloatInfo {
    Rect rect;              // Physical rect of float
    FloatSide side;         // Which side it floats to
    float clearance = 0;    // Space cleared above
};

/**
 * @brief Manages floats within a block formatting context
 */
class FloatManager {
public:
    FloatManager() = default;
    
    /**
     * @brief Set the containing block dimensions
     */
    void setContainingBlock(float width, float height) {
        containerWidth_ = width;
        containerHeight_ = height;
    }
    
    /**
     * @brief Add a float
     * @param side Float side
     * @param size Size of the floating element
     * @param y Current block position
     * @return Rect where float is placed
     */
    Rect addFloat(FloatSide side, const Size& size, float y) {
        // Find available space at this y position
        float x = 0;
        float placementY = y;
        
        if (side == FloatSide::Left || side == FloatSide::InlineStart) {
            // Place at left edge, after any existing left floats
            x = getLeftEdge(y);
            
            // Check if it fits
            float rightEdge = getRightEdge(y);
            if (x + size.width > rightEdge) {
                // Need to move down
                placementY = findSpaceForFloat(size, true, y);
                x = getLeftEdge(placementY);
            }
        } else {
            // Place at right edge
            float rightEdge = getRightEdge(y);
            
            if (rightEdge - size.width < getLeftEdge(y)) {
                placementY = findSpaceForFloat(size, false, y);
                rightEdge = getRightEdge(placementY);
            }
            
            x = rightEdge - size.width;
        }
        
        Rect floatRect{x, placementY, size.width, size.height};
        
        FloatInfo info;
        info.rect = floatRect;
        info.side = side;
        
        if (side == FloatSide::Left || side == FloatSide::InlineStart) {
            leftFloats_.push_back(info);
        } else {
            rightFloats_.push_back(info);
        }
        
        return floatRect;
    }
    
    /**
     * @brief Get available inline space at a given block position
     * @param y Block position
     * @param height Height of content to place
     * @return Available rect
     */
    Rect getAvailableSpace(float y, float height = 0) const {
        float left = getLeftEdge(y);
        float right = getRightEdge(y);
        
        // If height specified, check full height range
        if (height > 0) {
            float checkY = y;
            while (checkY < y + height) {
                left = std::max(left, getLeftEdge(checkY));
                right = std::min(right, getRightEdge(checkY));
                checkY += 1;  // Check every pixel (could be optimized)
            }
        }
        
        return Rect{left, y, right - left, height};
    }
    
    /**
     * @brief Get left edge at y position (accounting for floats)
     */
    float getLeftEdge(float y) const {
        float edge = 0;
        for (const auto& f : leftFloats_) {
            if (y >= f.rect.y && y < f.rect.y + f.rect.height) {
                edge = std::max(edge, f.rect.x + f.rect.width);
            }
        }
        return edge;
    }
    
    /**
     * @brief Get right edge at y position (accounting for floats)
     */
    float getRightEdge(float y) const {
        float edge = containerWidth_;
        for (const auto& f : rightFloats_) {
            if (y >= f.rect.y && y < f.rect.y + f.rect.height) {
                edge = std::min(edge, f.rect.x);
            }
        }
        return edge;
    }
    
    /**
     * @brief Get block position to clear floats
     * @param side Which side(s) to clear
     * @return Y position after clearing
     */
    float getClearance(FloatSide side) const {
        float clearY = 0;
        
        if (side == FloatSide::Left || side == FloatSide::InlineStart) {
            for (const auto& f : leftFloats_) {
                clearY = std::max(clearY, f.rect.y + f.rect.height);
            }
        }
        
        if (side == FloatSide::Right || side == FloatSide::InlineEnd) {
            for (const auto& f : rightFloats_) {
                clearY = std::max(clearY, f.rect.y + f.rect.height);
            }
        }
        
        return clearY;
    }
    
    /**
     * @brief Clear all floats
     */
    float clearBoth() const {
        float clearY = getClearance(FloatSide::Left);
        clearY = std::max(clearY, getClearance(FloatSide::Right));
        return clearY;
    }
    
    /**
     * @brief Reset manager
     */
    void reset() {
        leftFloats_.clear();
        rightFloats_.clear();
    }
    
    /**
     * @brief Check if there are any floats
     */
    bool hasFloats() const {
        return !leftFloats_.empty() || !rightFloats_.empty();
    }
    
    /**
     * @brief Get lowest float bottom
     */
    float lowestFloat() const {
        float lowest = 0;
        for (const auto& f : leftFloats_) {
            lowest = std::max(lowest, f.rect.y + f.rect.height);
        }
        for (const auto& f : rightFloats_) {
            lowest = std::max(lowest, f.rect.y + f.rect.height);
        }
        return lowest;
    }
    
private:
    float containerWidth_ = 0;
    float containerHeight_ = 0;
    std::vector<FloatInfo> leftFloats_;
    std::vector<FloatInfo> rightFloats_;
    
    float findSpaceForFloat(const Size& size, bool isLeft, float startY) const {
        float y = startY;
        float maxY = lowestFloat();
        
        while (y < maxY) {
            float left = getLeftEdge(y);
            float right = getRightEdge(y);
            
            if (right - left >= size.width) {
                return y;
            }
            
            // Find next float edge
            float nextY = maxY;
            for (const auto& f : leftFloats_) {
                if (f.rect.y + f.rect.height > y && f.rect.y + f.rect.height < nextY) {
                    nextY = f.rect.y + f.rect.height;
                }
            }
            for (const auto& f : rightFloats_) {
                if (f.rect.y + f.rect.height > y && f.rect.y + f.rect.height < nextY) {
                    nextY = f.rect.y + f.rect.height;
                }
            }
            
            y = nextY;
        }
        
        return y;
    }
};

} // namespace NXRender
