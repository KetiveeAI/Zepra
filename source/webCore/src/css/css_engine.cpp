/**
 * @file css_engine.cpp
 * @brief CSS cascade, parsing, and style resolution
 */

#include "webcore/css/css_engine.hpp"
#include "webcore/dom.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace Zepra::WebCore {

// =============================================================================
// MatchedRule
// =============================================================================

bool MatchedRule::operator<(const MatchedRule& other) const {
    // Cascade order: origin < importance < specificity < order
    if (origin != other.origin) return origin < other.origin;
    if (specificity < other.specificity) return true;
    if (specificity > other.specificity) return false;
    return order < other.order;
}

// =============================================================================
// CSSCascade
// =============================================================================

std::vector<MatchedRule> CSSCascade::collectMatchingRules(
    DOMElement* element,
    const std::vector<CSSStyleSheet*>& stylesheets,
    StyleOrigin origin) {
    
    std::vector<MatchedRule> results;
    size_t order = 0;
    
    for (auto* sheet : stylesheets) {
        if (!sheet || sheet->disabled()) continue;
        
        auto* rules = sheet->cssRules();
        if (!rules) continue;
        
        for (size_t i = 0; i < rules->length(); ++i) {
            auto* rule = rules->item(i);
            if (!rule) continue;
            
            if (rule->type() == CSSRuleType::Style) {
                auto* styleRule = static_cast<CSSStyleRule*>(rule);
                Selector sel = Selector::parse(styleRule->selectorText());
                
                if (sel.matches(element)) {
                    MatchedRule matched;
                    matched.rule = styleRule;
                    matched.origin = origin;
                    matched.specificity = sel.specificity();
                    matched.order = order++;
                    results.push_back(matched);
                }
            }
        }
    }
    
    return results;
}

void CSSCascade::sortByCascade(std::vector<MatchedRule>& rules) {
    std::stable_sort(rules.begin(), rules.end());
}

std::optional<std::string> CSSCascade::cascadedValue(
    const std::string& property,
    const std::vector<MatchedRule>& rules) {
    
    // Find the winning value (last in sorted order with the property)
    for (auto it = rules.rbegin(); it != rules.rend(); ++it) {
        auto* decl = it->rule->style();
        std::string value = decl->getPropertyValue(property);
        if (!value.empty()) {
            return value;
        }
    }
    
    return std::nullopt;
}

// =============================================================================
// StyleResolver
// =============================================================================

StyleResolver::StyleResolver() = default;
StyleResolver::~StyleResolver() = default;

void StyleResolver::addStyleSheet(std::shared_ptr<CSSStyleSheet> sheet, StyleOrigin origin) {
    stylesheets_.push_back({std::move(sheet), origin});
}

void StyleResolver::addUserAgentStyleSheet() {
    auto sheet = std::make_shared<CSSStyleSheet>();
    
    // Minimal UA stylesheet
    const char* uaCSS = R"(
        html, body, div, span, p, a, img, ul, ol, li, table, tr, td, th,
        h1, h2, h3, h4, h5, h6, form, input, button, textarea, select {
            margin: 0;
            padding: 0;
            border: 0;
        }
        
        html { display: block; }
        body { display: block; margin: 8px; }
        
        h1 { display: block; font-size: 2em; font-weight: bold; margin: 0.67em 0; }
        h2 { display: block; font-size: 1.5em; font-weight: bold; margin: 0.83em 0; }
        h3 { display: block; font-size: 1.17em; font-weight: bold; margin: 1em 0; }
        h4 { display: block; font-weight: bold; margin: 1.33em 0; }
        h5 { display: block; font-size: 0.83em; font-weight: bold; margin: 1.67em 0; }
        h6 { display: block; font-size: 0.67em; font-weight: bold; margin: 2.33em 0; }
        
        p { display: block; margin: 1em 0; }
        div { display: block; }
        span { display: inline; }
        
        a { color: blue; text-decoration: underline; cursor: pointer; }
        a:visited { color: purple; }
        
        ul, ol { display: block; margin: 1em 0; padding-left: 40px; }
        li { display: list-item; }
        
        table { display: table; border-collapse: separate; }
        tr { display: table-row; }
        td, th { display: table-cell; padding: 1px; }
        th { font-weight: bold; text-align: center; }
        
        input, button, textarea, select { 
            font-family: inherit;
            font-size: inherit;
        }
        
        button { cursor: pointer; }
        
        img { display: inline-block; }
        
        [hidden] { display: none !important; }
    )";
    
    sheet->parseContent(uaCSS);
    addStyleSheet(sheet, StyleOrigin::UserAgent);
}

