/**
 * @file css_selector.hpp
 * @brief CSS Selector parsing and matching
 *
 * @see https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Selectors
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace Zepra::WebCore {

// Forward declarations
class DOMElement;

/**
 * @brief Selector combinator types
 */
enum class SelectorCombinator {
    None,           // No combinator (simple selector)
    Descendant,     // Space: A B
    Child,          // >: A > B
    NextSibling,    // +: A + B
    SubsequentSibling, // ~: A ~ B
    Column,         // ||: A || B
};

/**
 * @brief Pseudo-class types
 */
enum class PseudoClassType {
    None,
    // Tree-structural
    Root,
    Empty,
    FirstChild,
    LastChild,
    OnlyChild,
    FirstOfType,
    LastOfType,
    OnlyOfType,
    NthChild,
    NthLastChild,
    NthOfType,
    NthLastOfType,
    // User action
    Hover,
    Active,
    Focus,
    FocusVisible,
    FocusWithin,
    // Input
    Enabled,
    Disabled,
    Checked,
    Indeterminate,
    Valid,
    Invalid,
    Required,
    Optional,
    ReadOnly,
    ReadWrite,
    PlaceholderShown,
    Default,
    // Link
    Link,
    Visited,
    AnyLink,
    Target,
    // Negation
    Not,
    Is,
    Where,
    Has,
};

/**
 * @brief Pseudo-element types
 */
enum class PseudoElementType {
    None,
    Before,
    After,
    FirstLine,
    FirstLetter,
    Selection,
    Placeholder,
    Marker,
    Backdrop,
};

/**
 * @brief Attribute selector operator
 */
enum class AttributeOperator {
    Exists,         // [attr]
    Equals,         // [attr=value]
    BeginsWith,     // [attr^=value]
    EndsWith,       // [attr$=value]
    Contains,       // [attr*=value]
    WhitespaceSeparated, // [attr~=value]
    HyphenSeparated,     // [attr|=value]
};

/**
 * @brief Attribute selector
 */
struct AttributeSelector {
    std::string name;
    AttributeOperator op = AttributeOperator::Exists;
    std::string value;
    bool caseInsensitive = false;
};

/**
 * @brief Nth expression (An+B)
 */
struct NthExpression {
    int a = 0;      // Coefficient
    int b = 0;      // Offset
    std::string ofSelector;  // For :nth-child(An+B of S)

    static NthExpression parse(const std::string& expr);
    bool matches(int index) const;
};

/**
 * @brief Simple selector (no combinators)
 */
class SimpleSelector {
public:
    SimpleSelector() = default;

    /// Type selector (element name or *)
    std::string typeSelector;
    bool isUniversal = false;

    /// Namespace
    std::optional<std::string> namespacePrefix;

    /// ID selector (#id)
    std::string idSelector;

    /// Class selectors (.class)
    std::vector<std::string> classSelectors;

    /// Attribute selectors ([attr])
    std::vector<AttributeSelector> attributeSelectors;

    /// Pseudo-classes (:hover, :nth-child())
    struct PseudoClass {
        PseudoClassType type;
        std::string argument;
        NthExpression nth;
        std::vector<class Selector> selectorList;  // For :not(), :is(), :has()
    };
    std::vector<PseudoClass> pseudoClasses;

    /// Pseudo-element (::before)
    PseudoElementType pseudoElement = PseudoElementType::None;

    /// Match against element
    bool matches(DOMElement* element) const;

    /// Calculate specificity contribution
    void addSpecificity(int& a, int& b, int& c) const;
};

/**
 * @brief Compound selector (chain of simple selectors with combinators)
 */
class CompoundSelector {
public:
    SimpleSelector base;
    SelectorCombinator combinator = SelectorCombinator::None;

    bool matches(DOMElement* element) const;
};

/**
 * @brief Complete CSS selector (comma-separated list)
 */
class Selector {
public:
    Selector() = default;

    /// Parse selector string
    static Selector parse(const std::string& selectorText);

    /// Selector text
    std::string selectorText() const { return selectorText_; }

    /// Compound selectors (connected by combinators)
    std::vector<CompoundSelector> compounds;

    /// Match against element
    bool matches(DOMElement* element) const;

    /// Calculate specificity (a, b, c)
    struct Specificity {
        int a = 0;  // ID selectors
        int b = 0;  // Class, attribute, pseudo-class
        int c = 0;  // Type, pseudo-element
        
        bool operator<(const Specificity& other) const;
        bool operator>(const Specificity& other) const;
        bool operator==(const Specificity& other) const;
    };
    Specificity specificity() const;

private:
    std::string selectorText_;
};

/**
 * @brief Selector parser
 */
class SelectorParser {
public:
    Selector parse(const std::string& input);
    std::vector<Selector> parseList(const std::string& input);

private:
    size_t pos_ = 0;
    std::string input_;

    void skipWhitespace();
    char peek() const;
    char consume();
    bool match(char c);
    bool match(const std::string& str);

    Selector parseSelector();
    CompoundSelector parseCompoundSelector();
    SimpleSelector parseSimpleSelector();
    AttributeSelector parseAttributeSelector();
    SimpleSelector::PseudoClass parsePseudoClass();
    PseudoElementType parsePseudoElement();
    std::string parseIdentifier();
    std::string parseString();
};

/**
 * @brief Selector matching engine
 */
class SelectorMatcher {
public:
    /// Match selector against element
    bool matches(const Selector& selector, DOMElement* element);

    /// Find all matching elements in subtree
    std::vector<DOMElement*> querySelectorAll(DOMElement* root, const Selector& selector);

    /// Find first matching element
    DOMElement* querySelector(DOMElement* root, const Selector& selector);

private:
    bool matchesSimple(const SimpleSelector& selector, DOMElement* element);
    bool matchesCompound(const std::vector<CompoundSelector>& compounds, 
                         size_t index, DOMElement* element);
    bool matchesPseudoClass(const SimpleSelector::PseudoClass& pc, DOMElement* element);
};

} // namespace Zepra::WebCore
