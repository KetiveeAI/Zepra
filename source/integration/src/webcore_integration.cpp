/*
 * WebCore Integration Implementation
 * 
 * Links native browser with:
 * - HTMLParser for DOM construction
 * - CSSEngine for style computation
 * - ScriptContext for JavaScript execution
 */

#include "html_parser.hpp"
#include "dom.hpp"
#include "css/css_engine.hpp"
#include "script_context.hpp"
#include "render_tree.hpp"

#include <iostream>
#include <vector>
#include <memory>

namespace ZepraNative {

using namespace Zepra::WebCore;

/**
 * @brief Renderable line with computed styles
 */
struct StyledLine {
    std::string text;
    float x, y, width, height;
    
    // Computed CSS values
    uint32_t color = 0x000000;
    uint32_t backgroundColor = 0xFFFFFF;
    float fontSize = 16.0f;
    bool bold = false;
    bool italic = false;
    std::string fontFamily = "sans-serif";
    std::string display = "block";
};

/**
 * @brief Page renderer using WebCore
 */
class PageRenderer {
public:
    PageRenderer() {
        cssEngine_ = std::make_unique<CSSEngine>();
        scriptContext_ = std::make_unique<ScriptContext>();
    }
    
    ~PageRenderer() = default;
    
    /**
     * @brief Parse HTML and build DOM
     */
    bool loadHTML(const std::string& html, const std::string& url) {
        url_ = url;
        
        // Parse HTML to DOM
        HTMLParser parser;
        document_ = parser.parse(html);
        
        if (!document_) {
            std::cerr << "[PageRenderer] Failed to parse HTML" << std::endl;
            return false;
        }
        
        // Initialize CSS engine with document
        cssEngine_->initialize(document_.get());
        
        // Extract and parse stylesheets
        extractStylesheets();
        
        // Compute styles for all elements
        cssEngine_->computeStyles();
        
        // Initialize script context
        scriptContext_->initialize(document_.get());
        
        // Execute inline scripts
        executeScripts();
        
        std::cout << "[PageRenderer] Loaded page with " 
                  << countElements(document_->body()) << " elements" << std::endl;
        
        return true;
    }
    
    /**
     * @brief Get page title
     */
    std::string getTitle() const {
        if (!document_) return "";
        
        auto* head = document_->head();
        if (!head) return "";
        
        // Find <title> element
        for (size_t i = 0; i < head->childNodes().size(); i++) {
            if (auto* el = dynamic_cast<DOMElement*>(head->childNodes()[i].get())) {
                if (el->tagName() == "TITLE" || el->tagName() == "title") {
                    return el->textContent();
                }
            }
        }
        return "";
    }
    
    /**
     * @brief Generate styled lines for rendering
     */
    std::vector<StyledLine> getStyledLines(float viewWidth, float viewHeight) {
        std::vector<StyledLine> lines;
        
        if (!document_ || !document_->body()) {
            return lines;
        }
        
        float y = 10.0f;
        float x = 10.0f;
        float maxWidth = viewWidth - 20.0f;
        
        // Traverse DOM and create lines
        traverseForRendering(document_->body(), x, y, maxWidth, lines);
        
        return lines;
    }
    
    /**
     * @brief Execute JavaScript code
     */
    bool executeJS(const std::string& script) {
        if (!scriptContext_) return false;
        
        auto result = scriptContext_->evaluate(script);
        if (!result.success) {
            std::cerr << "[JS Error] " << result.error << std::endl;
            return false;
        }
        return true;
    }
    
private:
    void extractStylesheets() {
        if (!document_) return;
        
        // Add default user-agent stylesheet
        cssEngine_->addStyleSheet(
            "body { margin: 8px; font-family: sans-serif; font-size: 16px; color: #1f2328; }\n"
            "h1 { font-size: 32px; font-weight: bold; margin: 16px 0; }\n"
            "h2 { font-size: 24px; font-weight: bold; margin: 12px 0; }\n"
            "h3 { font-size: 18px; font-weight: bold; margin: 10px 0; }\n"
            "p { margin: 8px 0; line-height: 1.5; }\n"
            "a { color: #0066cc; text-decoration: underline; }\n"
            "strong, b { font-weight: bold; }\n"
            "em, i { font-style: italic; }\n"
            "code { font-family: monospace; background: #f0f0f0; padding: 2px 4px; }\n"
            "pre { font-family: monospace; background: #f0f0f0; padding: 8px; }\n",
            StyleOrigin::UserAgent
        );
        
        // Extract <style> elements from <head>
        auto* head = document_->head();
        if (head) {
            for (size_t i = 0; i < head->childNodes().size(); i++) {
                if (auto* el = dynamic_cast<DOMElement*>(head->childNodes()[i].get())) {
                    if (el->tagName() == "STYLE" || el->tagName() == "style") {
                        cssEngine_->addStyleSheet(el->textContent(), StyleOrigin::Author);
                    }
                }
            }
        }
    }
    
