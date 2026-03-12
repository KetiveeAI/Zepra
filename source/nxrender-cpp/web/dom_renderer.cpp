/**
 * @file dom_renderer.cpp
 * @brief NXRender Web Layer - DOM to Widget Implementation
 * 
 * @copyright 2024 KetiveeAI
 */

#include "web/dom_renderer.h"
#include "web/style_resolver.h"
#include "widgets/container.h"
#include "widgets/label.h"
#include "widgets/button.h"
#include "widgets/textfield.h"
#include "theme/theme.h"
#include "nxgfx/color.h"

#ifdef USE_WEBCORE
#include "webcore/dom.hpp"
#include "webcore/css/css_engine.hpp"
#endif

#include <algorithm>
#include <cctype>

namespace NXRender::Web {

// =============================================================================
// Style Resolver Implementation
// =============================================================================

Color resolveColor(uint32_t cssColor) {
    return Color{
        static_cast<uint8_t>((cssColor >> 16) & 0xFF),
        static_cast<uint8_t>((cssColor >> 8) & 0xFF),
        static_cast<uint8_t>(cssColor & 0xFF),
        255
    };
}

#ifdef USE_WEBCORE
ResolvedFont resolveFont(const Zepra::WebCore::CSSComputedStyle* style) {
    ResolvedFont font;
    if (style) {
        font.family = "sans-serif";  // Default
        font.size = style->fontSize;
        font.bold = static_cast<int>(style->fontWeight) >= 700;
        font.italic = style->fontStyle == Zepra::WebCore::FontStyle::Italic;
    }
    return font;
}
#else
ResolvedFont resolveFont(const void*) {
    return {"sans-serif", 16.0f, false, false};
}
#endif

// =============================================================================
// DOMRenderer Implementation
// =============================================================================

class DOMRenderer::Impl {
public:
    std::function<void(const std::string&)> linkCallback_;
    std::function<void(const std::string&)> formCallback_;
    
#ifdef USE_WEBCORE
    Zepra::WebCore::CSSEngine* cssEngine_ = nullptr;
    Theme* theme_ = nullptr;
    RenderOptions options_;
    
