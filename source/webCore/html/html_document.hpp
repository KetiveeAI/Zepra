/**
 * @file html_document.hpp
 * @brief HTML Document interface
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace Zepra::WebCore {

class HTMLHeadElement;
class HTMLBodyElement;
class HTMLHtmlElement;

/**
 * @brief Document ready state
 */
enum class DocumentReadyState {
    Loading,
    Interactive,
    Complete
};

/**
 * @brief HTML Document
 */
class HTMLDocument {
public:
    HTMLDocument();
    ~HTMLDocument();
    
    // Document info
    std::string URL() const { return url_; }
    void setURL(const std::string& url) { url_ = url; }
    
    std::string baseURI() const { return baseURI_; }
    void setBaseURI(const std::string& uri) { baseURI_ = uri; }
    
    std::string title() const;
    void setTitle(const std::string& t);
    
    std::string characterSet() const { return charset_; }
    
    std::string contentType() const { return contentType_; }
    
    std::string referrer() const { return referrer_; }
    
    std::string domain() const;
    
    std::string cookie() const;
    void setCookie(const std::string& c);
    
    std::string lastModified() const { return lastModified_; }
    
    DocumentReadyState readyState() const { return readyState_; }
    
    // Document structure
    HTMLHtmlElement* documentElement() const { return documentElement_.get(); }
    HTMLHeadElement* head() const;
    HTMLBodyElement* body() const;
    void setBody(HTMLBodyElement* body);
    
    // Element access
    HTMLElement* getElementById(const std::string& id) const;
    std::vector<HTMLElement*> getElementsByTagName(const std::string& tag) const;
    std::vector<HTMLElement*> getElementsByClassName(const std::string& cls) const;
    std::vector<HTMLElement*> getElementsByName(const std::string& name) const;
    
    HTMLElement* querySelector(const std::string& selector) const;
    std::vector<HTMLElement*> querySelectorAll(const std::string& selector) const;
    
    // Element creation
    HTMLElement* createElement(const std::string& tagName) const;
    HTMLElement* createElementNS(const std::string& ns, const std::string& tagName) const;
    
    // Text nodes
    HTMLElement* createTextNode(const std::string& text) const;
    HTMLElement* createComment(const std::string& comment) const;
    HTMLElement* createDocumentFragment() const;
    
    // Document collections
    std::vector<HTMLElement*> images() const;
    std::vector<HTMLElement*> links() const;
    std::vector<HTMLElement*> forms() const;
    std::vector<HTMLElement*> scripts() const;
    std::vector<HTMLElement*> anchors() const;
    std::vector<HTMLElement*> embeds() const;
    std::vector<HTMLElement*> plugins() const;
    
    // Active element
    HTMLElement* activeElement() const { return activeElement_; }
    void setActiveElement(HTMLElement* el) { activeElement_ = el; }
    
    // Focus
    bool hasFocus() const { return hasFocus_; }
    
    // Document state
    std::string designMode() const { return designMode_; }
    void setDesignMode(const std::string& m) { designMode_ = m; }
    
    bool hidden() const { return hidden_; }
    std::string visibilityState() const { return hidden_ ? "hidden" : "visible"; }
    
    // Fullscreen
    HTMLElement* fullscreenElement() const { return fullscreenElement_; }
    bool fullscreenEnabled() const { return fullscreenEnabled_; }
    void exitFullscreen();
    
    // Commands
    bool execCommand(const std::string& cmd, bool showUI = false, 
                     const std::string& value = "");
    bool queryCommandEnabled(const std::string& cmd) const;
    bool queryCommandState(const std::string& cmd) const;
    std::string queryCommandValue(const std::string& cmd) const;
    
    // Document write
    void write(const std::string& html);
    void writeln(const std::string& html);
    void open();
    void close();
    
    // Events
    void onReadyStateChange(std::function<void()> cb) { onReadyStateChange_ = cb; }
    void onVisibilityChange(std::function<void()> cb) { onVisibilityChange_ = cb; }
    void onFullScreenChange(std::function<void()> cb) { onFullScreenChange_ = cb; }
    
    // Element registration
    void registerElement(const std::string& id, HTMLElement* el);
    void unregisterElement(const std::string& id);
    
private:
    std::string url_;
    std::string baseURI_;
    std::string charset_ = "UTF-8";
    std::string contentType_ = "text/html";
    std::string referrer_;
    std::string lastModified_;
    std::string designMode_ = "off";
    
    DocumentReadyState readyState_ = DocumentReadyState::Loading;
    
    std::unique_ptr<HTMLHtmlElement> documentElement_;
    HTMLElement* activeElement_ = nullptr;
    HTMLElement* fullscreenElement_ = nullptr;
    
    bool hasFocus_ = true;
    bool hidden_ = false;
    bool fullscreenEnabled_ = true;
    
    std::unordered_map<std::string, HTMLElement*> idMap_;
    
    std::function<void()> onReadyStateChange_;
    std::function<void()> onVisibilityChange_;
    std::function<void()> onFullScreenChange_;
};

} // namespace Zepra::WebCore