CSSComputedStyle StyleResolver::computeStyle(DOMElement* element, const CSSComputedStyle* parentStyle) {
    CSSComputedStyle style;
    
    // Start with inherited values from parent
    if (parentStyle) {
        style = CSSComputedStyle::inherit(*parentStyle);
    }
    
    // Collect matching rules
    std::vector<CSSStyleSheet*> sheets;
    for (const auto& entry : stylesheets_) {
        sheets.push_back(entry.sheet.get());
    }
    
    auto matchedRules = cascade_.collectMatchingRules(element, sheets);
    cascade_.sortByCascade(matchedRules);
    
    // Apply rules in cascade order
    for (const auto& matched : matchedRules) {
        applyDeclarations(style, matched.rule->style(), parentStyle);
    }
    
    // Apply inline styles (highest priority)
    if (element->hasAttribute("style")) {
        CSSStyleDeclaration inlineStyle;
        inlineStyle.setCssText(element->getAttribute("style"));
        applyDeclarations(style, &inlineStyle, parentStyle);
    }
    
    return style;
}

const CSSComputedStyle* StyleResolver::getComputedStyle(DOMElement* element) {
    auto it = styleCache_.find(element);
    if (it != styleCache_.end()) {
        return &it->second;
    }
    
    // Compute parent style first
    const CSSComputedStyle* parentStyle = nullptr;
    DOMNode* parentNode = element->parentNode();
    if (parentNode && parentNode->nodeType() == NodeType::Element) {
        parentStyle = getComputedStyle(static_cast<DOMElement*>(parentNode));
    }
    
    auto [inserted, success] = styleCache_.emplace(element, computeStyle(element, parentStyle));
    (void)success;
    return &inserted->second;
}

void StyleResolver::invalidateElement(DOMElement* element) {
    styleCache_.erase(element);
}

void StyleResolver::invalidateAll() {
    styleCache_.clear();
}

void StyleResolver::applyDeclarations(CSSComputedStyle& style, const CSSStyleDeclaration* decl,
                                       const CSSComputedStyle* parentStyle) {
    for (size_t i = 0; i < decl->length(); ++i) {
        std::string prop = decl->item(i);
        std::string value = decl->getPropertyValue(prop);
        applyProperty(style, prop, value, parentStyle);
    }
}