    std::unique_ptr<Widget> renderNode(Zepra::WebCore::DOMNode* node) {
        if (!node) return nullptr;
        
        // Text node
        if (auto* textNode = dynamic_cast<Zepra::WebCore::DOMText*>(node)) {
            std::string text = textNode->data();
            
            // Normalize whitespace
            std::string normalized;
            bool lastSpace = true;
            for (char c : text) {
                if (std::isspace(c)) {
                    if (!lastSpace) { normalized += ' '; lastSpace = true; }
                } else {
                    normalized += c; lastSpace = false;
                }
            }
            
            if (normalized.empty() || normalized == " ") return nullptr;
            
            auto label = std::make_unique<Label>(normalized);
            return label;
        }
        
        // Element node
        if (auto* element = dynamic_cast<Zepra::WebCore::DOMElement*>(node)) {
            std::string tag = element->tagName();
            std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
            
            // Skip invisible elements
            if (tag == "script" || tag == "style" || tag == "head" || 
                tag == "meta" || tag == "link" || tag == "title") {
                return nullptr;
            }
            
            // Get computed style
            const Zepra::WebCore::CSSComputedStyle* style = 
                cssEngine_ ? cssEngine_->getComputedStyle(element) : nullptr;
            
            // Skip display: none
            if (style && static_cast<int>(style->display) == 0) {
                return nullptr;
            }
            
            // Create widget based on tag
            std::unique_ptr<Widget> widget;
            
            if (tag == "button") {
                std::string text = element->textContent();
                auto btn = std::make_unique<Button>(text);
                widget = std::move(btn);
            }
            else if (tag == "input") {
                std::string type = element->getAttribute("type");
                std::string placeholder = element->getAttribute("placeholder");
                auto field = std::make_unique<TextField>();
                field->setPlaceholder(placeholder);
                widget = std::move(field);
            }
            else if (tag == "a") {
                std::string href = element->getAttribute("href");
                std::string text = element->textContent();
                auto label = std::make_unique<Label>(text);
                label->setColor(0x0066CC);  // Link color
                // TODO: Add click handler
                widget = std::move(label);
            }
            else if (tag == "h1" || tag == "h2" || tag == "h3" || 
                     tag == "h4" || tag == "h5" || tag == "h6") {
                std::string text = element->textContent();
                auto label = std::make_unique<Label>(text);
                if (style) {
                    label->setFontSize(style->fontSize);
                }
                widget = std::move(label);
            }
            else if (tag == "p" || tag == "span" || tag == "label") {
                std::string text = element->textContent();
                if (!text.empty()) {
                    auto label = std::make_unique<Label>(text);
                    widget = std::move(label);
                }
            }
            else {
                // Container elements (div, section, article, etc.)
                auto container = std::make_unique<Container>();
                
                // Render children
                for (size_t i = 0; i < element->childNodes().size(); i++) {
                    auto child = renderNode(element->childNodes()[i].get());
                    if (child) {
                        container->addChild(std::move(child));
                    }
                }
                
                if (container->childCount() > 0) {
                    widget = std::move(container);
                }
            }
            
            // Apply styles
            if (widget && style) {
                // Background
                if (!style->backgroundColor.isTransparent()) {
                    uint32_t bg = (style->backgroundColor.r << 16) | 
                                  (style->backgroundColor.g << 8) | 
                                  style->backgroundColor.b;
                    widget->setBackgroundColor(bg);
                }
                
                // Margins and padding (for containers)
                if (auto* cont = dynamic_cast<Container*>(widget.get())) {
                    cont->setPadding(
                        style->paddingTop.value,
                        style->paddingRight.value,
                        style->paddingBottom.value,
                        style->paddingLeft.value
                    );
                }
            }
            
            return widget;
        }
        
        return nullptr;
    }
#endif
};

DOMRenderer::DOMRenderer() : impl_(std::make_unique<Impl>()) {}
DOMRenderer::~DOMRenderer() = default;

RenderResult DOMRenderer::render(
    Zepra::WebCore::DOMNode* root,
    Zepra::WebCore::CSSEngine* cssEngine,
    const RenderOptions& options)
{
    RenderResult result;
    result.contentHeight = 0;
    
#ifdef USE_WEBCORE
    impl_->cssEngine_ = cssEngine;
    impl_->theme_ = options.theme;
    impl_->options_ = options;
    
    auto rootContainer = std::make_unique<Container>();
    rootContainer->setSize(options.viewportWidth, options.viewportHeight);
    
    if (auto* element = dynamic_cast<Zepra::WebCore::DOMElement*>(root)) {
        for (size_t i = 0; i < element->childNodes().size(); i++) {
            auto child = impl_->renderNode(element->childNodes()[i].get());
            if (child) {
                rootContainer->addChild(std::move(child));
            }
        }
    }
    
    result.rootWidget = std::move(rootContainer);
    result.contentHeight = options.viewportHeight;  // TODO: Calculate actual height
#endif
    
    return result;
}

void DOMRenderer::setLinkCallback(std::function<void(const std::string&)> callback) {
    impl_->linkCallback_ = std::move(callback);
}

void DOMRenderer::setFormCallback(std::function<void(const std::string&)> callback) {
    impl_->formCallback_ = std::move(callback);
}

// =============================================================================
// Quick Functions
// =============================================================================

std::unique_ptr<Container> renderHTML(
    const std::string& html,
    float width,
    float height,
    Theme* theme)
{
    // TODO: Parse HTML and render
    return std::make_unique<Container>();
}

std::unique_ptr<Widget> renderElement(
    Zepra::WebCore::DOMElement* element,
    Zepra::WebCore::CSSEngine* cssEngine)
{
#ifdef USE_WEBCORE
    DOMRenderer renderer;
    RenderOptions opts;
    auto result = renderer.render(element, cssEngine, opts);
    if (result.rootWidget) {
        return std::move(result.rootWidget);
    }
#endif
    return nullptr;
}

} // namespace NXRender::Web
