// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <functional>

namespace NXRender {
namespace Web {

// ==================================================================
// CSS Selector types and structures
// ==================================================================

enum class SelectorType : uint8_t {
    Universal,      // *
    Type,           // div, span, etc.
    Class,          // .classname
    Id,             // #id
    Attribute,      // [attr], [attr=val], etc.
    PseudoClass,    // :hover, :first-child, etc.
    PseudoElement,  // ::before, ::after, etc.
};

enum class Combinator : uint8_t {
    None,           // Initial / no combinator
    Descendant,     // space
    Child,          // >
    NextSibling,    // +
    SubsequentSibling, // ~
    Column,         // ||
};

enum class AttributeMatch : uint8_t {
    Has,            // [attr]
    Exact,          // [attr=val]
    Includes,       // [attr~=val]  (whitespace-separated list)
    DashMatch,      // [attr|=val]  (hyphen-separated prefix)
    Prefix,         // [attr^=val]
    Suffix,         // [attr$=val]
    Substring,      // [attr*=val]
};

enum class PseudoClassType : uint8_t {
    // Dynamic
    Hover, Active, Focus, FocusVisible, FocusWithin,
    Visited, Link, AnyLink, Target, TargetWithin,

    // Structural
    Root, Empty, FirstChild, LastChild, OnlyChild,
    FirstOfType, LastOfType, OnlyOfType,
    NthChild, NthLastChild, NthOfType, NthLastOfType,

    // Input
    Enabled, Disabled, Checked, Indeterminate,
    Required, Optional, ReadOnly, ReadWrite,
    Valid, Invalid, InRange, OutOfRange,
    PlaceholderShown, Default, Autofill,

    // Logical
    Is, Not, Where, Has,

    // Other
    Dir, Lang, Defined, PopoverOpen,
};

enum class PseudoElementType : uint8_t {
    Before, After, FirstLine, FirstLetter,
    Marker, Placeholder, Selection, Backdrop,
    FileSelectorButton,
};

struct NthExpression {
    int a = 0, b = 0; // an+b
    bool matches(int index) const;
    static NthExpression parse(const std::string& expr);
};

struct AttributeSelector {
    std::string name;
    std::string value;
    AttributeMatch match = AttributeMatch::Has;
    bool caseInsensitive = false;
};

struct SimpleSelector {
    SelectorType type = SelectorType::Universal;
    std::string value;                      // tag, class, or id name
    AttributeSelector attribute;            // for [attr] selectors
    PseudoClassType pseudoClass = PseudoClassType::Hover;
    PseudoElementType pseudoElement = PseudoElementType::Before;
    NthExpression nthExpr;                  // for :nth-child, etc.
    std::vector<struct Selector> selectorArgs; // for :is(), :not(), etc.
};

struct CompoundSelector {
    std::vector<SimpleSelector> simples;
    Combinator combinator = Combinator::None;
};

struct Selector {
    std::vector<CompoundSelector> compounds;

    // Specificity: (a, b, c) stored as packed int
    uint32_t specificity() const;
    std::string toString() const;
    bool isValid() const { return !compounds.empty(); }
};

// ==================================================================
// Specificity
// ==================================================================

struct Specificity {
    uint16_t a = 0; // IDs
    uint16_t b = 0; // classes, attributes, pseudo-classes
    uint16_t c = 0; // type selectors, pseudo-elements

    uint32_t packed() const { return (a << 20) | (b << 10) | c; }
    bool operator>(const Specificity& other) const { return packed() > other.packed(); }
    bool operator==(const Specificity& other) const { return packed() == other.packed(); }
    bool operator<(const Specificity& other) const { return packed() < other.packed(); }

    static Specificity compute(const Selector& selector);
};

// ==================================================================
// Element interface for selector matching
// ==================================================================

class ElementInterface {
public:
    virtual ~ElementInterface() = default;

    virtual const std::string& tagName() const = 0;
    virtual const std::string& id() const = 0;
    virtual bool hasClass(const std::string& cls) const = 0;
    virtual const std::vector<std::string>& classNames() const = 0;
    virtual bool hasAttribute(const std::string& name) const = 0;
    virtual std::string getAttribute(const std::string& name) const = 0;

    virtual ElementInterface* parent() const = 0;
    virtual ElementInterface* previousSibling() const = 0;
    virtual ElementInterface* nextSibling() const = 0;
    virtual ElementInterface* firstChild() const = 0;
    virtual ElementInterface* lastChild() const = 0;
    virtual int childIndex() const = 0;
    virtual int childCount() const = 0;
    virtual int childIndexOfType() const = 0;
    virtual int childCountOfType() const = 0;

    // Dynamic state
    virtual bool isHovered() const = 0;
    virtual bool isActive() const = 0;
    virtual bool isFocused() const = 0;
    virtual bool isVisited() const = 0;
    virtual bool isLink() const = 0;
    virtual bool isEnabled() const = 0;
    virtual bool isChecked() const = 0;
    virtual bool isRequired() const = 0;
    virtual bool isReadOnly() const = 0;
    virtual bool isEmpty() const = 0;
    virtual bool isRoot() const = 0;
    virtual bool isTarget() const = 0;
    virtual bool matchesLang(const std::string& lang) const = 0;
    virtual bool matchesDir(const std::string& dir) const = 0;
};

// ==================================================================
// Selector matcher
// ==================================================================

class SelectorMatcher {
public:
    bool matches(const Selector& selector, const ElementInterface* element) const;

private:
    bool matchCompound(const CompoundSelector& compound,
                       const ElementInterface* element) const;
    bool matchSimple(const SimpleSelector& simple,
                     const ElementInterface* element) const;
    bool matchAttribute(const AttributeSelector& attr,
                        const ElementInterface* element) const;
    bool matchPseudoClass(const SimpleSelector& simple,
                          const ElementInterface* element) const;
    bool matchNth(const NthExpression& expr, int index) const;
    bool matchCombinator(Combinator combinator,
                         const CompoundSelector& left,
                         const ElementInterface* element) const;

    // :is(), :not(), :where(), :has() evaluation
    bool matchFunctionalPseudo(PseudoClassType type,
                               const std::vector<Selector>& args,
                               const ElementInterface* element) const;
};

// ==================================================================
// Selector parser
// ==================================================================

class SelectorParser {
public:
    std::vector<Selector> parse(const std::string& selectorList);
    Selector parseOne(const std::string& selectorStr);

private:
    struct Token {
        enum Type {
            Ident, Hash, Dot, Colon, DoubleColon,
            BracketOpen, BracketClose, ParenOpen, ParenClose,
            Comma, Greater, Plus, Tilde, Star, Pipe,
            String, Number, Whitespace, Equals,
            PrefixMatch, SuffixMatch, SubstringMatch,
            DashMatch, IncludesMatch,
            Eof
        } type;
        std::string value;
    };

    std::vector<Token> tokenize(const std::string& input);

    size_t pos_ = 0;
    std::vector<Token> tokens_;

    const Token& peek() const;
    const Token& advance();
    bool expect(Token::Type type);
    bool at(Token::Type type) const;
    void skipWhitespace();

    Selector parseSelector();
    CompoundSelector parseCompoundSelector();
    SimpleSelector parseSimpleSelector();
    AttributeSelector parseAttributeSelector();
    NthExpression parseNthExpression();
    std::vector<Selector> parseSelectorArguments();
};

} // namespace Web
} // namespace NXRender
