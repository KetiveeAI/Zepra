/**
 * @file location.hpp
 * @brief Location interface for URL manipulation
 *
 * Represents the location (URL) of the document.
 *
 * @see https://developer.mozilla.org/en-US/docs/Web/API/Location
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace Zepra::WebCore {

// Forward declarations
class Window;

/**
 * @brief Location represents the URL of the current document
 *
 * Provides properties and methods for URL manipulation and navigation.
 */
class Location {
public:
    explicit Location(Window* window);
    ~Location();

    // =========================================================================
    // URL Properties
    // =========================================================================

    /// Full URL
    std::string href() const;
    void setHref(const std::string& href);

    /// Origin (protocol + host)
    std::string origin() const;

    /// Protocol scheme (with colon)
    std::string protocol() const;
    void setProtocol(const std::string& protocol);

    /// Host (hostname:port)
    std::string host() const;
    void setHost(const std::string& host);

    /// Hostname only
    std::string hostname() const;
    void setHostname(const std::string& hostname);

    /// Port number
    std::string port() const;
    void setPort(const std::string& port);

    /// Path (after host, before query)
    std::string pathname() const;
    void setPathname(const std::string& pathname);

    /// Query string (with ?)
    std::string search() const;
    void setSearch(const std::string& search);

    /// Hash/Fragment (with #)
    std::string hash() const;
    void setHash(const std::string& hash);

    /// Ancestor origins
    std::vector<std::string> ancestorOrigins() const;

    // =========================================================================
    // Navigation Methods
    // =========================================================================

    /// Navigate to URL (adds to history)
    void assign(const std::string& url);

    /// Reload current page
    void reload();

    /// Navigate to URL (replaces current history entry)
    void replace(const std::string& url);

    /// Get URL as string
    std::string toString() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    Window* window_;
};

/**
 * @brief History interface for browser history manipulation
 *
 * @see https://developer.mozilla.org/en-US/docs/Web/API/History
 */
class History {
public:
    explicit History(Window* window);
    ~History();

    // =========================================================================
    // Properties
    // =========================================================================

    /// Number of entries in history
    int length() const;

    /// Scroll restoration mode ("auto" or "manual")
    std::string scrollRestoration() const;
    void setScrollRestoration(const std::string& mode);

    /// Current state object
    std::string state() const;

    // =========================================================================
    // Navigation Methods
    // =========================================================================

    /// Go back one page
    void back();

    /// Go forward one page
    void forward();

    /// Go to specific page in history
    void go(int delta = 0);

    /// Push new state onto history
    void pushState(const std::string& state, const std::string& title, 
                   const std::string& url = "");

    /// Replace current state
    void replaceState(const std::string& state, const std::string& title,
                      const std::string& url = "");

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    Window* window_;
};

} // namespace Zepra::WebCore
