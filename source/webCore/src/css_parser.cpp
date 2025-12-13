/**
 * @file css_parser.cpp
 * @brief CSS parsing and style computation implementation
 */

#include "webcore/css_parser.hpp"
#include "webcore/dom.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <regex>

namespace Zepra::WebCore {

// Forward declaration for helper
static bool matchesPartial(const SelectorPart& part, DOMElement* element);

// =============================================================================
// Selector Implementation
// =============================================================================

void Selector::addPart(const SelectorPart& part) {
    parts_.push_back(part);
}

int Selector::specificity() const {
    int a = 0, b = 0, c = 0;
    
    for (const auto& part : parts_) {
        switch (part.type) {
            case SelectorType::Id:
                a++;
                break;
            case SelectorType::Class:
            case SelectorType::Attribute:
            case SelectorType::PseudoClass:
                b++;
                break;
            case SelectorType::Element:
            case SelectorType::PseudoElement:
                c++;
                break;
            default:
                break;
        }
    }
    
    return a * 10000 + b * 100 + c;
}

bool Selector::matches(DOMElement* element) const {
    if (!element || parts_.empty()) return false;
    
    DOMElement* current = element;
    
    for (int i = static_cast<int>(parts_.size()) - 1; i >= 0; i--) {
        const auto& part = parts_[i];
        
        switch (part.type) {
            case SelectorType::Universal:
                break;
                
            case SelectorType::Element:
                if (current->tagName() != part.value) return false;
                break;
                
            case SelectorType::Class: {
                auto classList = current->classList();
                if (std::find(classList.begin(), classList.end(), part.value) == classList.end()) {
                    return false;
                }
                break;
            }
                
            case SelectorType::Id:
                if (current->id() != part.value) return false;
                break;
                
            case SelectorType::Attribute:
                if (!current->hasAttribute(part.attribute)) return false;
                if (!part.attrValue.empty()) {
                    std::string attrVal = current->getAttribute(part.attribute);
                    if (part.attrOperator == "=") {
                        if (attrVal != part.attrValue) return false;
                    } else if (part.attrOperator == "~=") {
                        std::istringstream iss(attrVal);
                        std::string word;
                        bool found = false;
                        while (iss >> word) {
                            if (word == part.attrValue) { found = true; break; }
                        }
                        if (!found) return false;
                    } else if (part.attrOperator == "^=") {
                        if (attrVal.find(part.attrValue) != 0) return false;
                    } else if (part.attrOperator == "$=") {
                        if (attrVal.length() < part.attrValue.length()) return false;
                        if (attrVal.substr(attrVal.length() - part.attrValue.length()) != part.attrValue) return false;
                    } else if (part.attrOperator == "*=") {
                        if (attrVal.find(part.attrValue) == std::string::npos) return false;
                    }
                }
                break;
                
            case SelectorType::Descendant: {
                DOMNode* ancestor = current->parentNode();
                while (ancestor && ancestor->nodeType() == NodeType::Element) {
                    current = static_cast<DOMElement*>(ancestor);
                    if (i > 0 && matchesPartial(parts_[i-1], current)) {
                        i--;
                        break;
                    }
                    ancestor = ancestor->parentNode();
                }
                if (!ancestor) return false;
                break;
            }
                
            case SelectorType::Child: {
                DOMNode* parent = current->parentNode();
                if (!parent || parent->nodeType() != NodeType::Element) return false;
                current = static_cast<DOMElement*>(parent);
                break;
            }
                
            default:
                break;
        }
    }
    
    return true;
}

// Helper for descendant matching
static bool matchesPartial(const SelectorPart& part, DOMElement* element) {
    switch (part.type) {
        case SelectorType::Universal:
            return true;
        case SelectorType::Element:
            return element->tagName() == part.value;
        case SelectorType::Class: {
            auto classList = element->classList();
            return std::find(classList.begin(), classList.end(), part.value) != classList.end();
        }
        case SelectorType::Id:
            return element->id() == part.value;
        default:
            return false;
    }
}

// =============================================================================
// CSSValue Implementation
// =============================================================================

CSSValue CSSValue::makeKeyword(const std::string& kw) {
    CSSValue v;
    v.type = Type::Keyword;
    v.keyword = kw;
    return v;
}

CSSValue CSSValue::length(float value, const std::string& unit) {
    CSSValue v;
    v.type = Type::Length;
    v.number = value;
    v.unit = unit;
    return v;
}

CSSValue CSSValue::colorValue(const Color& c) {
    CSSValue v;
    v.type = Type::Color;
    v.color = c;
    return v;
}

// =============================================================================
// CSSRule Implementation
// =============================================================================

void CSSRule::addSelector(const Selector& selector) {
    selectors_.push_back(selector);
}

void CSSRule::addDeclaration(const CSSDeclaration& decl) {
    declarations_.push_back(decl);
}

// =============================================================================
// Stylesheet Implementation
// =============================================================================

void Stylesheet::addRule(std::unique_ptr<CSSRule> rule) {
    rules_.push_back(std::move(rule));
}

std::vector<const CSSRule*> Stylesheet::matchingRules(DOMElement* element) const {
    std::vector<const CSSRule*> matches;
    
    for (const auto& rule : rules_) {
        for (const auto& selector : rule->selectors()) {
            if (selector.matches(element)) {
                matches.push_back(rule.get());
                break;
            }
        }
    }
    
    return matches;
}

// =============================================================================
// CSSParser Implementation
// =============================================================================

static void trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

std::unique_ptr<Stylesheet> CSSParser::parse(const std::string& css) {
    auto stylesheet = std::make_unique<Stylesheet>();
    input_ = css;
    pos_ = 0;
    
    while (pos_ < input_.size()) {
        // Skip whitespace and comments
        while (pos_ < input_.size() && std::isspace(input_[pos_])) pos_++;
        
        if (pos_ >= input_.size()) break;
        
        // Skip comments
        if (pos_ + 1 < input_.size() && input_[pos_] == '/' && input_[pos_+1] == '*') {
            pos_ += 2;
            while (pos_ + 1 < input_.size() && !(input_[pos_] == '*' && input_[pos_+1] == '/')) {
                pos_++;
            }
            pos_ += 2;
            continue;
        }
        
        // Find selector (until '{')
        size_t bracePos = input_.find('{', pos_);
        if (bracePos == std::string::npos) break;
        
        std::string selectorStr = input_.substr(pos_, bracePos - pos_);
        trim(selectorStr);
        pos_ = bracePos + 1;
        
        // Find declarations (until '}')
        size_t endBrace = input_.find('}', pos_);
        if (endBrace == std::string::npos) break;
        
        std::string declBlock = input_.substr(pos_, endBrace - pos_);
        pos_ = endBrace + 1;
        
        // Parse rule
        auto rule = std::make_unique<CSSRule>();
        
        // Parse selectors (comma-separated)
        std::istringstream selectorStream(selectorStr);
        std::string singleSelector;
        while (std::getline(selectorStream, singleSelector, ',')) {
            trim(singleSelector);
            if (!singleSelector.empty()) {
                rule->addSelector(parseSelector(singleSelector));
            }
        }
        
        // Parse declarations (semicolon-separated)
        std::istringstream declStream(declBlock);
        std::string singleDecl;
        while (std::getline(declStream, singleDecl, ';')) {
            trim(singleDecl);
            if (!singleDecl.empty()) {
                rule->addDeclaration(parseDeclaration(singleDecl));
            }
        }
        
        stylesheet->addRule(std::move(rule));
    }
    
    return stylesheet;
}

std::vector<CSSDeclaration> CSSParser::parseInlineStyle(const std::string& style) {
    std::vector<CSSDeclaration> declarations;
    
    std::istringstream stream(style);
    std::string decl;
    while (std::getline(stream, decl, ';')) {
        trim(decl);
        if (!decl.empty()) {
            declarations.push_back(parseDeclaration(decl));
        }
    }
    
    return declarations;
}

Selector CSSParser::parseSelector(const std::string& selectorStr) {
    Selector selector;
    std::string s = selectorStr;
    size_t i = 0;
    
    while (i < s.size()) {
        SelectorPart part;
        
        // Skip whitespace
        while (i < s.size() && std::isspace(s[i])) i++;
        if (i >= s.size()) break;
        
        // Check for combinators
        if (s[i] == '>') {
            part.type = SelectorType::Child;
            part.value = ">";
            selector.addPart(part);
            i++;
            continue;
        } else if (s[i] == '+') {
            part.type = SelectorType::Adjacent;
            part.value = "+";
            selector.addPart(part);
            i++;
            continue;
        } else if (s[i] == '~') {
            part.type = SelectorType::Sibling;
            part.value = "~";
            selector.addPart(part);
            i++;
            continue;
        }
        
        // Universal selector
        if (s[i] == '*') {
            part.type = SelectorType::Universal;
            part.value = "*";
            selector.addPart(part);
            i++;
            continue;
        }
        
        // ID selector
        if (s[i] == '#') {
            i++;
            size_t start = i;
            while (i < s.size() && (std::isalnum(s[i]) || s[i] == '_' || s[i] == '-')) i++;
            part.type = SelectorType::Id;
            part.value = s.substr(start, i - start);
            selector.addPart(part);
            continue;
        }
        
        // Class selector
        if (s[i] == '.') {
            i++;
            size_t start = i;
            while (i < s.size() && (std::isalnum(s[i]) || s[i] == '_' || s[i] == '-')) i++;
            part.type = SelectorType::Class;
            part.value = s.substr(start, i - start);
            selector.addPart(part);
            continue;
        }
        
        // Attribute selector
        if (s[i] == '[') {
            i++;
            size_t attrStart = i;
            while (i < s.size() && s[i] != ']' && s[i] != '=' && s[i] != '~' && s[i] != '^' && s[i] != '$' && s[i] != '*') i++;
            part.attribute = s.substr(attrStart, i - attrStart);
            trim(part.attribute);
            
            // Check for operator
            if (i < s.size() && s[i] != ']') {
                if (s[i] == '=') {
                    part.attrOperator = "=";
                    i++;
                } else if (i + 1 < s.size() && s[i+1] == '=') {
                    part.attrOperator = s.substr(i, 2);
                    i += 2;
                }
                
                // Get value
                while (i < s.size() && std::isspace(s[i])) i++;
                if (i < s.size() && (s[i] == '"' || s[i] == '\'')) {
                    char quote = s[i];
                    i++;
                    size_t valStart = i;
                    while (i < s.size() && s[i] != quote) i++;
                    part.attrValue = s.substr(valStart, i - valStart);
                    i++;
                } else {
                    size_t valStart = i;
                    while (i < s.size() && s[i] != ']') i++;
                    part.attrValue = s.substr(valStart, i - valStart);
                    trim(part.attrValue);
                }
            }
            
            while (i < s.size() && s[i] != ']') i++;
            if (i < s.size()) i++;
            
            part.type = SelectorType::Attribute;
            selector.addPart(part);
            continue;
        }
        
        // Pseudo-class or pseudo-element
        if (s[i] == ':') {
            i++;
            if (i < s.size() && s[i] == ':') {
                i++;
                part.type = SelectorType::PseudoElement;
            } else {
                part.type = SelectorType::PseudoClass;
            }
            size_t start = i;
            while (i < s.size() && (std::isalnum(s[i]) || s[i] == '-')) i++;
            part.value = s.substr(start, i - start);
            selector.addPart(part);
            continue;
        }
        
        // Element selector
        if (std::isalnum(s[i])) {
            size_t start = i;
            while (i < s.size() && (std::isalnum(s[i]) || s[i] == '-')) i++;
            part.type = SelectorType::Element;
            part.value = s.substr(start, i - start);
            selector.addPart(part);
            continue;
        }
        
        // Unknown, skip
        i++;
    }
    
    return selector;
}

CSSDeclaration CSSParser::parseDeclaration(const std::string& declStr) {
    CSSDeclaration decl;
    
    size_t colonPos = declStr.find(':');
    if (colonPos == std::string::npos) {
        decl.property = declStr;
        return decl;
    }
    
    decl.property = declStr.substr(0, colonPos);
    trim(decl.property);
    std::transform(decl.property.begin(), decl.property.end(), decl.property.begin(), ::tolower);
    
    std::string valueStr = declStr.substr(colonPos + 1);
    
    // Check for !important
    size_t importantPos = valueStr.find("!important");
    if (importantPos != std::string::npos) {
        decl.important = true;
        valueStr = valueStr.substr(0, importantPos);
    }
    
    trim(valueStr);
    decl.value = parseValue(valueStr);
    
    return decl;
}

CSSValue CSSParser::parseValue(const std::string& value) {
    std::string v = value;
    trim(v);
    
    if (v.empty()) return CSSValue::makeKeyword("initial");
    
    // Keywords
    if (v == "auto" || v == "none" || v == "inherit" || v == "initial" || 
        v == "block" || v == "inline" || v == "inline-block" || v == "flex" || v == "grid" ||
        v == "hidden" || v == "visible" || v == "scroll" ||
        v == "left" || v == "right" || v == "center" ||
        v == "top" || v == "bottom" || v == "middle" ||
        v == "bold" || v == "normal" || v == "italic" ||
        v == "underline" || v == "solid" || v == "dashed" || v == "dotted") {
        return CSSValue::makeKeyword(v);
    }
    
    // Color hex
    if (v[0] == '#') {
        return CSSValue::colorValue(parseColor(v));
    }
    
    // Color named
    if (v == "black") return CSSValue::colorValue(Color::black());
    if (v == "white") return CSSValue::colorValue(Color::white());
    if (v == "red") return CSSValue::colorValue(Color::red());
    if (v == "green") return CSSValue::colorValue(Color::green());
    if (v == "blue") return CSSValue::colorValue(Color::blue());
    if (v == "orange") return CSSValue::colorValue({255, 165, 0, 255});
    if (v == "transparent") return CSSValue::colorValue(Color::transparent());
    
    // Color rgb/rgba
    if (v.substr(0, 4) == "rgb(" || v.substr(0, 5) == "rgba(") {
        return CSSValue::colorValue(parseColor(v));
    }
    
    // Number with unit
    size_t unitStart = 0;
    while (unitStart < v.size() && (std::isdigit(v[unitStart]) || v[unitStart] == '.' || v[unitStart] == '-')) {
        unitStart++;
    }
    
    if (unitStart > 0) {
        float num = std::strtof(v.c_str(), nullptr);
        std::string unit = v.substr(unitStart);
        
        if (unit.empty() || unit == "px" || unit == "em" || unit == "rem" || 
            unit == "%" || unit == "vh" || unit == "vw" || unit == "pt") {
            return CSSValue::length(num, unit.empty() ? "px" : unit);
        }
    }
    
    return CSSValue::makeKeyword(v);
}

Color CSSParser::parseColor(const std::string& colorStr) {
    std::string s = colorStr;
    trim(s);
    
    // Hex color
    if (s[0] == '#') {
        if (s.length() == 4) {
            // #RGB -> #RRGGBB
            uint8_t r = static_cast<uint8_t>(std::strtol(std::string(2, s[1]).c_str(), nullptr, 16));
            uint8_t g = static_cast<uint8_t>(std::strtol(std::string(2, s[2]).c_str(), nullptr, 16));
            uint8_t b = static_cast<uint8_t>(std::strtol(std::string(2, s[3]).c_str(), nullptr, 16));
            return {r, g, b, 255};
        } else if (s.length() == 7) {
            // #RRGGBB
            uint8_t r = static_cast<uint8_t>(std::strtol(s.substr(1, 2).c_str(), nullptr, 16));
            uint8_t g = static_cast<uint8_t>(std::strtol(s.substr(3, 2).c_str(), nullptr, 16));
            uint8_t b = static_cast<uint8_t>(std::strtol(s.substr(5, 2).c_str(), nullptr, 16));
            return {r, g, b, 255};
        } else if (s.length() == 9) {
            // #RRGGBBAA
            uint8_t r = static_cast<uint8_t>(std::strtol(s.substr(1, 2).c_str(), nullptr, 16));
            uint8_t g = static_cast<uint8_t>(std::strtol(s.substr(3, 2).c_str(), nullptr, 16));
            uint8_t b = static_cast<uint8_t>(std::strtol(s.substr(5, 2).c_str(), nullptr, 16));
            uint8_t a = static_cast<uint8_t>(std::strtol(s.substr(7, 2).c_str(), nullptr, 16));
            return {r, g, b, a};
        }
    }
    
    // rgb() / rgba()
    if (s.substr(0, 4) == "rgb(" || s.substr(0, 5) == "rgba(") {
        size_t start = s.find('(') + 1;
        size_t end = s.find(')');
        std::string values = s.substr(start, end - start);
        
        std::vector<float> nums;
        std::istringstream iss(values);
        std::string token;
        while (std::getline(iss, token, ',')) {
            trim(token);
            // Handle percentage
            if (!token.empty() && token.back() == '%') {
                token.pop_back();
                nums.push_back(std::strtof(token.c_str(), nullptr) * 2.55f);
            } else {
                nums.push_back(std::strtof(token.c_str(), nullptr));
            }
        }
        
        if (nums.size() >= 3) {
            uint8_t r = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, nums[0])));
            uint8_t g = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, nums[1])));
            uint8_t b = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, nums[2])));
            uint8_t a = nums.size() >= 4 ? static_cast<uint8_t>(nums[3] * 255) : 255;
            return {r, g, b, a};
        }
    }
    
    return Color::black();
}

