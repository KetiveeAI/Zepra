/**
 * @file html_anchor_element.cpp
 * @brief HTMLAnchorElement implementation
 */

#include "webcore/html/html_anchor_element.hpp"
#include <regex>
#include <sstream>

namespace Zepra::WebCore {

// =============================================================================
// HTMLAnchorElement::Impl
// =============================================================================

class HTMLAnchorElement::Impl {
public:
    std::unique_ptr<DOMTokenList> relList;
    
    // Parsed URL components (cached)
    mutable bool urlParsed = false;
    mutable std::string protocol;
    mutable std::string username;
    mutable std::string password;
    mutable std::string hostname;
    mutable std::string port;
    mutable std::string pathname;
    mutable std::string search;
    mutable std::string hash;
    
    void parseUrl(const std::string& href) const {
        if (urlParsed) return;
        urlParsed = true;
        
        // Simple URL parsing (production would use proper URL parser)
        std::regex urlRegex(
            R"(^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)",
            std::regex::ECMAScript);
        
        std::smatch match;
        if (std::regex_match(href, match, urlRegex)) {
            protocol = match[2].str();
            if (!protocol.empty()) protocol += ":";
            
            std::string authority = match[4].str();
            pathname = match[5].str();
            search = match[6].str();
            hash = match[8].str();
            
            // Parse authority (userinfo@host:port)
            size_t atPos = authority.find('@');
            std::string hostPart = authority;
            if (atPos != std::string::npos) {
                std::string userinfo = authority.substr(0, atPos);
                hostPart = authority.substr(atPos + 1);
                
                size_t colonPos = userinfo.find(':');
                if (colonPos != std::string::npos) {
                    username = userinfo.substr(0, colonPos);
                    password = userinfo.substr(colonPos + 1);
                } else {
                    username = userinfo;
                }
            }
            
            // Parse host:port
            size_t colonPos = hostPart.rfind(':');
            if (colonPos != std::string::npos && hostPart.find(']') == std::string::npos) {
                hostname = hostPart.substr(0, colonPos);
                port = hostPart.substr(colonPos + 1);
            } else {
                hostname = hostPart;
            }
        }
    }
    