void StyleResolver::applyProperty(CSSComputedStyle& style, const std::string& property,
                                   const std::string& value, const CSSComputedStyle* parentStyle) {
    // Handle inherit, initial, unset
    if (value == "inherit" && parentStyle) {
        // Copy from parent
        return;
    }
    if (value == "initial") {
        // Reset to initial
        return;
    }
    
    // Apply specific properties
    if (property == "display") {
        if (value == "none") style.display = DisplayValue::None;
        else if (value == "block") style.display = DisplayValue::Block;
        else if (value == "inline") style.display = DisplayValue::Inline;
        else if (value == "inline-block") style.display = DisplayValue::InlineBlock;
        else if (value == "flex") style.display = DisplayValue::Flex;
        else if (value == "inline-flex") style.display = DisplayValue::InlineFlex;
        else if (value == "grid") style.display = DisplayValue::Grid;
    }
    else if (property == "position") {
        if (value == "static") style.position = PositionValue::Static;
        else if (value == "relative") style.position = PositionValue::Relative;
        else if (value == "absolute") style.position = PositionValue::Absolute;
        else if (value == "fixed") style.position = PositionValue::Fixed;
        else if (value == "sticky") style.position = PositionValue::Sticky;
    }
    else if (property == "width") style.width = parseLength(value);
    else if (property == "height") style.height = parseLength(value);
    else if (property == "min-width") style.minWidth = parseLength(value);
    else if (property == "min-height") style.minHeight = parseLength(value);
    else if (property == "max-width") style.maxWidth = parseLength(value);
    else if (property == "max-height") style.maxHeight = parseLength(value);
    else if (property == "margin-top") style.marginTop = parseLength(value);
    else if (property == "margin-right") style.marginRight = parseLength(value);
    else if (property == "margin-bottom") style.marginBottom = parseLength(value);
    else if (property == "margin-left") style.marginLeft = parseLength(value);
    else if (property == "padding-top") style.paddingTop = parseLength(value);
    else if (property == "padding-right") style.paddingRight = parseLength(value);
    else if (property == "padding-bottom") style.paddingBottom = parseLength(value);
    else if (property == "padding-left") style.paddingLeft = parseLength(value);
    else if (property == "color") style.color = parseColor(value);
    else if (property == "background-color") style.backgroundColor = parseColor(value);
    else if (property == "font-size") {
        CSSLength len = parseLength(value);
        style.fontSize = len.value;  // Would need proper resolution
    }
    else if (property == "font-family") style.fontFamily = value;
    else if (property == "font-weight") {
        if (value == "normal") style.fontWeight = FontWeight::Normal;
        else if (value == "bold") style.fontWeight = FontWeight::Bold;
        else style.fontWeight = static_cast<FontWeight>(std::stoi(value));
    }
    else if (property == "opacity") style.opacity = std::stof(value);
    else if (property == "z-index") {
        if (value == "auto") {
            style.zIndexAuto = true;
        } else {
            style.zIndex = std::stoi(value);
            style.zIndexAuto = false;
        }
    }
    // ... more properties
}

CSSLength StyleResolver::parseLength(const std::string& value) {
    return CSSLength::parse(value);
}

CSSColor StyleResolver::parseColor(const std::string& value) {
    return CSSColor::parse(value);
}

// =============================================================================
// CSSParser
// =============================================================================

std::unique_ptr<CSSStyleSheet> CSSParser::parse(const std::string& css) {
    pos_ = 0;
    input_ = css;
    
    auto sheet = std::make_unique<CSSStyleSheet>();
    
    while (!eof()) {
        skipWhitespace();
        skipComment();
        skipWhitespace();
        
        if (eof()) break;
        
        auto rule = parseRule();
        if (rule) {
            rule->setParentStyleSheet(sheet.get());
            sheet->cssRules()->add(std::move(rule));
        }
    }
    
    return sheet;
}

void CSSParser::skipWhitespace() {
    while (pos_ < input_.size() && std::isspace(input_[pos_])) {
        pos_++;
    }
}

void CSSParser::skipComment() {
    if (pos_ + 1 < input_.size() && input_[pos_] == '/' && input_[pos_+1] == '*') {
        pos_ += 2;
        while (pos_ + 1 < input_.size()) {
            if (input_[pos_] == '*' && input_[pos_+1] == '/') {
                pos_ += 2;
                return;
            }
            pos_++;
        }
    }
}

char CSSParser::peek() const {
    return pos_ < input_.size() ? input_[pos_] : '\0';
}

char CSSParser::consume() {
    return pos_ < input_.size() ? input_[pos_++] : '\0';
}

bool CSSParser::match(char c) {
    if (peek() == c) {
        consume();
        return true;
    }
    return false;
}

bool CSSParser::eof() const {
    return pos_ >= input_.size();
}

std::unique_ptr<CSSRule> CSSParser::parseRule() {
    skipWhitespace();
    
    if (peek() == '@') {
        consume();
        std::string atKeyword = parseIdentifier();
        
        if (atKeyword == "media") return parseMediaRule();
        if (atKeyword == "font-face") return parseFontFaceRule();
        if (atKeyword == "keyframes" || atKeyword == "-webkit-keyframes") return parseKeyframesRule();
        if (atKeyword == "import") return parseImportRule();
        
        // Skip unknown at-rule
        while (!eof() && peek() != ';' && peek() != '{') consume();
        if (peek() == '{') {
            int depth = 1;
            consume();
            while (!eof() && depth > 0) {
                if (peek() == '{') depth++;
                else if (peek() == '}') depth--;
                consume();
            }
        } else if (peek() == ';') {
            consume();
        }
        return nullptr;
    }
    
    return parseStyleRule();
}

