/**
 * @file window.hpp
 * @brief Window interface - the global object
 *
 * Represents the browser window containing the document.
 *
 * @see https://developer.mozilla.org/en-US/docs/Web/API/Window
 */

#pragma once

#include "event_target.hpp"
#include "browser/location.hpp"
#include "browser/navigator.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

namespace Zepra::WebCore {

// Forward declarations
class DOMDocument;
class HTMLElement;
class CSSStyleDeclaration;
class Storage;
class Console;
class Performance;
class Screen;

/**
 * @brief Timer callback type
 */
using TimerCallback = std::function<void()>;

/**
 * @brief Animation frame callback type
 */
using AnimationFrameCallback = std::function<void(double timestamp)>;

/**
 * @brief Screen information
 */
class Screen {
public:
    Screen();
    ~Screen();

    /// Screen width in pixels
    unsigned int width() const;

    /// Screen height in pixels
    unsigned int height() const;

    /// Available width (excluding taskbar etc)
    unsigned int availWidth() const;

    /// Available height
    unsigned int availHeight() const;

    /// Color depth
    unsigned int colorDepth() const;

    /// Pixel depth
    unsigned int pixelDepth() const;

    /// Screen orientation
    std::string orientation() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Console interface for logging
 */
class Console {
public:
    Console();
    ~Console();