// =============================================================================
// StyleResolver Implementation
// =============================================================================

void StyleResolver::addStylesheet(std::shared_ptr<Stylesheet> stylesheet) {
    stylesheets_.push_back(stylesheet);
}

void StyleResolver::addUserAgentStylesheet() {
    CSSParser parser;
    auto ua = parser.parse(R"(
        html, body { display: block; margin: 0; padding: 0; }
        div, p, section, article, header, footer, nav, main, aside { display: block; }
        span, a, strong, em, b, i { display: inline; }
        h1 { display: block; font-size: 32px; font-weight: bold; margin: 0.67em 0; }
        h2 { display: block; font-size: 24px; font-weight: bold; margin: 0.83em 0; }
        h3 { display: block; font-size: 18.72px; font-weight: bold; margin: 1em 0; }
        h4 { display: block; font-size: 16px; font-weight: bold; margin: 1.33em 0; }
        h5 { display: block; font-size: 13.28px; font-weight: bold; margin: 1.67em 0; }
        h6 { display: block; font-size: 10.72px; font-weight: bold; margin: 2.33em 0; }
        p { display: block; margin: 1em 0; }
        ul, ol { display: block; margin: 1em 0; padding-left: 40px; }
        li { display: list-item; }
        a { color: blue; text-decoration: underline; }
        strong, b { font-weight: bold; }
        em, i { font-style: italic; }
        img { display: inline-block; }
        table { display: table; border-collapse: separate; }
        tr { display: table-row; }
        td, th { display: table-cell; padding: 1px; }
        input, button, select, textarea { display: inline-block; }
    )");
    stylesheets_.insert(stylesheets_.begin(), std::move(ua));
}

