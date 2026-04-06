// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "damage_tracker.h"
#include <algorithm>

namespace NXRender {

void DamageTracker::addDamage(const Rect& rect) {
    if (rect.width <= 0 || rect.height <= 0) return;

    for (size_t i = 0; i < rects_.size(); ++i) {
        Rect merged;
        if (mergeRects(rects_[i], rect, merged)) {
            rects_[i] = merged;
            return;
        }
    }
    
    rects_.push_back(rect);
}

void DamageTracker::unionDamage(const DamageTracker& other) {
    for(const auto& r : other.rects_) {
        addDamage(r);
    }
}

void DamageTracker::clear() {
    rects_.clear();
}

Rect DamageTracker::getBounds() const {
    if (rects_.empty()) return Rect(0,0,0,0);
    
    float minX = rects_[0].x;
    float minY = rects_[0].y;
    float maxX = rects_[0].x + rects_[0].width;
    float maxY = rects_[0].y + rects_[0].height;

    for (size_t i = 1; i < rects_.size(); ++i) {
        minX = std::min(minX, rects_[i].x);
        minY = std::min(minY, rects_[i].y);
        maxX = std::max(maxX, rects_[i].x + rects_[i].width);
        maxY = std::max(maxY, rects_[i].y + rects_[i].height);
    }

    return Rect(minX, minY, maxX - minX, maxY - minY);
}

bool DamageTracker::mergeRects(const Rect& r1, const Rect& r2, Rect& out) {
    // If rects intersect, we can merge them into a bounding box
    if (r1.intersects(r2)) {
        float minX = std::min(r1.x, r2.x);
        float minY = std::min(r1.y, r2.y);
        float maxX = std::max(r1.x + r1.width, r2.x + r2.width);
        float maxY = std::max(r1.y + r1.height, r2.y + r2.height);
        out = Rect(minX, minY, maxX - minX, maxY - minY);
        return true;
    }
    return false;
}

void DamageTracker::optimize() {
    if (rects_.size() < 2) return;

    bool merged;
    do {
        merged = false;
        for (size_t i = 0; i < rects_.size(); ++i) {
            for (size_t j = i + 1; j < rects_.size(); ++j) {
                Rect out;
                if (mergeRects(rects_[i], rects_[j], out)) {
                    rects_[i] = out;
                    rects_.erase(rects_.begin() + j);
                    merged = true;
                    break;
                }
            }
            if (merged) break; // Start over since vector modified
        }
    } while (merged);
}

} // namespace NXRender