    void log(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void debug(const std::string& message);
    void trace(const std::string& message);

    void group(const std::string& label = "");
    void groupCollapsed(const std::string& label = "");
    void groupEnd();

    void time(const std::string& label = "default");
    void timeLog(const std::string& label = "default");
    void timeEnd(const std::string& label = "default");

    void count(const std::string& label = "default");
    void countReset(const std::string& label = "default");

    void clear();
    void table(const std::string& data);
    void assert_(bool condition, const std::string& message = "");

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Performance interface
 */
class Performance {
public:
    Performance();
    ~Performance();

    /// High-resolution timestamp
    double now() const;

    /// Time origin
    double timeOrigin() const;

    /// Create performance mark
    void mark(const std::string& name);

    /// Create performance measure
    void measure(const std::string& name, 
                 const std::string& startMark = "",
                 const std::string& endMark = "");

    /// Clear marks
    void clearMarks(const std::string& name = "");

    /// Clear measures
    void clearMeasures(const std::string& name = "");

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Local/Session Storage
 */
class Storage {
public:
    Storage();
    ~Storage();

    /// Number of items
    size_t length() const;

    /// Get key at index
    std::string key(size_t index) const;

    /// Get item value
    std::string getItem(const std::string& key) const;

    /// Set item value
    void setItem(const std::string& key, const std::string& value);

    /// Remove item
    void removeItem(const std::string& key);

    /// Clear all items
    void clear();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Window - the global browser object
 *
 * Represents the browser window and provides access to
 * document, location, navigator, and various browser APIs.
 */
class Window : public EventTarget {
public:
    Window();
    ~Window() override;

    // =========================================================================
    // Core Properties
    // =========================================================================

    /// The document in this window
    DOMDocument* document();
    const DOMDocument* document() const;
    void setDocument(DOMDocument* doc);

    /// Current location
    Location* location();
    const Location* location() const;

    /// Navigation history
    History* history();
    const History* history() const;

    /// Browser information
    Navigator* navigator();
    const Navigator* navigator() const;

    /// Screen information
    Screen* screen();

    /// Console
    Console* console();

    /// Performance
    Performance* performance();

    /// Local storage
    Storage* localStorage();

    /// Session storage
    Storage* sessionStorage();

    // =========================================================================
    // Window Properties
    // =========================================================================

    /// Window name
    std::string name() const;
    void setName(const std::string& name);

    /// Window status text
    std::string status() const;
    void setStatus(const std::string& status);

    /// Whether window is closed
    bool closed() const;

    /// Device pixel ratio
    double devicePixelRatio() const;

    /// Inner width (viewport)
    int innerWidth() const;

    /// Inner height (viewport)
    int innerHeight() const;

    /// Outer width (including chrome)
    int outerWidth() const;

    /// Outer height (including chrome)
    int outerHeight() const;

    /// Page X offset (scroll position)
    double pageXOffset() const;

    /// Page Y offset (scroll position)
    double pageYOffset() const;

    /// Alias for pageXOffset
    double scrollX() const;

    /// Alias for pageYOffset
    double scrollY() const;

    /// Screen X position
    int screenX() const;

    /// Screen Y position
    int screenY() const;

    /// Whether window is in secure context (HTTPS)
    bool isSecureContext() const;

    /// Origin
    std::string origin() const;

    // =========================================================================
    // Frame Properties
    // =========================================================================

    /// Self reference
    Window* self();

    /// Window reference
    Window* window();

    /// Top-level window
    Window* top();

    /// Parent window
    Window* parent();

    /// Opener window
    Window* opener();

    /// Frame element
    HTMLElement* frameElement();

    /// Number of frames
    size_t length() const;

    /// Get frame by index
    Window* frame(size_t index);

    // =========================================================================
    // Timers
    // =========================================================================

    /// Set timeout
    int setTimeout(TimerCallback callback, int delay);

    /// Clear timeout
    void clearTimeout(int id);

    /// Set interval
    int setInterval(TimerCallback callback, int delay);

    /// Clear interval
    void clearInterval(int id);

    /// Request animation frame
    int requestAnimationFrame(AnimationFrameCallback callback);

    /// Cancel animation frame
    void cancelAnimationFrame(int id);

    /// Request idle callback
    int requestIdleCallback(TimerCallback callback, int timeout = -1);

    /// Cancel idle callback
    void cancelIdleCallback(int id);

    // =========================================================================
    // Dialog Methods
    // =========================================================================

    /// Show alert dialog
    void alert(const std::string& message = "");

    /// Show confirm dialog
    bool confirm(const std::string& message = "");

    /// Show prompt dialog
    std::string prompt(const std::string& message = "", 
                       const std::string& defaultValue = "");

    /// Print page
    void print();

    // =========================================================================
    // Window Control
    // =========================================================================

    /// Close window
    void close();

    /// Stop loading
    void stop();

    /// Focus window
    void focus();

    /// Blur window
    void blur();

    /// Open new window
    Window* open(const std::string& url = "", 
                 const std::string& target = "_blank",
                 const std::string& features = "");

    /// Move window
    void moveTo(int x, int y);
    void moveBy(int dx, int dy);

    /// Resize window
    void resizeTo(int width, int height);
    void resizeBy(int dw, int dh);

    /// Scroll
    void scroll(double x, double y);
    void scrollTo(double x, double y);
    void scrollBy(double dx, double dy);

    // =========================================================================
    // Style/Layout
    // =========================================================================

    /// Get computed style for element
    CSSStyleDeclaration* getComputedStyle(HTMLElement* element, 
                                           const std::string& pseudoElt = "");

    /// Match media query
    bool matchMedia(const std::string& query);

    // =========================================================================
    // Encoding
    // =========================================================================

    /// Base64 decode
    std::string atob(const std::string& encoded);

    /// Base64 encode
    std::string btoa(const std::string& data);

    // =========================================================================
    // Event Handlers
    // =========================================================================

    void setOnLoad(EventListener callback);
    void setOnUnload(EventListener callback);
    void setOnBeforeUnload(EventListener callback);
    void setOnError(EventListener callback);
    void setOnResize(EventListener callback);
    void setOnScroll(EventListener callback);
    void setOnFocus(EventListener callback);
    void setOnBlur(EventListener callback);
    void setOnOnline(EventListener callback);
    void setOnOffline(EventListener callback);
    void setOnHashChange(EventListener callback);
    void setOnPopState(EventListener callback);
    void setOnStorage(EventListener callback);
    void setOnMessage(EventListener callback);

    // =========================================================================
    // Internal
    // =========================================================================

    /// Process pending timers
    void processTimers();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
