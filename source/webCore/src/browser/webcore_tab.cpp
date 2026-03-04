/**
 * @file webcore_tab.cpp
 * @brief Per-Tab WebCore Instance Implementation
 */

#include "webcore_tab.h"
#include <nxgfx/context.h>
#include <iostream>

#ifdef USE_WEBCORE
#include "html_parser.hpp"
#include "dom.hpp"
#include "css/css_engine.hpp"
#include "css/css_computed_style.hpp"
#include "script_context.hpp"
#include <algorithm>
using namespace Zepra::WebCore;
#endif

namespace ZepraBrowser {

WebCoreTab::WebCoreTab(const std::string& url)
    : url_(url)
    , isLoading_(false)
{
    std::cout << "[WebCoreTab] Created for URL: " << url << std::endl;
    
#ifdef USE_WEBCORE
    // Initialize per-tab WebCore instances
    cssEngine_ = std::make_unique<CSSEngine>();
    scriptContext_ = std::make_unique<ScriptContext>();
    std::cout << "[WebCoreTab] Initialized DOM, CSS, and ScriptContext" << std::endl;
#endif
    
    // Load initial URL
    if (url != "zepra://start") {
        loadURL(url);
    }
}

WebCoreTab::~WebCoreTab() {
    std::cout << "[WebCoreTab] Destroying tab: " << url_ << std::endl;
}

void WebCoreTab::loadURL(const std::string& url) {
    url_ = url;
    isLoading_ = true;
    loadError_.clear();
    
    std::cout << "[WebCoreTab] Loading URL: " << url << std::endl;
    
    // TODO: Use nxhttp to fetch page content
    // For now, just mark as not loading
    isLoading_ = false;
}

void WebCoreTab::reload() {
    loadURL(url_);
}

void WebCoreTab::stop() {
    isLoading_ = false;
}

void WebCoreTab::parseHTML(const std::string& html) {
#ifdef USE_WEBCORE
    if (html.empty()) return;
    
    // Parse HTML to DOM
    HTMLParser parser;
    document_ = parser.parse(html);
    
    if (!document_) {
        loadError_ = "Failed to parse HTML";
        std::cerr << "[WebCore Tab] " << loadError_ << std::endl;
        return;
    }
    
    // Initialize CSS engine with document
    cssEngine_->initialize(document_.get());
    
    // Add default user-agent stylesheet
    cssEngine_->addStyleSheet(
        "body { margin: 8px; font-family: sans-serif; font-size: 16px; color: #1f2328; }\\n"
        "h1 { font-size: 32px; font-weight: bold; margin: 16px 0; }\\n"
        "h2 { font-size: 24px; font-weight: bold; margin: 12px 0; }\\n"
        "p { margin: 8px 0; line-height: 1.5; }\\n"
        "a { color: #0066cc; text-decoration: underline; }\\n",
        StyleOrigin::UserAgent
    );
    
    // Compute styles
    cssEngine_->computeStyles();
    
    // Initialize script context
    scriptContext_->initialize(document_.get());
    
    // Build layout tree
    buildLayoutTree();
    
    std::cout << "[WebCoreTab] Parsed HTML, built DOM and layout tree" << std::endl;
#endif
}

void WebCoreTab::buildLayoutTree() {
#ifdef USE_WEBCORE
    if (!document_ || !document_->body() || !cssEngine_) {
        return;
    }
    
    // Create root layout box from <body>
    layoutRoot_ = std::make_unique<LayoutBox>();
    layoutRoot_->type = LayoutType::Block;
    
    // Build layout tree recursively from body
    buildLayoutFromElement(document_->body(), layoutRoot_.get());
    
    std::cout << "[WebCoreTab] Built layout tree with " 
              << countLayoutBoxes(*layoutRoot_) << " boxes" << std::endl;
#endif
}

#ifdef USE_WEBCORE
void WebCoreTab::buildLayoutFromElement(DOMElement* element, LayoutBox* parent) {
    if (!element || !parent) return;
    
    // Get computed style using our new StyleResolver
    const CSSComputedStyle* style = cssEngine_->getComputedStyle(element);
    
    // Check display:none - skip hidden elements
    if (style && style->display == DisplayValue::None) {
        return;
    }
    
    // Process child nodes
    for (size_t i = 0; i < element->childNodes().size(); i++) {
        DOMNode* child = element->childNodes()[i].get();
        
        if (auto* textNode = dynamic_cast<DOMText*>(child)) {
            // Text node - create text box
            std::string text = textNode->data();
            
            // Normalize whitespace
            text = normalizeWhitespace(text);
            
            if (!text.empty()) {
                LayoutBox textBox;
                textBox.type = LayoutType::Text;
                textBox.text = text;
                
                // Apply inherited styles from parent element
                if (style) {
                    textBox.color = cssColorToUint32(style->color);
                    textBox.fontSize = style->fontSize;
                    textBox.bold = (style->fontWeight >= FontWeight::Bold);
                    textBox.italic = (style->fontStyle == FontStyle::Italic);
                }
                
                parent->children.push_back(std::move(textBox));
            }
        } else if (auto* childElement = dynamic_cast<DOMElement*>(child)) {
            // Element node - create layout box
            LayoutBox box;
            
            // Get computed style for this element
            const CSSComputedStyle* childStyle = cssEngine_->getComputedStyle(childElement);
            
            // Set layout type based on display value
            if (childStyle) {
                switch (childStyle->display) {
                    case DisplayValue::None:
                        continue; // Skip hidden elements
                    case DisplayValue::Block:
                        box.type = LayoutType::Block;
                        break;
                    case DisplayValue::Inline:
                        box.type = LayoutType::Inline;
                        break;
                    case DisplayValue::InlineBlock:
                        box.type = LayoutType::InlineBlock;
                        break;
                    case DisplayValue::Flex:
                        box.type = LayoutType::Flex;
                        break;
                    default:
                        box.type = LayoutType::Block;
                }
                
                // Apply style properties
                box.color = cssColorToUint32(childStyle->color);
                box.fontSize = childStyle->fontSize;
                box.bold = (childStyle->fontWeight >= FontWeight::Bold);
                box.italic = (childStyle->fontStyle == FontStyle::Italic);
                
                // Background
                if (childStyle->backgroundColor.a > 0) {
                    box.hasBgColor = true;
                    box.bgColor = cssColorToUint32(childStyle->backgroundColor);
                }
                
                // Box model (convert lengths to pixels)
                box.marginTop = childStyle->marginTop.toPx(16, 16, 1920, 1080, 0);
                box.marginRight = childStyle->marginRight.toPx(16, 16, 1920, 1080, 0);
                box.marginBottom = childStyle->marginBottom.toPx(16, 16, 1920, 1080, 0);
                box.marginLeft = childStyle->marginLeft.toPx(16, 16, 1920, 1080, 0);
                
                box.paddingTop = childStyle->paddingTop.toPx(16, 16, 1920, 1080, 0);
                box.paddingRight = childStyle->paddingRight.toPx(16, 16, 1920, 1080, 0);
                box.paddingBottom = childStyle->paddingBottom.toPx(16, 16, 1920, 1080, 0);
                box.paddingLeft = childStyle->paddingLeft.toPx(16, 16, 1920, 1080, 0);
            }
            
            // Check for links
            std::string tagName = childElement->tagName();
            std::transform(tagName.begin(), tagName.end(), tagName.begin(), ::tolower);
            if (tagName == "a") {
                box.isLink = true;
                box.href = childElement->getAttribute("href");
                box.target = childElement->getAttribute("target");
            }
            
            // Check for images
            if (tagName == "img") {
                box.isImage = true;
                // Note: texture loading handled separately
            }
            
            // Check for inputs
            if (tagName == "input") {
                box.isInput = true;
                box.inputType = childElement->getAttribute("type");
                box.placeholder = childElement->getAttribute("placeholder");
            }
            
            // Recursively build children
            buildLayoutFromElement(childElement, &box);
            
            parent->children.push_back(std::move(box));
        }
    }
}

std::string WebCoreTab::normalizeWhitespace(const std::string& text) {
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

uint32_t WebCoreTab::cssColorToUint32(const CSSColor& color) {
    return (color.r << 16) | (color.g << 8) | color.b;
}

size_t WebCoreTab::countLayoutBoxes(const LayoutBox& box) {
    size_t count = 1;
    for (const auto& child : box.children) {
        count += countLayoutBoxes(child);
    }
    return count;
}
#endif

void WebCoreTab::render(NXRender::GpuContext* gpu, float x, float y, float width, float height) {
    if (!gpu) return;
    
    // Render tab content using nxgfx
    if (url_ == "zepra://start") {
        // Render start page (implemented elsewhere)
        return;
    }
    
    if (isLoading_) {
        // Draw loading indicator
        gpu->drawText("Loading...", x + 20, y + 40, NXRender::Color(0x666666));
        return;
    }
    
    if (!loadError_.empty()) {
        // Draw error message
        gpu->drawText("Failed to load page", x + 20, y + 40, NXRender::Color(0xCC0000));
        gpu->drawText("Error: " + loadError_, x + 20, y + 70, NXRender::Color(0x666666));
        return;
    }
    
#ifdef USE_WEBCORE
    // Render layout tree if available
    if (layoutRoot_) {
        // Paint layout boxes recursively using NXRender
        paintLayoutBox(gpu, *layoutRoot_, x, y, width, height, 0.0f);
    }
#endif
}

void WebCoreTab::paintLayoutBox(NXRender::GpuContext* gpu, const LayoutBox& box, 
                                 float offsetX, float offsetY, float maxWidth, float maxHeight, float scrollY) {
    // Calculate absolute position
    float absX = offsetX + box.x;
    float absY = offsetY + box.y - scrollY;
    
    // Clip check - skip if completely outside viewport
    if (absY + box.height < 0 || absY > maxHeight) {
        return;
    }
    
    // Draw background if present (hasBgColor indicates explicit background)
    if (box.hasBgColor) {
        gpu->fillRect(NXRender::Rect(absX, absY, box.width, box.height), NXRender::Color(box.bgColor));
    }
    
    // Draw border if present (uses borderTop/etc for thickness)
    float borderThickness = box.borderTop;  // Use top border width
    if (borderThickness > 0) {
        gpu->strokeRect(NXRender::Rect(absX, absY, box.width, box.height), NXRender::Color(box.borderColor), borderThickness);
    }
    
    // Draw text content if this is a text box
    if (box.type == LayoutType::Text && !box.text.empty()) {
        gpu->drawText(box.text, absX + box.paddingLeft, absY + box.paddingTop, 
                      NXRender::Color(box.color), box.fontSize);
    }
    
    // Draw image if present
    if (box.isImage && box.textureId > 0) {
        // Use textureId for drawing (NXRender should have texture support)
        // gpu->drawTexture(box.textureId, absX, absY, box.width, box.height);
    }
    
    // Recursively paint children (children is vector of LayoutBox, not pointers)
    for (const auto& child : box.children) {
        paintLayoutBox(gpu, child, absX + box.paddingLeft, absY + box.paddingTop, 
                       box.width - box.paddingLeft - box.paddingRight,
                       box.height - box.paddingTop - box.paddingBottom, scrollY);
    }
}

bool WebCoreTab::executeScript(const std::string& code) {
#ifdef USE_WEBCORE
    if (!scriptContext_) return false;
    
    auto result = scriptContext_->evaluate(code);
    if (!result.success) {
        std::cerr << "[WebCoreTab JS Error] " << result.error << std::endl;
        return false;
    }
    return true;
#else
    return false;
#endif
}

std::string WebCoreTab::title() const {
#ifdef USE_WEBCORE
    if (!document_) return "New Tab";
    
    auto* head = document_->head();
    if (!head) return url_;
    
    // Find <title> element
    for (size_t i = 0; i < head->childNodes().size(); i++) {
        if (auto* el = dynamic_cast<DOMElement*>(head->childNodes()[i].get())) {
            if (el->tagName() == "TITLE" || el->tagName() == "title") {
                return el->textContent();
            }
        }
    }
#endif
    
    return url_.empty() ? "New Tab" : url_;
}

} // namespace ZepraBrowser
