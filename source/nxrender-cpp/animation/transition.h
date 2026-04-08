// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file transition.h
 * @brief Implicit property transition system.
 *
 * Watches property changes on widgets and automatically creates
 * smooth animations between old and new values.
 */

#pragma once

#include "easing.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace NXRender {

class Widget;

/**
 * @brief Configuration for a single property transition.
 */
struct TransitionConfig {
    std::string property;         // Property name (e.g. "opacity", "x", "backgroundColor")
    int durationMs = 300;
    int delayMs = 0;
    EasingFunction easing = Easing::easeOut;
};

/**
 * @brief Active transition state for a single property.
 */
struct ActiveTransition {
    std::string property;
    float fromValue;
    float toValue;
    float currentValue;
    float elapsedMs;
    int durationMs;
    int delayMs;
    EasingFunction easing;
    std::function<void(float)> setter;
    bool complete;
};

/**
 * @brief Manages implicit transitions for widgets.
 *
 * Usage:
 *   TransitionManager::instance().setTransitions(widget, {
 *       {"opacity", 300, 0, Easing::easeOut},
 *       {"x", 200, 0, Easing::easeInOut}
 *   });
 *
 * Then when a property changes:
 *   TransitionManager::instance().transitionTo(widget, "opacity", 0.5f, setter);
 *
 * The transition will animate from the current value to the target.
 */
class TransitionManager {
public:
    static TransitionManager& instance();

    /**
     * @brief Configure transitions for a widget.
     */
    void setTransitions(Widget* widget, const std::vector<TransitionConfig>& configs);

    /**
     * @brief Remove all transitions for a widget.
     */
    void removeTransitions(Widget* widget);

    /**
     * @brief Start a transition for a property.
     * If a transition is already running for this property on this widget,
     * the new transition starts from the current animated value.
     */
    void transitionTo(Widget* widget, const std::string& property,
                      float fromValue, float toValue,
                      std::function<void(float)> setter);

    /**
     * @brief Cancel all active transitions for a widget.
     */
    void cancelAll(Widget* widget);

    /**
     * @brief Cancel a specific property transition.
     */
    void cancel(Widget* widget, const std::string& property);

    /**
     * @brief Update all active transitions. Call each frame.
     * @return true if any transition was updated.
     */
    bool update(float deltaMs);

    /**
     * @brief Check if widget has any active transitions.
     */
    bool hasActiveTransitions(Widget* widget) const;

    /**
     * @brief Check if a specific property is transitioning.
     */
    bool isTransitioning(Widget* widget, const std::string& property) const;

    /**
     * @brief Get the current interpolated value of a transitioning property.
     */
    float currentValue(Widget* widget, const std::string& property) const;

    /**
     * @brief Total number of active transitions.
     */
    size_t activeCount() const;

private:
    TransitionManager() = default;
    TransitionManager(const TransitionManager&) = delete;
    TransitionManager& operator=(const TransitionManager&) = delete;

    struct WidgetTransitions {
        std::unordered_map<std::string, TransitionConfig> configs;
        std::vector<ActiveTransition> active;
    };

    std::unordered_map<Widget*, WidgetTransitions> widgets_;
};

} // namespace NXRender
