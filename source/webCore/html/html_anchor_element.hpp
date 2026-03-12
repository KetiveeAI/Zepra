/**
 * @file html_anchor_element.hpp
 * @brief HTMLAnchorElement interface for <a> elements
 *
 * Implements hyperlink functionality per HTML Living Standard.
 *
 * @see https://html.spec.whatwg.org/multipage/text-level-semantics.html#the-a-element
 */

#pragma once

#include "html/html_element.hpp"

namespace Zepra::WebCore {

/**
 * @brief Referrer policy options
 */
enum class ReferrerPolicy {
    NoReferrer,
    NoReferrerWhenDowngrade,
    Origin,
    OriginWhenCrossOrigin,
    SameOrigin,
    StrictOrigin,
    StrictOriginWhenCrossOrigin,
    UnsafeUrl
};

/**
 * @brief HTMLAnchorElement represents an <a> hyperlink element
 *
 * Provides properties and methods for manipulating hyperlinks,
 * including URL components and navigation behavior.
 */
class HTMLAnchorElement : public HTMLElement {
public:
    HTMLAnchorElement();
    ~HTMLAnchorElement() override;

    // =========================================================================
    // Hyperlink Properties
    // =========================================================================

    /// Full URL
    std::string href() const;
    void setHref(const std::string& href);

    /// Target window/frame
    std::string target() const;
    void setTarget(const std::string& target);

    /// Download filename (triggers download instead of navigation)
    std::string download() const;
    void setDownload(const std::string& filename);

    /// Ping URLs (space-separated)
    std::string ping() const;
    void setPing(const std::string& ping);

    /// Link relationship
    std::string rel() const;
    void setRel(const std::string& rel);

    /// Relationship as token list
    DOMTokenList* relList();

    /// Referrer policy
    std::string referrerPolicy() const;
    void setReferrerPolicy(const std::string& policy);

    /// Link type/MIME type hint
    std::string type() const;
    void setType(const std::string& type);

    /// Language of linked resource
    std::string hreflang() const;
    void setHreflang(const std::string& lang);

    /// Link text content
    std::string text() const;
    void setText(const std::string& text);

    // =========================================================================
    // URL Decomposition (like Location interface)
    // =========================================================================

    /// URL origin (read-only)
    std::string origin() const;

    /// Protocol/scheme (with colon)
    std::string protocol() const;
    void setProtocol(const std::string& protocol);

    /// Username
    std::string username() const;
    void setUsername(const std::string& username);

    /// Password
    std::string password() const;
    void setPassword(const std::string& password);

    /// Host (hostname:port)
    std::string host() const;
    void setHost(const std::string& host);

    /// Hostname only
    std::string hostname() const;
    void setHostname(const std::string& hostname);

    /// Port number
    std::string port() const;
    void setPort(const std::string& port);

    /// Path
    std::string pathname() const;
    void setPathname(const std::string& pathname);

    /// Search/query string (with ?)
    std::string search() const;
    void setSearch(const std::string& search);

    /// Fragment/hash (with #)
    std::string hash() const;
    void setHash(const std::string& hash);

    // =========================================================================
    // Methods
    // =========================================================================

    /// Get URL as string
    std::string toString() const;

    /// Clone node
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    /// Rebuild href from URL components
    void rebuildHref();
};

/**
 * @brief DOMTokenList for space-separated attribute lists
 *
 * Manages lists like class and rel attributes.
 */
class DOMTokenList {
public:
    DOMTokenList(HTMLElement* element, const std::string& attributeName);
    ~DOMTokenList();

    /// Number of tokens
    size_t length() const;

    /// Get token at index
    std::string item(size_t index) const;

    /// Check if token exists
    bool contains(const std::string& token) const;

    /// Add token(s)
    void add(const std::string& token);
    void add(const std::vector<std::string>& tokens);

    /// Remove token(s)
    void remove(const std::string& token);
    void remove(const std::vector<std::string>& tokens);

    /// Replace token
    bool replace(const std::string& oldToken, const std::string& newToken);

    /// Toggle token (returns true if token is now present)
    bool toggle(const std::string& token);
    bool toggle(const std::string& token, bool force);

    /// Check if token is supported
    bool supports(const std::string& token) const;

    /// Get/set full value string
    std::string value() const;
    void setValue(const std::string& value);

private:
    HTMLElement* element_;
    std::string attributeName_;
};

} // namespace Zepra::WebCore