    void executeScripts() {
        if (!document_ || !scriptContext_) return;
        
        // Find and execute <script> elements
        executeScriptsInElement(document_->body());
        
        // Fire DOMContentLoaded
        scriptContext_->fireDOMContentLoaded();
    }
    
    void executeScriptsInElement(DOMElement* element) {
        if (!element) return;
        
        for (size_t i = 0; i < element->childNodes().size(); i++) {
            if (auto* el = dynamic_cast<DOMElement*>(element->childNodes()[i].get())) {
                if (el->tagName() == "SCRIPT" || el->tagName() == "script") {
                    std::string code = el->textContent();
                    if (!code.empty()) {
                        std::cout << "[JS] Executing script (" << code.length() << " bytes)" << std::endl;
                        executeJS(code);
                    }
                } else {
                    executeScriptsInElement(el);
                }
            }
        }
    }
    
    void traverseForRendering(DOMElement* element, float& x, float& y, 
                               float maxWidth, std::vector<StyledLine>& lines) {
        if (!element) return;
        
        // Get computed style
        const CSSComputedStyle* style = cssEngine_->getComputedStyle(element);
        
        // Check display:none
        if (style && style->display == DisplayValue::None) {
            return;
        }
        
        // Process children
        for (size_t i = 0; i < element->childNodes().size(); i++) {
            DOMNode* child = element->childNodes()[i].get();
            
            if (auto* textNode = dynamic_cast<DOMText*>(child)) {
                // Render text - DOMText uses data(), not textContent()
                std::string text = textNode->data();
                if (!text.empty() && text.find_first_not_of(" \t\n\r") != std::string::npos) {
                    // Trim and normalize whitespace
                    text = normalizeWhitespace(text);
                    
                    if (!text.empty()) {
                        StyledLine line;
                        line.text = text;
                        line.x = x;
                        line.y = y;
                        
                        if (style) {
                            line.color = cssColorToUint32(style->color);
                            line.backgroundColor = cssColorToUint32(style->backgroundColor);
                            line.fontSize = style->fontSize;
                            line.bold = (style->fontWeight >= FontWeight::Bold);
                            line.italic = (style->fontStyle == FontStyle::Italic);
                        }
                        
                        lines.push_back(line);
                        y += line.fontSize + 4;  // Line height
                    }
                }
            } else if (auto* childElement = dynamic_cast<DOMElement*>(child)) {
                // Check for block elements
                std::string tag = childElement->tagName();
                std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
                
                bool isBlock = (tag == "div" || tag == "p" || tag == "h1" || 
                               tag == "h2" || tag == "h3" || tag == "h4" ||
                               tag == "li" || tag == "ul" || tag == "ol" ||
                               tag == "pre" || tag == "blockquote");
                
                if (isBlock) {
                    y += 8;  // Block margin
                }
                
                traverseForRendering(childElement, x, y, maxWidth, lines);
                
                if (isBlock) {
                    y += 8;  // Block margin
                }
            }
        }
    }
    
    static std::string normalizeWhitespace(const std::string& text) {
        std::string result;
        bool lastWasSpace = true;
        
        for (char c : text) {
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                if (!lastWasSpace) {
                    result += ' ';
                    lastWasSpace = true;
                }
            } else {
                result += c;
                lastWasSpace = false;
            }
        }
        
        // Trim trailing space
        if (!result.empty() && result.back() == ' ') {
            result.pop_back();
        }
        
        return result;
    }
    
    static uint32_t cssColorToUint32(const CSSColor& color) {
        return (color.r << 16) | (color.g << 8) | color.b;
    }
    
    static size_t countElements(DOMElement* el) {
        if (!el) return 0;
        size_t count = 1;
        for (size_t i = 0; i < el->childNodes().size(); i++) {
            if (auto* child = dynamic_cast<DOMElement*>(el->childNodes()[i].get())) {
                count += countElements(child);
            }
        }
        return count;
    }
    
    std::unique_ptr<DOMDocument> document_;
    std::unique_ptr<CSSEngine> cssEngine_;
    std::unique_ptr<ScriptContext> scriptContext_;
    std::string url_;
};

// Global instance
static std::unique_ptr<PageRenderer> g_renderer;

// C API for easy integration
extern "C" {

int webcore_init() {
    g_renderer = std::make_unique<PageRenderer>();
    return 1;
}

int webcore_load_html(const char* html, const char* url) {
    if (!g_renderer) return 0;
    return g_renderer->loadHTML(html, url) ? 1 : 0;
}

const char* webcore_get_title() {
    static std::string title;
    if (!g_renderer) return "";
    title = g_renderer->getTitle();
    return title.c_str();
}

int webcore_exec_js(const char* script) {
    if (!g_renderer) return 0;
    return g_renderer->executeJS(script) ? 1 : 0;
}

void webcore_shutdown() {
    g_renderer.reset();
}

} // extern "C"

} // namespace ZepraNative
