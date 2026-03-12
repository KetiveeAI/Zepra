// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file layout.h
 * @brief Base layout types
 */

#pragma once

#include "../nxgfx/primitives.h"

namespace NXRender {

class Widget;

/**
 * @brief Layout constraints
 */
struct Constraints {
    float minWidth = 0;
    float maxWidth = 1e6f;  // Large number = unconstrained
    float minHeight = 0;
    float maxHeight = 1e6f;
    
    static Constraints tight(const Size& size) {
        return {size.width, size.width, size.height, size.height};
    }
    
    static Constraints loose(const Size& size) {
        return {0, size.width, 0, size.height};
    }
    
    static Constraints expand() {
        return {0, 1e6f, 0, 1e6f};
    }
    
    Size constrain(const Size& size) const {
        return Size(
            std::max(minWidth, std::min(maxWidth, size.width)),
            std::max(minHeight, std::min(maxHeight, size.height))
        );
    }
    
    bool isTight() const {
        return minWidth == maxWidth && minHeight == maxHeight;
    }
};

} // namespace NXRender
