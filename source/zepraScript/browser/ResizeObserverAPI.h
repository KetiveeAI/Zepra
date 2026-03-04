/**
 * @file ResizeObserverAPI.h
 * @brief ResizeObserver API for monitoring element size changes
 * 
 * W3C Resize Observer specification implementation.
 */

#pragma once

#include "../config.hpp"
#include "runtime/objects/value.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace Zepra::Browser {

using Runtime::Value;

// Forward declaration
class DOMWrapper;

// =============================================================================
// ResizeObserverSize
// =============================================================================

/**
 * @brief Size reported in ResizeObserverEntry
 */
struct ResizeObserverSize {
    double inlineSize = 0;   // Width in horizontal writing modes
    double blockSize = 0;    // Height in horizontal writing modes
};

// =============================================================================
// ResizeObserverEntry
// =============================================================================

/**
 * @brief Entry passed to ResizeObserver callback
 */
class ResizeObserverEntry {
public:
    ResizeObserverEntry(DOMWrapper* target,
                        const ResizeObserverSize& contentBoxSize,
                        const ResizeObserverSize& borderBoxSize);
    
    /**
     * @brief Target element being observed
     */
    DOMWrapper* target() const { return target_; }
    
    /**
     * @brief Content box size (content area only)
     */
    const std::vector<ResizeObserverSize>& contentBoxSize() const { return contentBoxSize_; }
    
    /**
     * @brief Border box size (including padding and border)
     */
    const std::vector<ResizeObserverSize>& borderBoxSize() const { return borderBoxSize_; }
    
    /**
     * @brief Device pixel content box size
     */
    const std::vector<ResizeObserverSize>& devicePixelContentBoxSize() const { 
        return devicePixelContentBoxSize_; 
    }
    
    /**
     * @brief Legacy: content rect (DOMRectReadOnly)
     */
    struct ContentRect {
        double x = 0;
        double y = 0;
        double width = 0;
        double height = 0;
    };
    ContentRect contentRect() const;
    
private:
    DOMWrapper* target_;
    std::vector<ResizeObserverSize> contentBoxSize_;
    std::vector<ResizeObserverSize> borderBoxSize_;
    std::vector<ResizeObserverSize> devicePixelContentBoxSize_;
};

// =============================================================================
// ResizeObserverOptions
// =============================================================================

/**
 * @brief Options for ResizeObserver.observe()
 */
struct ResizeObserverOptions {
    enum class BoxType { ContentBox, BorderBox, DevicePixelContentBox };
    
    BoxType box = BoxType::ContentBox;
};

// =============================================================================
// ResizeObserver
// =============================================================================

/**
 * @brief Observes size changes of DOM elements
 */
class ResizeObserver {
public:
    using Callback = std::function<void(const std::vector<ResizeObserverEntry>&, ResizeObserver*)>;
    
    /**
     * @brief Construct with callback
     */
    explicit ResizeObserver(Callback callback);
    ~ResizeObserver();
    
    /**
     * @brief Start observing an element
     */
    void observe(DOMWrapper* target, const ResizeObserverOptions& options = {});
    
    /**
     * @brief Stop observing a specific element
     */
    void unobserve(DOMWrapper* target);
    
    /**
     * @brief Stop observing all elements
     */
    void disconnect();
    
    // Internal: notify of size change
    void notifyResize(DOMWrapper* target, 
                      const ResizeObserverSize& contentBox,
                      const ResizeObserverSize& borderBox);
    
private:
    struct ObservationTarget {
        DOMWrapper* element;
        ResizeObserverOptions options;
        ResizeObserverSize lastContentSize;
        ResizeObserverSize lastBorderSize;
    };
    
    Callback callback_;
    std::vector<ObservationTarget> targets_;
    std::vector<ResizeObserverEntry> pendingEntries_;
};

// =============================================================================
// Builtin Functions
// =============================================================================

void initResizeObserver();

} // namespace Zepra::Browser
