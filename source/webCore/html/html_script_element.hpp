/**
 * @file html_script_element.hpp
 * @brief HTML Script Element
 * 
 * Other elements (link, meta, style, title, base, head, body, html)
 * have been moved to their dedicated header files.
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief HTML Script Element (<script>)
 */
class HTMLScriptElement : public HTMLElement {
public:
    using LoadCallback = std::function<void()>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    
    HTMLScriptElement();
    ~HTMLScriptElement() override;
    
    // Source
    std::string src() const;
    void setSrc(const std::string& url);
    
    // Type
    std::string type() const;
    void setType(const std::string& t);
    
    // Async/defer
    bool async() const;
    void setAsync(bool a);
    
    bool defer() const;
    void setDefer(bool d);
    
    // Module
    bool noModule() const;
    void setNoModule(bool n);
    
    // Integrity
    std::string integrity() const;
    void setIntegrity(const std::string& i);
    
    // CORS
    std::string crossOrigin() const;
    void setCrossOrigin(const std::string& c);
    
    // Referrer policy
    std::string referrerPolicy() const;
    void setReferrerPolicy(const std::string& p);
    
    // Script content (inline script)
    std::string text() const;
    void setText(const std::string& t);
    
    // Fetch options
    std::string fetchPriority() const;
    void setFetchPriority(const std::string& p);
    
    // Blocking
    std::string blocking() const;
    void setBlocking(const std::string& b);
    
    // Callbacks
    void onLoad(LoadCallback cb);
    void onError(ErrorCallback cb);
    
    // Script state
    bool isLoading() const;
    bool isLoaded() const;
    bool hasError() const;
    std::string errorMessage() const;
    
    // Script type helpers
    bool isModule() const;
    bool isClassic() const;
    bool isImportMap() const;
    bool isExternal() const;
    bool isInline() const;
    
    // DOM
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTML Html Element (<html>)
 */
class HTMLHtmlElement : public HTMLElement {
public:
    HTMLHtmlElement();
    ~HTMLHtmlElement() override;
    
    // Version
    std::string version() const;
    void setVersion(const std::string& v);
    
    // DOM
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief HTML Noscript Element (<noscript>)
 */
class HTMLNoScriptElement : public HTMLElement {
public:
    HTMLNoScriptElement();
    ~HTMLNoScriptElement() override;
    
    std::unique_ptr<DOMNode> cloneNode(bool deep) const override;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