ComputedStyle StyleResolver::computeStyle(DOMElement* element, const ComputedStyle* parentStyle) {
    ComputedStyle style;
    
    // Inheritance
    if (parentStyle) {
        style.color = parentStyle->color;
        style.fontFamily = parentStyle->fontFamily;
        style.fontSize = parentStyle->fontSize;
        style.fontBold = parentStyle->fontBold;
        style.fontItalic = parentStyle->fontItalic;
    }
    
    // Collect all matching rules with specificity
    std::vector<std::pair<int, const CSSDeclaration*>> declarations;
    
    // Inline styles (highest specificity)
    std::vector<CSSDeclaration> inlineDecls;
    std::string inlineStyle = element->getAttribute("style");
    if (!inlineStyle.empty()) {
        CSSParser parser;
        inlineDecls = parser.parseInlineStyle(inlineStyle);
        for (const auto& decl : inlineDecls) {
            int priority = 1000000 + (decl.important ? 200000 : 0);
            declarations.push_back({priority, &decl});
        }
    }
    
    for (const auto& stylesheet : stylesheets_) {
        auto rules = stylesheet->matchingRules(element);
        for (const auto* rule : rules) {
            int maxSpec = 0;
            for (const auto& sel : rule->selectors()) {
                if (sel.matches(element)) {
                    maxSpec = std::max(maxSpec, sel.specificity());
                }
            }
            for (const auto& decl : rule->declarations()) {
                int priority = maxSpec + (decl.important ? 100000 : 0);
                declarations.push_back({priority, &decl});
            }
        }
    }
    
    // Sort by specificity
    std::sort(declarations.begin(), declarations.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });
    
    // Apply declarations
    for (const auto& [_, decl] : declarations) {
        applyDeclaration(style, *decl, parentStyle);
    }
    
    return style;
}

