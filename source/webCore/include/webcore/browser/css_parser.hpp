#pragma once

/**
 * @file css_parser.hpp
 * @brief CSS parsing and style computation
 */

#include "render_tree.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief CSS selector types
 */
enum class SelectorType {
    Universal,      // *
    Element,        // div, p, span
    Class,          // .className
    Id,             // #idName
    Attribute,      // [attr=value]
    Descendant,     // div p
    Child,          // div > p
    Adjacent,       // div + p
    Sibling,        // div ~ p
    PseudoClass,    // :hover, :active
    PseudoElement   // ::before, ::after
};

/**
 * @brief CSS selector component
 */
struct SelectorPart {
    SelectorType type;
    std::string value;
    std::string attribute;
    std::string attrOperator;
    std::string attrValue;
};

/**
 * @brief CSS selector (chain of parts)
 */
class Selector {
public:
    void addPart(const SelectorPart& part);
    const std::vector<SelectorPart>& parts() const { return parts_; }
    
    // Specificity (a, b, c, d)
    int specificity() const;
    
    // Match against element
    bool matches(class DOMElement* element) const;
    
private:
    std::vector<SelectorPart> parts_;
};

/**
 * @brief CSS property value
 */
struct CSSValue {
    enum class Type {
        Keyword,    // auto, none, inherit
        Length,     // 10px, 2em, 50%
        Color,      // #fff, rgb(), rgba()
        Number,     // 0, 1, 2
        String,     // "text"
        Function,   // calc(), url()
        List        // multiple values
    } type;
    
    std::string keyword;
    float number = 0;
    std::string unit;
    Color color;
    std::vector<CSSValue> list;
    
    static CSSValue makeKeyword(const std::string& kw);
    static CSSValue length(float value, const std::string& unit);
    static CSSValue colorValue(const Color& c);
};

/**
 * @brief CSS declaration (property: value)
 */
struct CSSDeclaration {
    std::string property;
    CSSValue value;
    bool important = false;
};

/**
 * @brief CSS rule (selector + declarations)
 */
class CSSRule {
public:
    void addSelector(const Selector& selector);
    void addDeclaration(const CSSDeclaration& decl);
    
    const std::vector<Selector>& selectors() const { return selectors_; }
    const std::vector<CSSDeclaration>& declarations() const { return declarations_; }
    
private:
    std::vector<Selector> selectors_;
    std::vector<CSSDeclaration> declarations_;
};

/**
 * @brief CSS stylesheet
 */
class Stylesheet {
public:
    void addRule(std::unique_ptr<CSSRule> rule);
    const std::vector<std::unique_ptr<CSSRule>>& rules() const { return rules_; }
    
    // Find rules matching an element
    std::vector<const CSSRule*> matchingRules(class DOMElement* element) const;
    
private:
    std::vector<std::unique_ptr<CSSRule>> rules_;
};

/**
 * @brief CSS parser
 */
class CSSParser {
public:
    std::unique_ptr<Stylesheet> parse(const std::string& css);
    
    // Parse inline style
    std::vector<CSSDeclaration> parseInlineStyle(const std::string& style);
    
    // Parse single value
    CSSValue parseValue(const std::string& value);
    
private:
    Selector parseSelector(const std::string& selectorStr);
    CSSDeclaration parseDeclaration(const std::string& declStr);
    Color parseColor(const std::string& colorStr);
    
    size_t pos_ = 0;
    std::string input_;
};

/**
 * @brief Style resolver - computes final styles for elements
 */
class StyleResolver {
public:
    void addStylesheet(std::shared_ptr<Stylesheet> stylesheet);
    void addUserAgentStylesheet();
    
    // Compute style for element
    ComputedStyle computeStyle(class DOMElement* element, const ComputedStyle* parentStyle = nullptr);
    
private:
    void applyDeclaration(ComputedStyle& style, const CSSDeclaration& decl, const ComputedStyle* parentStyle);
    float resolveLength(const CSSValue& value, float fontSize, float parentFontSize);
    
    std::vector<std::shared_ptr<Stylesheet>> stylesheets_;
};

} // namespace Zepra::WebCore
