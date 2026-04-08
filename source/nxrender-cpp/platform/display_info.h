// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file display_info.h
 * @brief Single source of truth for viewport dimensions, DPI, and device pixel ratio.
 *
 * Every subsystem that needs viewport dimensions pulls from DisplayInfo.
 * No magic numbers anywhere in the codebase.
 * Updated by the windowing system on every resize event.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include <mutex>

namespace NXRender {

/**
 * @brief Display/viewport information queried from the OS/compositor.
 *
 * windowWidth/windowHeight: the browser window's client area in logical pixels.
 * screenWidth/screenHeight: the physical monitor resolution (for reference only).
 * dpi: dots per inch reported by the OS.
 * devicePixelRatio: 1.0 on standard displays, 2.0 on HiDPI/Retina.
 */
struct DisplayMetrics {
    int windowWidth = 0;
    int windowHeight = 0;
    int screenWidth = 0;
    int screenHeight = 0;
    float dpi = 96.0f;
    float devicePixelRatio = 1.0f;

    int physicalWidth() const {
        return static_cast<int>(windowWidth * devicePixelRatio);
    }

    int physicalHeight() const {
        return static_cast<int>(windowHeight * devicePixelRatio);
    }

    float aspectRatio() const {
        if (windowHeight <= 0) return 1.0f;
        return static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    }

    bool isHiDPI() const { return devicePixelRatio > 1.4f; }
    bool isValid() const { return windowWidth > 0 && windowHeight > 0; }
};

/**
 * @brief Callback signature for display change notifications.
 */
using DisplayChangeCallback = std::function<void(const DisplayMetrics&)>;

/**
 * @brief Central display info manager.
 *
 * Singleton — all subsystems register as observers and get notified on
 * resize or DPI change. Thread-safe.
 */
class DisplayInfo {
public:
    static DisplayInfo& instance();

    /**
     * @brief Query initial display info from the OS/compositor.
     * Called once at startup by the platform layer.
     */
    void queryFromOS();

    /**
     * @brief Update window dimensions. Called by the windowing system on resize.
     */
    void setWindowSize(int width, int height);

    /**
     * @brief Update screen resolution. Called on monitor change.
     */
    void setScreenSize(int width, int height);

    /**
     * @brief Update DPI. Called on DPI change (e.g. moving window between monitors).
     */
    void setDPI(float dpi);

    /**
     * @brief Update device pixel ratio explicitly.
     */
    void setDevicePixelRatio(float ratio);

    /**
     * @brief Set all metrics at once. Fires a single notification.
     */
    void setMetrics(const DisplayMetrics& metrics);

    /**
     * @brief Get current display metrics. Lock-free read.
     */
    const DisplayMetrics& metrics() const { return metrics_; }

    // Convenience accessors
    int windowWidth() const { return metrics_.windowWidth; }
    int windowHeight() const { return metrics_.windowHeight; }
    int screenWidth() const { return metrics_.screenWidth; }
    int screenHeight() const { return metrics_.screenHeight; }
    float dpi() const { return metrics_.dpi; }
    float devicePixelRatio() const { return metrics_.devicePixelRatio; }
    int physicalWidth() const { return metrics_.physicalWidth(); }
    int physicalHeight() const { return metrics_.physicalHeight(); }

    /**
     * @brief Register a callback for display changes.
     * @return Observer ID for unregistration.
     */
    uint32_t addObserver(DisplayChangeCallback callback);

    /**
     * @brief Remove a previously registered observer.
     */
    void removeObserver(uint32_t id);

private:
    DisplayInfo() = default;
    DisplayInfo(const DisplayInfo&) = delete;
    DisplayInfo& operator=(const DisplayInfo&) = delete;

    void notifyObservers();

    DisplayMetrics metrics_;
    std::mutex observerMutex_;
    uint32_t nextObserverId_ = 1;

    struct Observer {
        uint32_t id;
        DisplayChangeCallback callback;
    };
    std::vector<Observer> observers_;
};

} // namespace NXRender
