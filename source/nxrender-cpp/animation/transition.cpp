// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "animation/transition.h"
#include "widgets/widget.h"
#include <algorithm>
#include <cmath>

namespace NXRender {

TransitionManager& TransitionManager::instance() {
    static TransitionManager mgr;
    return mgr;
}

void TransitionManager::setTransitions(Widget* widget, const std::vector<TransitionConfig>& configs) {
    if (!widget) return;
    auto& wt = widgets_[widget];
    wt.configs.clear();
    for (const auto& config : configs) {
        wt.configs[config.property] = config;
    }
}

void TransitionManager::removeTransitions(Widget* widget) {
    widgets_.erase(widget);
}

void TransitionManager::transitionTo(Widget* widget, const std::string& property,
                                      float fromValue, float toValue,
                                      std::function<void(float)> setter) {
    if (!widget || !setter) return;

    auto it = widgets_.find(widget);
    if (it == widgets_.end()) {
        // No transitions configured — apply immediately
        setter(toValue);
        return;
    }

    auto& wt = it->second;
    auto configIt = wt.configs.find(property);
    if (configIt == wt.configs.end()) {
        // No transition config for this property — apply immediately
        setter(toValue);
        return;
    }

    const auto& config = configIt->second;

    // Check if there's already an active transition for this property
    float actualFrom = fromValue;
    for (auto& active : wt.active) {
        if (active.property == property && !active.complete) {
            // Interrupt: start from current animated value
            actualFrom = active.currentValue;
            active.complete = true; // Mark old one for removal
            break;
        }
    }

    // If from and to are the same, skip
    if (std::abs(actualFrom - toValue) < 0.001f) {
        setter(toValue);
        return;
    }

    ActiveTransition at;
    at.property = property;
    at.fromValue = actualFrom;
    at.toValue = toValue;
    at.currentValue = actualFrom;
    at.elapsedMs = 0;
    at.durationMs = config.durationMs;
    at.delayMs = config.delayMs;
    at.easing = config.easing;
    at.setter = std::move(setter);
    at.complete = false;

    wt.active.push_back(std::move(at));
}

void TransitionManager::cancelAll(Widget* widget) {
    auto it = widgets_.find(widget);
    if (it != widgets_.end()) {
        it->second.active.clear();
    }
}

void TransitionManager::cancel(Widget* widget, const std::string& property) {
    auto it = widgets_.find(widget);
    if (it == widgets_.end()) return;

    auto& active = it->second.active;
    active.erase(
        std::remove_if(active.begin(), active.end(),
                       [&property](const ActiveTransition& t) {
                           return t.property == property;
                       }),
        active.end());
}

bool TransitionManager::update(float deltaMs) {
    bool anyUpdated = false;

    for (auto& [widget, wt] : widgets_) {
        for (auto& at : wt.active) {
            if (at.complete) continue;

            at.elapsedMs += deltaMs;

            float effective = at.elapsedMs - static_cast<float>(at.delayMs);
            if (effective < 0) continue; // Still in delay

            float dur = static_cast<float>(at.durationMs);
            float progress = (dur > 0) ? std::min(effective / dur, 1.0f) : 1.0f;

            float easedProgress = at.easing(progress);
            at.currentValue = at.fromValue + (at.toValue - at.fromValue) * easedProgress;

            if (at.setter) {
                at.setter(at.currentValue);
            }

            anyUpdated = true;

            if (progress >= 1.0f) {
                at.currentValue = at.toValue;
                if (at.setter) at.setter(at.toValue);
                at.complete = true;
            }
        }

        // Remove completed transitions
        wt.active.erase(
            std::remove_if(wt.active.begin(), wt.active.end(),
                           [](const ActiveTransition& t) { return t.complete; }),
            wt.active.end());
    }

    return anyUpdated;
}

bool TransitionManager::hasActiveTransitions(Widget* widget) const {
    auto it = widgets_.find(widget);
    if (it == widgets_.end()) return false;
    return !it->second.active.empty();
}

bool TransitionManager::isTransitioning(Widget* widget, const std::string& property) const {
    auto it = widgets_.find(widget);
    if (it == widgets_.end()) return false;
    for (const auto& at : it->second.active) {
        if (at.property == property && !at.complete) return true;
    }
    return false;
}

float TransitionManager::currentValue(Widget* widget, const std::string& property) const {
    auto it = widgets_.find(widget);
    if (it == widgets_.end()) return 0.0f;
    for (const auto& at : it->second.active) {
        if (at.property == property) return at.currentValue;
    }
    return 0.0f;
}

size_t TransitionManager::activeCount() const {
    size_t total = 0;
    for (const auto& [widget, wt] : widgets_) {
        total += wt.active.size();
    }
    return total;
}

} // namespace NXRender