    void invalidateUrl() {
        urlParsed = false;
    }
};

// =============================================================================
// HTMLAnchorElement
// =============================================================================

HTMLAnchorElement::HTMLAnchorElement()
    : HTMLElement("a"),
      impl_(std::make_unique<Impl>()) {
    impl_->relList = std::make_unique<DOMTokenList>(this, "rel");
}

HTMLAnchorElement::~HTMLAnchorElement() = default;

std::string HTMLAnchorElement::href() const {
    return getAttribute("href");
}

void HTMLAnchorElement::setHref(const std::string& href) {
    setAttribute("href", href);
    impl_->invalidateUrl();
}

std::string HTMLAnchorElement::target() const {
    return getAttribute("target");
}

void HTMLAnchorElement::setTarget(const std::string& target) {
    setAttribute("target", target);
}

std::string HTMLAnchorElement::download() const {
    return getAttribute("download");
}

void HTMLAnchorElement::setDownload(const std::string& filename) {
    setAttribute("download", filename);
}

std::string HTMLAnchorElement::ping() const {
    return getAttribute("ping");
}

void HTMLAnchorElement::setPing(const std::string& ping) {
    setAttribute("ping", ping);
}

std::string HTMLAnchorElement::rel() const {
    return getAttribute("rel");
}

void HTMLAnchorElement::setRel(const std::string& rel) {
    setAttribute("rel", rel);
}

DOMTokenList* HTMLAnchorElement::relList() {
    return impl_->relList.get();
}

std::string HTMLAnchorElement::referrerPolicy() const {
    return getAttribute("referrerpolicy");
}

void HTMLAnchorElement::setReferrerPolicy(const std::string& policy) {
    setAttribute("referrerpolicy", policy);
}

std::string HTMLAnchorElement::type() const {
    return getAttribute("type");
}

void HTMLAnchorElement::setType(const std::string& type) {
    setAttribute("type", type);
}

std::string HTMLAnchorElement::hreflang() const {
    return getAttribute("hreflang");
}

void HTMLAnchorElement::setHreflang(const std::string& lang) {
    setAttribute("hreflang", lang);
}

std::string HTMLAnchorElement::text() const {
    return innerText();
}

void HTMLAnchorElement::setText(const std::string& text) {
    setInnerText(text);
}

// URL decomposition

std::string HTMLAnchorElement::origin() const {
    impl_->parseUrl(href());
    std::string result = impl_->protocol + "//" + impl_->hostname;
    if (!impl_->port.empty()) {
        result += ":" + impl_->port;
    }
    return result;
}

std::string HTMLAnchorElement::protocol() const {
    impl_->parseUrl(href());
    return impl_->protocol;
}

void HTMLAnchorElement::setProtocol(const std::string& protocol) {
    impl_->parseUrl(href());
    impl_->protocol = protocol;
    rebuildHref();
}

std::string HTMLAnchorElement::username() const {
    impl_->parseUrl(href());
    return impl_->username;
}

void HTMLAnchorElement::setUsername(const std::string& username) {
    impl_->parseUrl(href());
    impl_->username = username;
    rebuildHref();
}

std::string HTMLAnchorElement::password() const {
    impl_->parseUrl(href());
    return impl_->password;
}

void HTMLAnchorElement::setPassword(const std::string& password) {
    impl_->parseUrl(href());
    impl_->password = password;
    rebuildHref();
}

std::string HTMLAnchorElement::host() const {
    impl_->parseUrl(href());
    std::string result = impl_->hostname;
    if (!impl_->port.empty()) {
        result += ":" + impl_->port;
    }
    return result;
}

void HTMLAnchorElement::setHost(const std::string& host) {
    size_t colonPos = host.rfind(':');
    if (colonPos != std::string::npos) {
        impl_->hostname = host.substr(0, colonPos);
        impl_->port = host.substr(colonPos + 1);
    } else {
        impl_->hostname = host;
        impl_->port.clear();
    }
    rebuildHref();
}

std::string HTMLAnchorElement::hostname() const {
    impl_->parseUrl(href());
    return impl_->hostname;
}

void HTMLAnchorElement::setHostname(const std::string& hostname) {
    impl_->parseUrl(href());
    impl_->hostname = hostname;
    rebuildHref();
}

std::string HTMLAnchorElement::port() const {
    impl_->parseUrl(href());
    return impl_->port;
}

void HTMLAnchorElement::setPort(const std::string& port) {
    impl_->parseUrl(href());
    impl_->port = port;
    rebuildHref();
}

std::string HTMLAnchorElement::pathname() const {
    impl_->parseUrl(href());
    return impl_->pathname;
}

void HTMLAnchorElement::setPathname(const std::string& pathname) {
    impl_->parseUrl(href());
    impl_->pathname = pathname;
    rebuildHref();
}

std::string HTMLAnchorElement::search() const {
    impl_->parseUrl(href());
    return impl_->search;
}

void HTMLAnchorElement::setSearch(const std::string& search) {
    impl_->parseUrl(href());
    impl_->search = search.empty() || search[0] == '?' ? search : "?" + search;
    rebuildHref();
}

std::string HTMLAnchorElement::hash() const {
    impl_->parseUrl(href());
    return impl_->hash;
}

void HTMLAnchorElement::setHash(const std::string& hash) {
    impl_->parseUrl(href());
    impl_->hash = hash.empty() || hash[0] == '#' ? hash : "#" + hash;
    rebuildHref();
}

std::string HTMLAnchorElement::toString() const {
    return href();
}

void HTMLAnchorElement::rebuildHref() {
    std::ostringstream oss;
    
    oss << impl_->protocol << "//";
    
    if (!impl_->username.empty()) {
        oss << impl_->username;
        if (!impl_->password.empty()) {
            oss << ":" << impl_->password;
        }
        oss << "@";
    }
    
    oss << impl_->hostname;
    
    if (!impl_->port.empty()) {
        oss << ":" << impl_->port;
    }
    
    oss << impl_->pathname;
    oss << impl_->search;
    oss << impl_->hash;
    
    setAttribute("href", oss.str());
    impl_->urlParsed = true;
}

std::unique_ptr<DOMNode> HTMLAnchorElement::cloneNode(bool deep) const {
    auto clone = std::make_unique<HTMLAnchorElement>();
    copyHTMLElementProperties(clone.get());
    
    if (deep) {
        for (const auto& child : childNodes()) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    
    return clone;
}

// =============================================================================
// DOMTokenList Implementation
// =============================================================================

DOMTokenList::DOMTokenList(HTMLElement* element, const std::string& attributeName)
    : element_(element), attributeName_(attributeName) {}

DOMTokenList::~DOMTokenList() = default;

std::vector<std::string> tokenize(const std::string& value) {
    std::vector<std::string> tokens;
    std::istringstream iss(value);
    std::string token;
    while (iss >> token) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

size_t DOMTokenList::length() const {
    return tokenize(element_->getAttribute(attributeName_)).size();
}

std::string DOMTokenList::item(size_t index) const {
    auto tokens = tokenize(element_->getAttribute(attributeName_));
    return index < tokens.size() ? tokens[index] : "";
}

bool DOMTokenList::contains(const std::string& token) const {
    auto tokens = tokenize(element_->getAttribute(attributeName_));
    return std::find(tokens.begin(), tokens.end(), token) != tokens.end();
}

void DOMTokenList::add(const std::string& token) {
    if (!contains(token)) {
        std::string value = element_->getAttribute(attributeName_);
        if (!value.empty()) value += " ";
        value += token;
        element_->setAttribute(attributeName_, value);
    }
}

void DOMTokenList::add(const std::vector<std::string>& tokens) {
    for (const auto& token : tokens) {
        add(token);
    }
}

void DOMTokenList::remove(const std::string& token) {
    auto tokens = tokenize(element_->getAttribute(attributeName_));
    tokens.erase(std::remove(tokens.begin(), tokens.end(), token), tokens.end());
    
    std::ostringstream oss;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) oss << " ";
        oss << tokens[i];
    }
    element_->setAttribute(attributeName_, oss.str());
}

void DOMTokenList::remove(const std::vector<std::string>& tokens) {
    for (const auto& token : tokens) {
        remove(token);
    }
}

bool DOMTokenList::replace(const std::string& oldToken, const std::string& newToken) {
    if (!contains(oldToken)) return false;
    remove(oldToken);
    add(newToken);
    return true;
}

bool DOMTokenList::toggle(const std::string& token) {
    if (contains(token)) {
        remove(token);
        return false;
    } else {
        add(token);
        return true;
    }
}

bool DOMTokenList::toggle(const std::string& token, bool force) {
    if (force) {
        add(token);
        return true;
    } else {
        remove(token);
        return false;
    }
}

bool DOMTokenList::supports(const std::string& token) const {
    (void)token;
    return true;  // All tokens are supported
}

std::string DOMTokenList::value() const {
    return element_->getAttribute(attributeName_);
}

void DOMTokenList::setValue(const std::string& value) {
    element_->setAttribute(attributeName_, value);
}

} // namespace Zepra::WebCore
