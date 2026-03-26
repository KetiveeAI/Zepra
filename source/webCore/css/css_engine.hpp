/**
 * @file css_engine.hpp
 * @brief CSS cascade, style resolution, and engine
 *
 * @see https://developer.mozilla.org/en-US/docs/Web/CSS/Cascade
 */

#pragma once

#include "css/css_style_sheet.hpp"
#include "css/css_style_declaration.hpp"
#include "css/css_rule.hpp"
#include "css/css_selector.hpp"
#include "css/css_computed_style.hpp"
#include <memory>
#include <vector>
#include <unordered_map>

namespace Zepra::WebCore {

// Forward declarations
class DOMDocument;
class DOMElement;

/**
 * @brief Style origin (cascade layer)
 */
enum class StyleOrigin {
    UserAgent,      // Browser default styles
    User,           // User preferences
    Author,         // Page stylesheets
    AuthorImportant,
    UserImportant,
    UserAgentImportant,
    Animation,
    Transition,
};

/**
 * @brief Matched rule with cascade info
 */
struct MatchedRule {
    const CSSStyleRule* rule;
    StyleOrigin origin;
    Selector::Specificity specificity;
    size_t order;   // Source order
    
    /// Compare for cascade ordering
    bool operator<(const MatchedRule& other) const;
};

/**
 * @brief CSS cascade engine
 */
class CSSCascade {
public:
    /// Collect all matching rules for element
    std::vector<MatchedRule> collectMatchingRules(
        DOMElement* element,
        const std::vector<CSSStyleSheet*>& stylesheets,
        StyleOrigin origin = StyleOrigin::Author
    );

    /// Sort by cascade order
    void sortByCascade(std::vector<MatchedRule>& rules);

    /// Compute cascaded value for property
    std::optional<std::string> cascadedValue(
        const std::string& property,
        const std::vector<MatchedRule>& rules
    );
    
    /// Check if selector matches element
    bool selectorMatches(DOMElement* element, const std::string& selector);
    
    /// Match a single selector group (handles combinators)
    bool selectorGroupMatches(DOMElement* element, const std::string& selector);
    
    /// Match compound selector (tag.class#id[attr]) against single element
    bool compoundSelectorMatches(DOMElement* element, const std::string& compound);
    
    /// Calculate specificity of a selector
    Selector::Specificity calculateSpecificity(const std::string& selector);
};

/**
 * @brief Style resolver - computes final styles
 */
class StyleResolver {
public:
    StyleResolver();
    ~StyleResolver();

    /// Add stylesheet
    void addStyleSheet(std::shared_ptr<CSSStyleSheet> sheet, StyleOrigin origin = StyleOrigin::Author);

    /// Add user agent stylesheet
    void addUserAgentStyleSheet();

    /// Compute style for element
    CSSComputedStyle computeStyle(DOMElement* element, const CSSComputedStyle* parentStyle = nullptr);

    /// Get computed style (cached)
    const CSSComputedStyle* getComputedStyle(DOMElement* element);

    /// Invalidate cached styles
    void invalidateElement(DOMElement* element);
    void invalidateAll();

private:
    struct StyleSheetEntry {
        std::shared_ptr<CSSStyleSheet> sheet;
        StyleOrigin origin;
    };

    std::vector<StyleSheetEntry> stylesheets_;
    std::unordered_map<DOMElement*, CSSComputedStyle> styleCache_;
    CSSCascade cascade_;
    SelectorMatcher matcher_;

    void applyDeclarations(CSSComputedStyle& style, const CSSStyleDeclaration* decl,
                           const CSSComputedStyle* parentStyle);
    void applyProperty(CSSComputedStyle& style, const std::string& property,
                       const std::string& value, const CSSComputedStyle* parentStyle);
    CSSLength parseLength(const std::string& value);
    CSSColor parseColor(const std::string& value);
};

/**
 * @brief CSS parser (enhanced)
 */
class CSSParser {
public:
    /// Parse stylesheet
    std::unique_ptr<CSSStyleSheet> parse(const std::string& css);

    /// Parse inline style
    std::unique_ptr<CSSStyleDeclaration> parseInlineStyle(const std::string& style);

    /// Parse single selector
    Selector parseSelector(const std::string& selector);

    /// Parse media query
    std::unique_ptr<MediaList> parseMediaQuery(const std::string& query);

private:
    size_t pos_ = 0;
    std::string input_;

    void skipWhitespace();
    void skipComment();
    char peek() const;
    char consume();
    bool match(char c);
    bool eof() const;

    std::unique_ptr<CSSRule> parseRule();
    std::unique_ptr<CSSStyleRule> parseStyleRule();
    std::unique_ptr<CSSMediaRule> parseMediaRule();
    std::unique_ptr<CSSFontFaceRule> parseFontFaceRule();
    std::unique_ptr<CSSKeyframesRule> parseKeyframesRule();
    std::unique_ptr<CSSImportRule> parseImportRule();

    std::string parseSelector_();
    std::string parseDeclarationBlock();
    void parseDeclarations(CSSStyleDeclaration* decl);
    std::string parsePropertyValue();
    std::string parseString();
    std::string parseIdentifier();
    std::string parseUrl();
};

/**
 * @brief CSS engine - main interface
 */
class CSSEngine {
public:
    CSSEngine();
    ~CSSEngine();

    /// Initialize with document
    void initialize(DOMDocument* document);

    /// Parse and add stylesheet
    void addStyleSheet(const std::string& css, StyleOrigin origin = StyleOrigin::Author);

    /// Add stylesheet from link
    void addStyleSheetFromUrl(const std::string& url, StyleOrigin origin = StyleOrigin::Author);

    /// Compute styles for document
    void computeStyles();

    /// Get computed style for element
    const CSSComputedStyle* getComputedStyle(DOMElement* element);

    /// Invalidate styles
    void invalidate(DOMElement* element = nullptr);

    /// Parser access
    CSSParser& parser() { return parser_; }

    /// Style resolver
    StyleResolver& resolver() { return resolver_; }

    /// Check @supports condition
    bool supports(const std::string& property, const std::string& value);
    bool supportsCondition(const std::string& condition);

    /// CSS namespace
    static std::string escape(const std::string& ident);
    static bool supportsPropertySyntax(const std::string& syntax);

private:
    DOMDocument* document_ = nullptr;
    CSSParser parser_;
    StyleResolver resolver_;
    std::vector<std::shared_ptr<CSSStyleSheet>> stylesheets_;
};

} // namespace Zepra::WebCore
