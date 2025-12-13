/**
 * @file location.cpp
 * @brief Location and History implementation
 */

#include "webcore/browser/location.hpp"
#include <regex>
#include <sstream>

namespace Zepra::WebCore {

// =============================================================================
// Location::Impl
// =============================================================================

class Location::Impl {
public:
    std::string href;
    std::string protocol = "https:";
    std::string hostname;
    std::string port;
    std::string pathname = "/";
    std::string search;
    std::string hash;
    std::vector<std::string> ancestorOrigins;
    
    void parseHref(const std::string& url) {
        // Simple URL parsing
        std::regex urlRegex(
            R"(^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)",
            std::regex::ECMAScript);
        
        std::smatch match;
        if (std::regex_match(url, match, urlRegex)) {
            protocol = match[2].str();
            if (!protocol.empty()) protocol += ":";
            
            std::string authority = match[4].str();
            pathname = match[5].str();
            if (pathname.empty()) pathname = "/";
            search = match[6].str();
            hash = match[8].str();
            
            // Parse host:port
            size_t colonPos = authority.rfind(':');
            if (colonPos != std::string::npos && authority.find(']') < colonPos) {
                hostname = authority.substr(0, colonPos);
                port = authority.substr(colonPos + 1);
            } else {
                hostname = authority;
                port.clear();
            }
        }
        
        rebuildHref();
    }
    
    void rebuildHref() {
        std::ostringstream oss;
        oss << protocol << "//" << hostname;
        if (!port.empty()) oss << ":" << port;
        oss << pathname << search << hash;
        href = oss.str();
    }
};

// =============================================================================
// Location
// =============================================================================

Location::Location(Window* window)
    : impl_(std::make_unique<Impl>()), window_(window) {}

Location::~Location() = default;

std::string Location::href() const {
    return impl_->href;
}

void Location::setHref(const std::string& href) {
    impl_->parseHref(href);
    // Navigation would be triggered here
}

std::string Location::origin() const {
    std::string result = impl_->protocol + "//" + impl_->hostname;
    if (!impl_->port.empty()) {
        result += ":" + impl_->port;
    }
    return result;
}

std::string Location::protocol() const {
    return impl_->protocol;
}

void Location::setProtocol(const std::string& protocol) {
    impl_->protocol = protocol;
    if (!impl_->protocol.empty() && impl_->protocol.back() != ':') {
        impl_->protocol += ':';
    }
    impl_->rebuildHref();
}

std::string Location::host() const {
    std::string result = impl_->hostname;
    if (!impl_->port.empty()) {
        result += ":" + impl_->port;
    }
    return result;
}

void Location::setHost(const std::string& host) {
    size_t colonPos = host.rfind(':');
    if (colonPos != std::string::npos) {
        impl_->hostname = host.substr(0, colonPos);
        impl_->port = host.substr(colonPos + 1);
    } else {
        impl_->hostname = host;
        impl_->port.clear();
    }
    impl_->rebuildHref();
}

std::string Location::hostname() const {
    return impl_->hostname;
}

void Location::setHostname(const std::string& hostname) {
    impl_->hostname = hostname;
    impl_->rebuildHref();
}

std::string Location::port() const {
    return impl_->port;
}

void Location::setPort(const std::string& port) {
    impl_->port = port;
    impl_->rebuildHref();
}

std::string Location::pathname() const {
    return impl_->pathname;
}

void Location::setPathname(const std::string& pathname) {
    impl_->pathname = pathname.empty() || pathname[0] != '/' ? "/" + pathname : pathname;
    impl_->rebuildHref();
}

std::string Location::search() const {
    return impl_->search;
}

void Location::setSearch(const std::string& search) {
    impl_->search = search.empty() || search[0] == '?' ? search : "?" + search;
    impl_->rebuildHref();
}

std::string Location::hash() const {
    return impl_->hash;
}

void Location::setHash(const std::string& hash) {
    impl_->hash = hash.empty() || hash[0] == '#' ? hash : "#" + hash;
    impl_->rebuildHref();
}

std::vector<std::string> Location::ancestorOrigins() const {
    return impl_->ancestorOrigins;
}

void Location::assign(const std::string& url) {
    setHref(url);
}

void Location::reload() {
    // Would trigger page reload
}

void Location::replace(const std::string& url) {
    impl_->parseHref(url);
    // Navigate without adding to history
}

std::string Location::toString() const {
    return impl_->href;
}

// =============================================================================
// History::Impl
// =============================================================================

class History::Impl {
public:
    std::vector<std::pair<std::string, std::string>> entries;  // state, url
    int currentIndex = -1;
    std::string scrollRestoration = "auto";
};

// =============================================================================
// History
// =============================================================================

History::History(Window* window)
    : impl_(std::make_unique<Impl>()), window_(window) {}

History::~History() = default;

int History::length() const {
    return static_cast<int>(impl_->entries.size());
}

std::string History::scrollRestoration() const {
    return impl_->scrollRestoration;
}

void History::setScrollRestoration(const std::string& mode) {
    if (mode == "auto" || mode == "manual") {
        impl_->scrollRestoration = mode;
    }
}

std::string History::state() const {
    if (impl_->currentIndex >= 0 && impl_->currentIndex < static_cast<int>(impl_->entries.size())) {
        return impl_->entries[impl_->currentIndex].first;
    }
    return "";
}

void History::back() {
    go(-1);
}

void History::forward() {
    go(1);
}

void History::go(int delta) {
    int newIndex = impl_->currentIndex + delta;
    if (newIndex >= 0 && newIndex < static_cast<int>(impl_->entries.size())) {
        impl_->currentIndex = newIndex;
        // Would navigate and fire popstate event
    }
}

void History::pushState(const std::string& state, const std::string& /*title*/, const std::string& url) {
    // Remove any forward entries
    if (impl_->currentIndex < static_cast<int>(impl_->entries.size()) - 1) {
        impl_->entries.erase(impl_->entries.begin() + impl_->currentIndex + 1, impl_->entries.end());
    }
    
    impl_->entries.push_back({state, url});
    impl_->currentIndex = static_cast<int>(impl_->entries.size()) - 1;
}

void History::replaceState(const std::string& state, const std::string& /*title*/, const std::string& url) {
    if (impl_->currentIndex >= 0 && impl_->currentIndex < static_cast<int>(impl_->entries.size())) {
        impl_->entries[impl_->currentIndex] = {state, url};
    } else {
        pushState(state, "", url);
    }
}

} // namespace Zepra::WebCore
