/**
 * @file dom_renderer.cpp
 * @brief NXRender Web Layer - DOM to Widget Implementation
 * 
 * Converts WebCore DOM tree to NXRender widget tree for rendering.
 * Handles all standard HTML elements and CSS styling.
 * 
 * @copyright 2024-2025 KetiveeAI
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
#include "browser/dom.hpp"
#include "css/css_engine.hpp"
#include "css/css_computed_style.hpp"
#endif

#include <algorithm>
#include <cctype>
#include <unordered_set>

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
        font.family = style->fontFamily.empty() ? "sans-serif" : style->fontFamily;
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
// Tag classification
// =============================================================================

namespace {

// Tags that are invisible and should be skipped
const std::unordered_set<std::string> kInvisibleTags = {
    "script", "style", "head", "meta", "link", "title", "base", "noscript"
};

// Tags that are block-level containers
const std::unordered_set<std::string> kBlockContainerTags = {
    "div", "section", "article", "header", "footer", "nav", "main", "aside",
    "figure", "figcaption", "details", "summary", "dialog", "address",
    "blockquote", "fieldset", "pre", "form"
};

// Tags that produce text/label output
const std::unordered_set<std::string> kTextTags = {
    "p", "span", "label", "em", "strong", "b", "i", "u", "s", "small",
    "sub", "sup", "mark", "abbr", "code", "var", "samp", "kbd", "cite",
    "q", "dfn", "time", "data", "ruby", "bdi", "bdo", "wbr"
};

float headingFontSize(const std::string& tag) {
    if (tag == "h1") return 32.0f;
    if (tag == "h2") return 28.0f;
    if (tag == "h3") return 24.0f;
    if (tag == "h4") return 20.0f;
    if (tag == "h5") return 18.0f;
    if (tag == "h6") return 16.0f;
    return 16.0f;
}

} // anonymous namespace

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

    // =========================================================================
    // Apply CSS computed style to a widget
    // =========================================================================
    void applyStyle(Widget* widget, const Zepra::WebCore::CSSComputedStyle* style) {
        if (!widget || !style) return;
        
        // Background color
        if (!style->backgroundColor.isTransparent()) {
            uint32_t bg = (style->backgroundColor.r << 16) | 
                          (style->backgroundColor.g << 8) | 
                          style->backgroundColor.b;
            widget->setBackgroundColor(bg);
        }
        
        // Margins
        widget->setMargin(EdgeInsets{
            style->marginTop.value,
            style->marginRight.value,
            style->marginBottom.value,
            style->marginLeft.value
        });
        
        // Padding (containers only)
        if (auto* cont = dynamic_cast<Container*>(widget)) {
            cont->setPadding(
                style->paddingTop.value,
                style->paddingRight.value,
                style->paddingBottom.value,
                style->paddingLeft.value
            );
            
            // Width / height
            if (style->width.value > 0 && !style->width.isAuto())
                cont->setCSSWidth(style->width.value);
            if (style->height.value > 0 && !style->height.isAuto())
                cont->setCSSHeight(style->height.value);
            
            // Border
            if (style->borderTopWidth > 0) {
                cont->setBorderWidth(style->borderTopWidth);
                uint32_t bc = (style->borderTopColor.r << 16) |
                              (style->borderTopColor.g << 8) |
                              style->borderTopColor.b;
                cont->setBorderColor(bc);
            }
            cont->setBorderRadius(style->borderTopLeftRadius);
            
            // Opacity
            cont->setOpacity(style->opacity);
            
            // Overflow
            if (style->overflowX == Zepra::WebCore::OverflowValue::Hidden ||
                style->overflowX == Zepra::WebCore::OverflowValue::Scroll ||
                style->overflowY == Zepra::WebCore::OverflowValue::Hidden ||
                style->overflowY == Zepra::WebCore::OverflowValue::Scroll) {
                cont->setClipChildren(true);
            }
            
            // Display → layout mode
            switch (style->display) {
                case Zepra::WebCore::DisplayValue::Flex:
                case Zepra::WebCore::DisplayValue::InlineFlex: {
                    cont->setLayoutMode(LayoutMode::Flex);
                    auto& flex = cont->flexLayout();
                    // Direction
                    switch (style->flexDirection) {
                        case Zepra::WebCore::FlexDirection::Row:
                            flex.direction = FlexDirection::Row; break;
                        case Zepra::WebCore::FlexDirection::RowReverse:
                            flex.direction = FlexDirection::RowReverse; break;
                        case Zepra::WebCore::FlexDirection::Column:
                            flex.direction = FlexDirection::Column; break;
                        case Zepra::WebCore::FlexDirection::ColumnReverse:
                            flex.direction = FlexDirection::ColumnReverse; break;
                    }
                    // Justify
                    switch (style->justifyContent) {
                        case Zepra::WebCore::JustifyAlign::FlexStart:
                            flex.justifyContent = JustifyContent::FlexStart; break;
                        case Zepra::WebCore::JustifyAlign::FlexEnd:
                            flex.justifyContent = JustifyContent::FlexEnd; break;
                        case Zepra::WebCore::JustifyAlign::Center:
                            flex.justifyContent = JustifyContent::Center; break;
                        case Zepra::WebCore::JustifyAlign::SpaceBetween:
                            flex.justifyContent = JustifyContent::SpaceBetween; break;
                        case Zepra::WebCore::JustifyAlign::SpaceAround:
                            flex.justifyContent = JustifyContent::SpaceAround; break;
                        case Zepra::WebCore::JustifyAlign::SpaceEvenly:
                            flex.justifyContent = JustifyContent::SpaceEvenly; break;
                        default: break;
                    }
                    // Align items
                    switch (style->alignItems) {
                        case Zepra::WebCore::JustifyAlign::FlexStart:
                            flex.alignItems = AlignItems::FlexStart; break;
                        case Zepra::WebCore::JustifyAlign::FlexEnd:
                            flex.alignItems = AlignItems::FlexEnd; break;
                        case Zepra::WebCore::JustifyAlign::Center:
                            flex.alignItems = AlignItems::Center; break;
                        case Zepra::WebCore::JustifyAlign::Stretch:
                            flex.alignItems = AlignItems::Stretch; break;
                        default: break;
                    }
                    // Wrap
                    if (style->flexWrap)
                        flex.wrap = FlexWrap::Wrap;
                    // Gap
                    if (style->gap.value > 0)
                        flex.setGap(style->gap.value);
                    break;
                }
                case Zepra::WebCore::DisplayValue::None:
                    cont->setLayoutMode(LayoutMode::None);
                    break;
                default:
                    cont->setLayoutMode(LayoutMode::Block);
                    break;
            }
        }
        
        // Visibility
        if (style->visibility == Zepra::WebCore::Visibility::Hidden)
            widget->setVisible(false);
    }

    // =========================================================================
    // Render a DOM node to a widget
    // =========================================================================
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
        auto* element = dynamic_cast<Zepra::WebCore::DOMElement*>(node);
        if (!element) return nullptr;
        
        std::string tag = element->tagName();
        std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
        
        // Skip invisible elements
        if (kInvisibleTags.count(tag)) return nullptr;
        
        // Get computed style
        const Zepra::WebCore::CSSComputedStyle* style = 
            cssEngine_ ? cssEngine_->getComputedStyle(element) : nullptr;
        
        // display: none
        if (style && style->display == Zepra::WebCore::DisplayValue::None)
            return nullptr;
        
        std::unique_ptr<Widget> widget;
        
        // =================================================================
        // Button
        // =================================================================
        if (tag == "button") {
            std::string text = element->textContent();
            widget = std::make_unique<Button>(text);
        }
        // =================================================================
        // Input
        // =================================================================
        else if (tag == "input") {
            std::string type = element->getAttribute("type");
            auto field = std::make_unique<TextField>();
            field->setPlaceholder(element->getAttribute("placeholder"));
            widget = std::move(field);
        }
        // =================================================================
        // Textarea
        // =================================================================
        else if (tag == "textarea") {
            auto field = std::make_unique<TextField>();
            field->setPlaceholder(element->getAttribute("placeholder"));
            field->setText(element->textContent());
            widget = std::move(field);
        }
        // =================================================================
        // Select (dropdown)
        // =================================================================
        else if (tag == "select") {
            // Render as container with text for now
            auto cont = std::make_unique<Container>();
            std::string firstOption;
            for (size_t i = 0; i < element->childNodes().size(); i++) {
                auto* child = element->childNodes()[i].get();
                if (auto* opt = dynamic_cast<Zepra::WebCore::DOMElement*>(child)) {
                    if (firstOption.empty()) firstOption = opt->textContent();
                }
            }
            if (!firstOption.empty()) {
                cont->addChild(std::make_unique<Label>(firstOption));
            }
            widget = std::move(cont);
        }
        // =================================================================
        // Anchor
        // =================================================================
        else if (tag == "a") {
            std::string href = element->getAttribute("href");
            std::string text = element->textContent();
            auto label = std::make_unique<Label>(text);
            label->setColor(0x0066CC);
            widget = std::move(label);
        }
        // =================================================================
        // Headings
        // =================================================================
        else if (tag[0] == 'h' && tag.size() == 2 && tag[1] >= '1' && tag[1] <= '6') {
            std::string text = element->textContent();
            auto label = std::make_unique<Label>(text);
            float fontSize = style ? style->fontSize : headingFontSize(tag);
            label->setFontSize(fontSize);
            widget = std::move(label);
        }
        // =================================================================
        // Image
        // =================================================================
        else if (tag == "img") {
            std::string alt = element->getAttribute("alt");
            // Placeholder: show alt text in a bordered container
            auto cont = std::make_unique<Container>();
            cont->setBorderWidth(1);
            cont->setBorderColor(Color{200, 200, 200, 255});
            
            float w = 150, h = 100;
            std::string wAttr = element->getAttribute("width");
            std::string hAttr = element->getAttribute("height");
            if (!wAttr.empty()) w = std::stof(wAttr);
            if (!hAttr.empty()) h = std::stof(hAttr);
            cont->setCSSWidth(w);
            cont->setCSSHeight(h);
            
            if (!alt.empty()) {
                cont->addChild(std::make_unique<Label>(alt));
            } else {
                cont->addChild(std::make_unique<Label>("[image]"));
            }
            widget = std::move(cont);
        }
        // =================================================================
        // Horizontal Rule
        // =================================================================
        else if (tag == "hr") {
            auto cont = std::make_unique<Container>();
            cont->setCSSHeight(1);
            cont->setBackgroundColor(Color{200, 200, 200, 255});
            cont->setMargin(EdgeInsets{8, 0, 8, 0});
            widget = std::move(cont);
        }
        // =================================================================
        // Line Break
        // =================================================================
        else if (tag == "br") {
            auto label = std::make_unique<Label>("\n");
            widget = std::move(label);
        }
        // =================================================================
        // Unordered / Ordered Lists
        // =================================================================
        else if (tag == "ul" || tag == "ol") {
            auto cont = std::make_unique<Container>();
            cont->setLayoutMode(LayoutMode::Block);
            cont->setPadding(0, 0, 0, 20); // Left indent for list items
            
            int index = 1;
            for (size_t i = 0; i < element->childNodes().size(); i++) {
                auto* childNode = element->childNodes()[i].get();
                auto* childElem = dynamic_cast<Zepra::WebCore::DOMElement*>(childNode);
                if (!childElem) continue;
                
                std::string childTag = childElem->tagName();
                std::transform(childTag.begin(), childTag.end(), childTag.begin(), ::tolower);
                
                if (childTag == "li") {
                    auto itemCont = std::make_unique<Container>();
                    itemCont->setLayoutMode(LayoutMode::Flex);
                    itemCont->flexLayout().direction = FlexDirection::Row;
                    
                    // Bullet or number
                    std::string prefix = (tag == "ul") ? "• " : std::to_string(index++) + ". ";
                    itemCont->addChild(std::make_unique<Label>(prefix));
                    
                    // Item content
                    auto inner = renderNode(childElem);
                    if (inner) {
                        itemCont->addChild(std::move(inner));
                    } else {
                        itemCont->addChild(std::make_unique<Label>(childElem->textContent()));
                    }
                    
                    cont->addChild(std::move(itemCont));
                }
            }
            widget = std::move(cont);
        }
        // =================================================================
        // Table
        // =================================================================
        else if (tag == "table") {
            auto cont = std::make_unique<Container>();
            cont->setLayoutMode(LayoutMode::Block);
            cont->setBorderWidth(1);
            cont->setBorderColor(Color{180, 180, 180, 255});
            
            for (size_t i = 0; i < element->childNodes().size(); i++) {
                auto child = renderNode(element->childNodes()[i].get());
                if (child) cont->addChild(std::move(child));
            }
            widget = std::move(cont);
        }
        else if (tag == "thead" || tag == "tbody" || tag == "tfoot") {
            // Pass through — just render children
            auto cont = std::make_unique<Container>();
            cont->setLayoutMode(LayoutMode::Block);
            for (size_t i = 0; i < element->childNodes().size(); i++) {
                auto child = renderNode(element->childNodes()[i].get());
                if (child) cont->addChild(std::move(child));
            }
            if (cont->childCount() > 0) widget = std::move(cont);
        }
        else if (tag == "tr") {
            auto row = std::make_unique<Container>();
            row->setLayoutMode(LayoutMode::Flex);
            row->flexLayout().direction = FlexDirection::Row;
            row->setBorderWidth(1);
            row->setBorderColor(Color{220, 220, 220, 255});
            
            for (size_t i = 0; i < element->childNodes().size(); i++) {
                auto child = renderNode(element->childNodes()[i].get());
                if (child) row->addChild(std::move(child));
            }
            if (row->childCount() > 0) widget = std::move(row);
        }
        else if (tag == "td" || tag == "th") {
            auto cell = std::make_unique<Container>();
            cell->setPadding(4, 8, 4, 8);
            if (tag == "th") {
                // Bold text for header cells
            }
            
            std::string text = element->textContent();
            if (!text.empty()) {
                cell->addChild(std::make_unique<Label>(text));
            }
            for (size_t i = 0; i < element->childNodes().size(); i++) {
                auto child = renderNode(element->childNodes()[i].get());
                if (child) cell->addChild(std::move(child));
            }
            widget = std::move(cell);
        }
        // =================================================================
        // Text-producing tags (p, span, em, strong, etc.)
        // =================================================================
        else if (kTextTags.count(tag)) {
            // Check if has child elements (mixed content)
            bool hasChildElements = false;
            for (size_t i = 0; i < element->childNodes().size(); i++) {
                if (dynamic_cast<Zepra::WebCore::DOMElement*>(element->childNodes()[i].get())) {
                    hasChildElements = true;
                    break;
                }
            }
            
            if (hasChildElements) {
                // Mixed content — render as container
                auto cont = std::make_unique<Container>();
                if (tag == "p") {
                    cont->setLayoutMode(LayoutMode::Block);
                    cont->setMargin(EdgeInsets{8, 0, 8, 0});
                } else {
                    cont->setLayoutMode(LayoutMode::Flex);
                    cont->flexLayout().direction = FlexDirection::Row;
                }
                
                for (size_t i = 0; i < element->childNodes().size(); i++) {
                    auto child = renderNode(element->childNodes()[i].get());
                    if (child) cont->addChild(std::move(child));
                }
                if (cont->childCount() > 0) widget = std::move(cont);
            } else {
                std::string text = element->textContent();
                if (!text.empty()) {
                    auto label = std::make_unique<Label>(text);
                    if (tag == "p") {
                        label->setMargin(EdgeInsets{8, 0, 8, 0});
                    }
                    widget = std::move(label);
                }
            }
        }
        // =================================================================
        // Block container tags (div, section, article, header, etc.)
        // =================================================================
        else if (kBlockContainerTags.count(tag)) {
            auto container = std::make_unique<Container>();
            container->setLayoutMode(LayoutMode::Block);
            
            for (size_t i = 0; i < element->childNodes().size(); i++) {
                auto child = renderNode(element->childNodes()[i].get());
                if (child) container->addChild(std::move(child));
            }
            
            if (container->childCount() > 0) widget = std::move(container);
        }
        // =================================================================
        // Fallback: generic container
        // =================================================================
        else {
            auto container = std::make_unique<Container>();
            container->setLayoutMode(LayoutMode::Block);
            
            for (size_t i = 0; i < element->childNodes().size(); i++) {
                auto child = renderNode(element->childNodes()[i].get());
                if (child) container->addChild(std::move(child));
            }
            
            if (container->childCount() > 0) {
                widget = std::move(container);
            }
        }
        
        // Apply CSS styles
        if (widget) {
            applyStyle(widget.get(), style);
        }
        
        return widget;
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
    rootContainer->setLayoutMode(LayoutMode::Block);
    
    if (auto* element = dynamic_cast<Zepra::WebCore::DOMElement*>(root)) {
        for (size_t i = 0; i < element->childNodes().size(); i++) {
            auto child = impl_->renderNode(element->childNodes()[i].get());
            if (child) {
                rootContainer->addChild(std::move(child));
            }
        }
    }
    
    // Run layout pass
    rootContainer->layout();
    
    result.rootWidget = std::move(rootContainer);
    result.contentHeight = options.viewportHeight;  // TODO: actual content height
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
    auto root = std::make_unique<Container>();
    root->setSize(width, height);
    root->setLayoutMode(LayoutMode::Block);
    return root;
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