void StyleResolver::applyDeclaration(ComputedStyle& style, const CSSDeclaration& decl, const ComputedStyle* parentStyle) {
    const auto& prop = decl.property;
    const auto& val = decl.value;
    
    // Determine parent values
    float parentFontSize = parentStyle ? parentStyle->fontSize : 16.0f;
    
    // Handle 'inherit' keyword globally
    if (val.keyword == "inherit") {
        if (!parentStyle) return; // Use default/initial if no parent
        
        if (prop == "color") style.color = parentStyle->color;
        else if (prop == "font-family") style.fontFamily = parentStyle->fontFamily;
        else if (prop == "font-size") style.fontSize = parentStyle->fontSize;
        else if (prop == "font-weight") style.fontBold = parentStyle->fontBold;
        else if (prop == "font-style") style.fontItalic = parentStyle->fontItalic;
        else if (prop == "display") style.display = parentStyle->display;
        else if (prop == "position") style.position = parentStyle->position;
        else if (prop == "visibility") { /* TODO */ }
        else if (prop == "margin") style.margin = parentStyle->margin;
        else if (prop == "margin-top") style.margin.top = parentStyle->margin.top;
        else if (prop == "margin-right") style.margin.right = parentStyle->margin.right;
        else if (prop == "margin-bottom") style.margin.bottom = parentStyle->margin.bottom;
        else if (prop == "margin-left") style.margin.left = parentStyle->margin.left;
        else if (prop == "padding") style.padding = parentStyle->padding;
        else if (prop == "padding-top") style.padding.top = parentStyle->padding.top;
        else if (prop == "padding-right") style.padding.right = parentStyle->padding.right;
        else if (prop == "padding-bottom") style.padding.bottom = parentStyle->padding.bottom;
        else if (prop == "padding-left") style.padding.left = parentStyle->padding.left;
        else if (prop == "background-color") style.backgroundColor = parentStyle->backgroundColor;
        else if (prop == "width") { style.width = parentStyle->width; style.autoWidth = parentStyle->autoWidth; }
        else if (prop == "height") { style.height = parentStyle->height; style.autoHeight = parentStyle->autoHeight; }
        return;
    }
    
    // Display
    if (prop == "display") {
        if (val.keyword == "block") style.display = Display::Block;
        else if (val.keyword == "inline") style.display = Display::Inline;
        else if (val.keyword == "inline-block") style.display = Display::InlineBlock;
        else if (val.keyword == "flex") style.display = Display::Flex;
        else if (val.keyword == "grid") style.display = Display::Grid;
        else if (val.keyword == "none") style.display = Display::None;
    }
    
    // Sizing
    else if (prop == "width") {
        if (val.type == CSSValue::Type::Length) {
            style.width = resolveLength(val, style.fontSize, parentFontSize);
            style.autoWidth = false;
        } else if (val.keyword == "auto") {
            style.autoWidth = true;
        }
    }
    else if (prop == "height") {
        if (val.type == CSSValue::Type::Length) {
            style.height = resolveLength(val, style.fontSize, parentFontSize);
            style.autoHeight = false;
        } else if (val.keyword == "auto") {
            style.autoHeight = true;
        }
    }
    
    // Margins
    else if (prop == "margin") {
        if (val.type == CSSValue::Type::Length) {
            float v = resolveLength(val, style.fontSize, parentFontSize);
            style.margin = {v, v, v, v};
        }
    }
    else if (prop == "margin-top" && val.type == CSSValue::Type::Length) {
        style.margin.top = resolveLength(val, style.fontSize, parentFontSize);
    }
    else if (prop == "margin-right" && val.type == CSSValue::Type::Length) {
        style.margin.right = resolveLength(val, style.fontSize, parentFontSize);
    }
    else if (prop == "margin-bottom" && val.type == CSSValue::Type::Length) {
        style.margin.bottom = resolveLength(val, style.fontSize, parentFontSize);
    }
    else if (prop == "margin-left" && val.type == CSSValue::Type::Length) {
        style.margin.left = resolveLength(val, style.fontSize, parentFontSize);
    }
    
    // Padding
    else if (prop == "padding") {
        if (val.type == CSSValue::Type::Length) {
            float v = resolveLength(val, style.fontSize, parentFontSize);
            style.padding = {v, v, v, v};
        }
    }
    else if (prop == "padding-top" && val.type == CSSValue::Type::Length) {
        style.padding.top = resolveLength(val, style.fontSize, parentFontSize);
    }
    else if (prop == "padding-right" && val.type == CSSValue::Type::Length) {
        style.padding.right = resolveLength(val, style.fontSize, parentFontSize);
    }
    else if (prop == "padding-bottom" && val.type == CSSValue::Type::Length) {
        style.padding.bottom = resolveLength(val, style.fontSize, parentFontSize);
    }
    else if (prop == "padding-left" && val.type == CSSValue::Type::Length) {
        style.padding.left = resolveLength(val, style.fontSize, parentFontSize);
    }
    
    // Colors
    else if (prop == "color" && val.type == CSSValue::Type::Color) {
        style.color = val.color;
    }
    else if (prop == "background-color" && val.type == CSSValue::Type::Color) {
        style.backgroundColor = val.color;
    }
    else if (prop == "border-color" && val.type == CSSValue::Type::Color) {
        style.borderColor = val.color;
    }
    
    // Font
    else if (prop == "font-size") {
        if (val.type == CSSValue::Type::Length) {
            // For font-size, em is relative to parent font size
            style.fontSize = resolveLength(val, parentFontSize, parentFontSize);
        }
    }
    else if (prop == "font-family") {
        style.fontFamily = val.keyword;
    }
    else if (prop == "font-weight") {
        style.fontBold = (val.keyword == "bold" || val.number >= 700);
    }
    else if (prop == "font-style") {
        style.fontItalic = (val.keyword == "italic" || val.keyword == "oblique");
    }
    
    // Overflow
    else if (prop == "overflow") {
        if (val.keyword == "hidden") style.overflow = ComputedStyle::Overflow::Hidden;
        else if (val.keyword == "scroll") style.overflow = ComputedStyle::Overflow::Scroll;
        else if (val.keyword == "auto") style.overflow = ComputedStyle::Overflow::Auto;
        else style.overflow = ComputedStyle::Overflow::Visible;
    }
    
    // Z-index
    else if (prop == "z-index" && val.type == CSSValue::Type::Number) {
        style.zIndex = static_cast<int>(val.number);
    }
}

float StyleResolver::resolveLength(const CSSValue& value, float fontSize, float parentFontSize) {
    if (value.type != CSSValue::Type::Length) return 0;
    
    if (value.unit.empty() || value.unit == "px") {
        return value.number;
    } else if (value.unit == "em") {
        return value.number * fontSize;
    } else if (value.unit == "rem") {
        return value.number * 16;  // Assume root is 16px
    } else if (value.unit == "%") {
        return value.number * parentFontSize / 100.0f;
    } else if (value.unit == "pt") {
        return value.number * 1.333f;
    }
    
    return value.number;
}

} // namespace Zepra::WebCore
