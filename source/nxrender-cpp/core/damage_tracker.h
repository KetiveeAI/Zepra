// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "nxgfx/primitives.h"
#include <vector>

namespace NXRender {

class DamageTracker {
public:
    DamageTracker() = default;

    void addDamage(const Rect& rect);
    void unionDamage(const DamageTracker& other);
    
    void clear();
    
    const std::vector<Rect>& getDamageRects() const { return rects_; }
    Rect getBounds() const;
    bool hasDamage() const { return !rects_.empty(); }

    void optimize();

private:
    std::vector<Rect> rects_;
    
    bool mergeRects(const Rect& r1, const Rect& r2, Rect& out);
};

} // namespace NXRender