std::unique_ptr<CSSStyleRule> CSSParser::parseStyleRule() {
    auto rule = std::make_unique<CSSStyleRule>();
    
    // Parse selector
    std::string selector = parseSelector_();
    rule->setSelectorText(selector);
    
    skipWhitespace();
    if (!match('{')) return nullptr;
    
    // Parse declarations
    parseDeclarations(rule->style());
    
    match('}');
    
    return rule;
}

std::string CSSParser::parseSelector_() {
    std::string result;
    while (!eof() && peek() != '{') {
        result += consume();
    }
    // Trim
    size_t start = result.find_first_not_of(" \t\n\r");
    size_t end = result.find_last_not_of(" \t\n\r");
    return (start != std::string::npos) ? result.substr(start, end - start + 1) : "";
}

void CSSParser::parseDeclarations(CSSStyleDeclaration* decl) {
    while (!eof() && peek() != '}') {
        skipWhitespace();
        skipComment();
        
        if (peek() == '}') break;
        
        // Property name
        std::string property = parseIdentifier();
        skipWhitespace();
        
        if (!match(':')) {
            // Skip to next declaration
            while (!eof() && peek() != ';' && peek() != '}') consume();
            match(';');
            continue;
        }
        
        skipWhitespace();
        
        // Property value
        std::string value = parsePropertyValue();
        
        // Check for !important
        std::string priority;
        size_t impPos = value.find("!important");
        if (impPos != std::string::npos) {
            priority = "important";
            value = value.substr(0, impPos);
            // Trim
            size_t end = value.find_last_not_of(" \t");
            if (end != std::string::npos) {
                value = value.substr(0, end + 1);
            }
        }
        
        decl->setProperty(property, value, priority);
        
        skipWhitespace();
        match(';');
    }
}

std::string CSSParser::parsePropertyValue() {
    std::string result;
    int parenDepth = 0;
    
    while (!eof()) {
        char c = peek();
        if (c == '(') parenDepth++;
        else if (c == ')') parenDepth--;
        else if ((c == ';' || c == '}') && parenDepth == 0) break;
        
        result += consume();
    }
    
    return result;
}

std::string CSSParser::parseIdentifier() {
    std::string result;
    while (!eof()) {
        char c = peek();
        if (std::isalnum(c) || c == '-' || c == '_') {
            result += consume();
        } else {
            break;
        }
    }
    return result;
}

std::unique_ptr<CSSMediaRule> CSSParser::parseMediaRule() {
    auto rule = std::make_unique<CSSMediaRule>();
    
    skipWhitespace();
    
    // Parse condition
    std::string condition;
    while (!eof() && peek() != '{') {
        condition += consume();
    }
    // Trim
    size_t start = condition.find_first_not_of(" \t\n\r");
    size_t end = condition.find_last_not_of(" \t\n\r");
    if (start != std::string::npos) {
        condition = condition.substr(start, end - start + 1);
    }
    rule->setConditionText(condition);
    
    if (!match('{')) return nullptr;
    
    // Parse nested rules
    while (!eof() && peek() != '}') {
        skipWhitespace();
        skipComment();
        if (peek() == '}') break;
        
        auto nested = parseRule();
        if (nested) {
            nested->setParentRule(rule.get());
            rule->cssRules()->add(std::move(nested));
        }
    }
    
    match('}');
    
    return rule;
}

std::unique_ptr<CSSFontFaceRule> CSSParser::parseFontFaceRule() {
    auto rule = std::make_unique<CSSFontFaceRule>();
    
    skipWhitespace();
    if (!match('{')) return nullptr;
    
    parseDeclarations(rule->style());
    
    match('}');
    
    return rule;
}

