/**
 * @file html_history.hpp
 * @brief History API interfaces
 */

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <any>

namespace Zepra::WebCore {

/**
 * @brief Scroll restoration mode
 */
enum class ScrollRestoration {
    Auto,
    Manual
};

/**
 * @brief History entry
 */
struct HistoryEntry {
    std::string url;
    std::string title;
    std::any state;
};

/**
 * @brief History API
 */
class History {
public:
    History();
    ~History();
    
    // Properties
    int length() const { return static_cast<int>(entries_.size()); }
    
    ScrollRestoration scrollRestoration() const { return scrollRestoration_; }
    void setScrollRestoration(ScrollRestoration sr) { scrollRestoration_ = sr; }
    
    std::any state() const;
    
    // Methods
    void go(int delta = 0);
    void back() { go(-1); }
    void forward() { go(1); }
    
    void pushState(const std::any& state, const std::string& title, 
                   const std::string& url = "");
    void replaceState(const std::any& state, const std::string& title,
                      const std::string& url = "");
    
    // Events
    std::function<void(const std::any& state)> onPopState;
    
private:
    std::vector<HistoryEntry> entries_;
    int currentIndex_ = -1;
    ScrollRestoration scrollRestoration_ = ScrollRestoration::Auto;
};

/**
 * @brief Location API
 */
class Location {
public:
    Location() = default;
    
    // Properties
    std::string href() const { return href_; }
    void setHref(const std::string& h);
    
    std::string protocol() const;
    void setProtocol(const std::string& p);
    
    std::string host() const;
    void setHost(const std::string& h);
    
    std::string hostname() const;
    void setHostname(const std::string& h);
    
    std::string port() const;
    void setPort(const std::string& p);
    
    std::string pathname() const;
    void setPathname(const std::string& p);
    
    std::string search() const;
    void setSearch(const std::string& s);
    
    std::string hash() const;
    void setHash(const std::string& h);
    
    std::string origin() const;
    
    // Methods
    void assign(const std::string& url);
    void replace(const std::string& url);
    void reload(bool forceReload = false);
    
    std::string toString() const { return href_; }
    
    // Event callback
    std::function<void(const std::string&)> onNavigate;
    
private:
    std::string href_;
    
    void parseHref();
    void updateHref();
};

/**
 * @brief PopState event
 */
struct PopStateEvent {
    std::any state;
};

/**
 * @brief HashChange event
 */
struct HashChangeEvent {
    std::string oldURL;
    std::string newURL;
};

} // namespace Zepra::WebCore