std::unique_ptr<CSSKeyframesRule> CSSParser::parseKeyframesRule() {
    auto rule = std::make_unique<CSSKeyframesRule>();
    
    skipWhitespace();
    std::string name = parseIdentifier();
    rule->setName(name);
    
    skipWhitespace();
    if (!match('{')) return nullptr;
    
    // Parse keyframes
    while (!eof() && peek() != '}') {
        skipWhitespace();
        skipComment();
        if (peek() == '}') break;
        
        auto keyframe = std::make_unique<CSSKeyframeRule>();
        
        // Parse key
        std::string key;
        while (!eof() && peek() != '{') {
            key += consume();
        }
        // Trim
        size_t start = key.find_first_not_of(" \t\n\r");
        size_t end = key.find_last_not_of(" \t\n\r");
        if (start != std::string::npos) {
            key = key.substr(start, end - start + 1);
        }
        keyframe->setKeyText(key);
        
        if (!match('{')) break;
        
        parseDeclarations(keyframe->style());
        
        match('}');
        
        rule->cssRules()->add(std::move(keyframe));
    }
    
    match('}');
    
    return rule;
}

std::unique_ptr<CSSImportRule> CSSParser::parseImportRule() {
    auto rule = std::make_unique<CSSImportRule>();
    
    skipWhitespace();
    
    // Skip url() or string
    while (!eof() && peek() != ';') {
        consume();
    }
    
    match(';');
    
    return rule;
}

// =============================================================================
// CSSEngine
// =============================================================================

CSSEngine::CSSEngine() = default;
CSSEngine::~CSSEngine() = default;

void CSSEngine::initialize(DOMDocument* document) {
    document_ = document;
    resolver_.addUserAgentStyleSheet();
}

void CSSEngine::addStyleSheet(const std::string& css, StyleOrigin origin) {
    auto sheet = parser_.parse(css);
    stylesheets_.push_back(std::move(sheet));
    resolver_.addStyleSheet(stylesheets_.back(), origin);
}

void CSSEngine::addStyleSheetFromUrl(const std::string& /*url*/, StyleOrigin /*origin*/) {
    // Would fetch and parse
}

void CSSEngine::computeStyles() {
    resolver_.invalidateAll();
}

const CSSComputedStyle* CSSEngine::getComputedStyle(DOMElement* element) {
    return resolver_.getComputedStyle(element);
}

void CSSEngine::invalidate(DOMElement* element) {
    if (element) {
        resolver_.invalidateElement(element);
    } else {
        resolver_.invalidateAll();
    }
}

bool CSSEngine::supports(const std::string& property, const std::string& /*value*/) {
    // Basic property support check
    static const std::vector<std::string> supportedProperties = {
        "display", "position", "width", "height", "margin", "padding",
        "border", "background", "color", "font", "flex", "grid", "transform",
        "transition", "animation", "opacity", "z-index", "overflow"
    };
    
    for (const auto& p : supportedProperties) {
        if (property.find(p) == 0) return true;
    }
    return false;
}

bool CSSEngine::supportsCondition(const std::string& condition) {
    // Parse @supports condition
    // (property: value) or (property: value) and (...)
    size_t colonPos = condition.find(':');
    if (colonPos == std::string::npos) return false;
    
    size_t start = condition.find('(');
    size_t end = condition.find(')');
    if (start == std::string::npos || end == std::string::npos) return false;
    
    std::string prop = condition.substr(start + 1, colonPos - start - 1);
    std::string val = condition.substr(colonPos + 1, end - colonPos - 1);
    
    // Trim
    auto trim = [](std::string& s) {
        size_t a = s.find_first_not_of(" \t");
        size_t b = s.find_last_not_of(" \t");
        if (a != std::string::npos) s = s.substr(a, b - a + 1);
    };
    trim(prop);
    trim(val);
    
    return supports(prop, val);
}

std::string CSSEngine::escape(const std::string& ident) {
    std::string result;
    for (char c : ident) {
        if (std::isalnum(c) || c == '-' || c == '_') {
            result += c;
        } else {
            result += '\\';
            result += c;
        }
    }
    return result;
}

bool CSSEngine::supportsPropertySyntax(const std::string& /*syntax*/) {
    return true;  // Would validate CSS syntax
}

} // namespace Zepra::WebCore
