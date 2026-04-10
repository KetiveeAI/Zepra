// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "selector.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace NXRender {
namespace Web {

// ==================================================================
// NthExpression
// ==================================================================

bool NthExpression::matches(int index) const {
    // an+b formula: index is 1-based
    if (a == 0) return index == b;
    if (a > 0) {
        int diff = index - b;
        return diff >= 0 && diff % a == 0;
    }
    // a < 0
    int diff = index - b;
    return diff <= 0 && (-diff) % (-a) == 0;
}

NthExpression NthExpression::parse(const std::string& expr) {
    NthExpression result;
    std::string s = expr;
    // Remove spaces
    s.erase(std::remove(s.begin(), s.end(), ' '), s.end());

    if (s == "odd") { result.a = 2; result.b = 1; return result; }
    if (s == "even") { result.a = 2; result.b = 0; return result; }

    // Parse an+b
    auto npos = s.find('n');
    if (npos == std::string::npos) {
        result.a = 0;
        result.b = std::atoi(s.c_str());
    } else {
        std::string aStr = s.substr(0, npos);
        if (aStr.empty() || aStr == "+") result.a = 1;
        else if (aStr == "-") result.a = -1;
        else result.a = std::atoi(aStr.c_str());

        if (npos + 1 < s.size()) {
            result.b = std::atoi(s.c_str() + npos + 1);
        }
    }
    return result;
}

// ==================================================================
// Specificity
// ==================================================================

uint32_t Selector::specificity() const {
    return Specificity::compute(*this).packed();
}

Specificity Specificity::compute(const Selector& selector) {
    Specificity spec;
    for (const auto& compound : selector.compounds) {
        for (const auto& simple : compound.simples) {
            switch (simple.type) {
                case SelectorType::Id:
                    spec.a++;
                    break;
                case SelectorType::Class:
                case SelectorType::Attribute:
                    spec.b++;
                    break;
                case SelectorType::PseudoClass:
                    if (simple.pseudoClass == PseudoClassType::Where) {
                        // :where() does not contribute
                    } else if (simple.pseudoClass == PseudoClassType::Is ||
                               simple.pseudoClass == PseudoClassType::Not ||
                               simple.pseudoClass == PseudoClassType::Has) {
                        // Use highest specificity of arguments
                        uint32_t maxSpec = 0;
                        for (const auto& arg : simple.selectorArgs) {
                            maxSpec = std::max(maxSpec, arg.specificity());
                        }
                        spec.b++; // Simplified — real impl sums components
                    } else {
                        spec.b++;
                    }
                    break;
                case SelectorType::PseudoElement:
                    spec.c++;
                    break;
                case SelectorType::Type:
                    spec.c++;
                    break;
                case SelectorType::Universal:
                    break;
            }
        }
    }
    return spec;
}

std::string Selector::toString() const {
    std::string result;
    for (size_t i = 0; i < compounds.size(); i++) {
        if (i > 0) {
            switch (compounds[i].combinator) {
                case Combinator::Descendant: result += " "; break;
                case Combinator::Child: result += " > "; break;
                case Combinator::NextSibling: result += " + "; break;
                case Combinator::SubsequentSibling: result += " ~ "; break;
                default: break;
            }
        }
        for (const auto& s : compounds[i].simples) {
            switch (s.type) {
                case SelectorType::Universal: result += "*"; break;
                case SelectorType::Type: result += s.value; break;
                case SelectorType::Class: result += "." + s.value; break;
                case SelectorType::Id: result += "#" + s.value; break;
                case SelectorType::Attribute: result += "[" + s.attribute.name + "]"; break;
                case SelectorType::PseudoClass: result += ":" + s.value; break;
                case SelectorType::PseudoElement: result += "::" + s.value; break;
            }
        }
    }
    return result;
}

// ==================================================================
// SelectorMatcher
// ==================================================================

bool SelectorMatcher::matches(const Selector& selector, const ElementInterface* element) const {
    if (selector.compounds.empty() || !element) return false;

    // Match right to left
    int idx = static_cast<int>(selector.compounds.size()) - 1;
    const ElementInterface* current = element;

    if (!matchCompound(selector.compounds[idx], current)) return false;
    idx--;

    while (idx >= 0 && current) {
        Combinator comb = selector.compounds[idx + 1].combinator;

        switch (comb) {
            case Combinator::Descendant: {
                bool found = false;
                current = current->parent();
                while (current) {
                    if (matchCompound(selector.compounds[idx], current)) {
                        found = true;
                        break;
                    }
                    current = current->parent();
                }
                if (!found) return false;
                break;
            }
            case Combinator::Child:
                current = current->parent();
                if (!current || !matchCompound(selector.compounds[idx], current)) return false;
                break;
            case Combinator::NextSibling:
                current = current->previousSibling();
                if (!current || !matchCompound(selector.compounds[idx], current)) return false;
                break;
            case Combinator::SubsequentSibling: {
                bool found = false;
                current = current->previousSibling();
                while (current) {
                    if (matchCompound(selector.compounds[idx], current)) {
                        found = true;
                        break;
                    }
                    current = current->previousSibling();
                }
                if (!found) return false;
                break;
            }
            default:
                return false;
        }
        idx--;
    }

    return idx < 0;
}

bool SelectorMatcher::matchCompound(const CompoundSelector& compound,
                                      const ElementInterface* element) const {
    for (const auto& simple : compound.simples) {
        if (!matchSimple(simple, element)) return false;
    }
    return true;
}

bool SelectorMatcher::matchSimple(const SimpleSelector& simple,
                                    const ElementInterface* element) const {
    switch (simple.type) {
        case SelectorType::Universal:
            return true;
        case SelectorType::Type:
            return element->tagName() == simple.value;
        case SelectorType::Class:
            return element->hasClass(simple.value);
        case SelectorType::Id:
            return element->id() == simple.value;
        case SelectorType::Attribute:
            return matchAttribute(simple.attribute, element);
        case SelectorType::PseudoClass:
            return matchPseudoClass(simple, element);
        case SelectorType::PseudoElement:
            return true; // Pseudo-elements always match for painting purposes
    }
    return false;
}

bool SelectorMatcher::matchAttribute(const AttributeSelector& attr,
                                       const ElementInterface* element) const {
    if (!element->hasAttribute(attr.name)) return false;
    if (attr.match == AttributeMatch::Has) return true;

    std::string val = element->getAttribute(attr.name);
    std::string target = attr.value;

    if (attr.caseInsensitive) {
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);
        std::transform(target.begin(), target.end(), target.begin(), ::tolower);
    }

    switch (attr.match) {
        case AttributeMatch::Exact: return val == target;
        case AttributeMatch::Includes: {
            std::istringstream iss(val);
            std::string word;
            while (iss >> word) { if (word == target) return true; }
            return false;
        }
        case AttributeMatch::DashMatch:
            return val == target || (val.size() > target.size() &&
                   val.substr(0, target.size()) == target && val[target.size()] == '-');
        case AttributeMatch::Prefix:
            return val.size() >= target.size() && val.substr(0, target.size()) == target;
        case AttributeMatch::Suffix:
            return val.size() >= target.size() &&
                   val.substr(val.size() - target.size()) == target;
        case AttributeMatch::Substring:
            return val.find(target) != std::string::npos;
        default: return false;
    }
}

bool SelectorMatcher::matchPseudoClass(const SimpleSelector& simple,
                                         const ElementInterface* element) const {
    switch (simple.pseudoClass) {
        case PseudoClassType::Hover: return element->isHovered();
        case PseudoClassType::Active: return element->isActive();
        case PseudoClassType::Focus: return element->isFocused();
        case PseudoClassType::Link: return element->isLink() && !element->isVisited();
        case PseudoClassType::Visited: return element->isVisited();
        case PseudoClassType::AnyLink: return element->isLink();
        case PseudoClassType::Target: return element->isTarget();
        case PseudoClassType::Root: return element->isRoot();
        case PseudoClassType::Empty: return element->isEmpty();
        case PseudoClassType::Enabled: return element->isEnabled();
        case PseudoClassType::Disabled: return !element->isEnabled();
        case PseudoClassType::Checked: return element->isChecked();
        case PseudoClassType::Required: return element->isRequired();
        case PseudoClassType::Optional: return !element->isRequired();
        case PseudoClassType::ReadOnly: return element->isReadOnly();
        case PseudoClassType::ReadWrite: return !element->isReadOnly();

        case PseudoClassType::FirstChild:
            return element->childIndex() == 0;
        case PseudoClassType::LastChild:
            return element->parent() && element->childIndex() == element->parent()->childCount() - 1;
        case PseudoClassType::OnlyChild:
            return element->parent() && element->parent()->childCount() == 1;
        case PseudoClassType::FirstOfType:
            return element->childIndexOfType() == 0;
        case PseudoClassType::LastOfType:
            return element->parent() && element->childIndexOfType() == element->childCountOfType() - 1;
        case PseudoClassType::OnlyOfType:
            return element->childCountOfType() == 1;

        case PseudoClassType::NthChild:
            return matchNth(simple.nthExpr, element->childIndex() + 1);
        case PseudoClassType::NthLastChild:
            return element->parent() &&
                   matchNth(simple.nthExpr, element->parent()->childCount() - element->childIndex());
        case PseudoClassType::NthOfType:
            return matchNth(simple.nthExpr, element->childIndexOfType() + 1);
        case PseudoClassType::NthLastOfType:
            return matchNth(simple.nthExpr, element->childCountOfType() - element->childIndexOfType());

        case PseudoClassType::Is:
        case PseudoClassType::Not:
        case PseudoClassType::Where:
        case PseudoClassType::Has:
            return matchFunctionalPseudo(simple.pseudoClass, simple.selectorArgs, element);

        case PseudoClassType::Dir:
            return element->matchesDir(simple.value);
        case PseudoClassType::Lang:
            return element->matchesLang(simple.value);

        default:
            return false;
    }
}

bool SelectorMatcher::matchNth(const NthExpression& expr, int index) const {
    return expr.matches(index);
}

bool SelectorMatcher::matchFunctionalPseudo(PseudoClassType type,
                                              const std::vector<Selector>& args,
                                              const ElementInterface* element) const {
    switch (type) {
        case PseudoClassType::Is:
        case PseudoClassType::Where: {
            for (const auto& arg : args) {
                if (matches(arg, element)) return true;
            }
            return false;
        }
        case PseudoClassType::Not: {
            for (const auto& arg : args) {
                if (matches(arg, element)) return false;
            }
            return true;
        }
        case PseudoClassType::Has: {
            // :has() — check if any descendant matches
            // Simplified: only check direct children
            for (const auto& arg : args) {
                auto* child = element->firstChild();
                while (child) {
                    if (matches(arg, child)) return true;
                    child = child->nextSibling();
                }
            }
            return false;
        }
        default: return false;
    }
}

// ==================================================================
// SelectorParser — tokenizer
// ==================================================================

std::vector<SelectorParser::Token> SelectorParser::tokenize(const std::string& input) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < input.size()) {
        char c = input[i];

        if (std::isspace(c)) {
            while (i < input.size() && std::isspace(input[i])) i++;
            tokens.push_back({Token::Whitespace, " "});
            continue;
        }

        switch (c) {
            case '#': tokens.push_back({Token::Hash, "#"}); i++; break;
            case '.': tokens.push_back({Token::Dot, "."}); i++; break;
            case ',': tokens.push_back({Token::Comma, ","}); i++; break;
            case '>': tokens.push_back({Token::Greater, ">"}); i++; break;
            case '+': tokens.push_back({Token::Plus, "+"}); i++; break;
            case '~': {
                if (i + 1 < input.size() && input[i + 1] == '=') {
                    tokens.push_back({Token::IncludesMatch, "~="}); i += 2;
                } else {
                    tokens.push_back({Token::Tilde, "~"}); i++;
                }
                break;
            }
            case '*': {
                if (i + 1 < input.size() && input[i + 1] == '=') {
                    tokens.push_back({Token::SubstringMatch, "*="}); i += 2;
                } else {
                    tokens.push_back({Token::Star, "*"}); i++;
                }
                break;
            }
            case '|': {
                if (i + 1 < input.size() && input[i + 1] == '=') {
                    tokens.push_back({Token::DashMatch, "|="}); i += 2;
                } else {
                    tokens.push_back({Token::Pipe, "|"}); i++;
                }
                break;
            }
            case '[': tokens.push_back({Token::BracketOpen, "["}); i++; break;
            case ']': tokens.push_back({Token::BracketClose, "]"}); i++; break;
            case '(': tokens.push_back({Token::ParenOpen, "("}); i++; break;
            case ')': tokens.push_back({Token::ParenClose, ")"}); i++; break;
            case '=': tokens.push_back({Token::Equals, "="}); i++; break;
            case '^': {
                if (i + 1 < input.size() && input[i + 1] == '=') {
                    tokens.push_back({Token::PrefixMatch, "^="}); i += 2;
                } else {
                    i++; // Skip unknown
                }
                break;
            }
            case '$': {
                if (i + 1 < input.size() && input[i + 1] == '=') {
                    tokens.push_back({Token::SuffixMatch, "$="}); i += 2;
                } else {
                    i++;
                }
                break;
            }
            case ':': {
                if (i + 1 < input.size() && input[i + 1] == ':') {
                    tokens.push_back({Token::DoubleColon, "::"}); i += 2;
                } else {
                    tokens.push_back({Token::Colon, ":"}); i++;
                }
                break;
            }
            case '"':
            case '\'': {
                char quote = c;
                i++;
                std::string str;
                while (i < input.size() && input[i] != quote) {
                    if (input[i] == '\\' && i + 1 < input.size()) { str += input[i + 1]; i += 2; }
                    else { str += input[i]; i++; }
                }
                if (i < input.size()) i++;
                tokens.push_back({Token::String, str});
                break;
            }
            default: {
                if (std::isalpha(c) || c == '_' || c == '-') {
                    std::string ident;
                    while (i < input.size() && (std::isalnum(input[i]) || input[i] == '_' || input[i] == '-')) {
                        ident += input[i]; i++;
                    }
                    tokens.push_back({Token::Ident, ident});
                } else if (std::isdigit(c)) {
                    std::string num;
                    while (i < input.size() && (std::isdigit(input[i]) || input[i] == '.')) {
                        num += input[i]; i++;
                    }
                    tokens.push_back({Token::Number, num});
                } else {
                    i++;
                }
            }
        }
    }

    tokens.push_back({Token::Eof, ""});
    return tokens;
}

const SelectorParser::Token& SelectorParser::peek() const {
    return tokens_[pos_];
}

const SelectorParser::Token& SelectorParser::advance() {
    return tokens_[pos_++];
}

bool SelectorParser::expect(Token::Type type) {
    if (tokens_[pos_].type == type) { pos_++; return true; }
    return false;
}

bool SelectorParser::at(Token::Type type) const {
    return tokens_[pos_].type == type;
}

void SelectorParser::skipWhitespace() {
    while (pos_ < tokens_.size() && tokens_[pos_].type == Token::Whitespace) pos_++;
}

// ==================================================================
// SelectorParser — parse
// ==================================================================

std::vector<Selector> SelectorParser::parse(const std::string& selectorList) {
    tokens_ = tokenize(selectorList);
    pos_ = 0;
    std::vector<Selector> selectors;

    skipWhitespace();
    while (!at(Token::Eof)) {
        selectors.push_back(parseSelector());
        skipWhitespace();
        if (at(Token::Comma)) {
            advance();
            skipWhitespace();
        }
    }
    return selectors;
}

Selector SelectorParser::parseOne(const std::string& selectorStr) {
    auto list = parse(selectorStr);
    if (!list.empty()) return list[0];
    return Selector();
}

Selector SelectorParser::parseSelector() {
    Selector sel;
    sel.compounds.push_back(parseCompoundSelector());

    while (pos_ < tokens_.size()) {
        Combinator comb = Combinator::None;

        if (at(Token::Whitespace)) {
            skipWhitespace();
            if (at(Token::Greater)) { advance(); skipWhitespace(); comb = Combinator::Child; }
            else if (at(Token::Plus)) { advance(); skipWhitespace(); comb = Combinator::NextSibling; }
            else if (at(Token::Tilde)) { advance(); skipWhitespace(); comb = Combinator::SubsequentSibling; }
            else if (!at(Token::Comma) && !at(Token::Eof) && !at(Token::ParenClose)) {
                comb = Combinator::Descendant;
            } else break;
        } else if (at(Token::Greater)) {
            advance(); skipWhitespace(); comb = Combinator::Child;
        } else if (at(Token::Plus)) {
            advance(); skipWhitespace(); comb = Combinator::NextSibling;
        } else if (at(Token::Tilde)) {
            advance(); skipWhitespace(); comb = Combinator::SubsequentSibling;
        } else break;

        if (comb == Combinator::None) break;

        CompoundSelector compound = parseCompoundSelector();
        compound.combinator = comb;
        sel.compounds.push_back(std::move(compound));
    }

    return sel;
}

CompoundSelector SelectorParser::parseCompoundSelector() {
    CompoundSelector compound;

    while (pos_ < tokens_.size()) {
        auto type = peek().type;
        if (type == Token::Ident || type == Token::Star ||
            type == Token::Hash || type == Token::Dot ||
            type == Token::Colon || type == Token::DoubleColon ||
            type == Token::BracketOpen) {
            compound.simples.push_back(parseSimpleSelector());
        } else break;
    }

    if (compound.simples.empty()) {
        SimpleSelector universal;
        universal.type = SelectorType::Universal;
        compound.simples.push_back(universal);
    }

    return compound;
}

SimpleSelector SelectorParser::parseSimpleSelector() {
    SimpleSelector simple;

    if (at(Token::Star)) {
        advance();
        simple.type = SelectorType::Universal;
    } else if (at(Token::Ident)) {
        simple.type = SelectorType::Type;
        simple.value = advance().value;
    } else if (at(Token::Hash)) {
        advance();
        simple.type = SelectorType::Id;
        if (at(Token::Ident)) simple.value = advance().value;
    } else if (at(Token::Dot)) {
        advance();
        simple.type = SelectorType::Class;
        if (at(Token::Ident)) simple.value = advance().value;
    } else if (at(Token::BracketOpen)) {
        advance();
        simple.type = SelectorType::Attribute;
        simple.attribute = parseAttributeSelector();
        expect(Token::BracketClose);
    } else if (at(Token::DoubleColon)) {
        advance();
        simple.type = SelectorType::PseudoElement;
        if (at(Token::Ident)) {
            simple.value = advance().value;
            if (simple.value == "before") simple.pseudoElement = PseudoElementType::Before;
            else if (simple.value == "after") simple.pseudoElement = PseudoElementType::After;
            else if (simple.value == "first-line") simple.pseudoElement = PseudoElementType::FirstLine;
            else if (simple.value == "first-letter") simple.pseudoElement = PseudoElementType::FirstLetter;
            else if (simple.value == "marker") simple.pseudoElement = PseudoElementType::Marker;
            else if (simple.value == "placeholder") simple.pseudoElement = PseudoElementType::Placeholder;
            else if (simple.value == "selection") simple.pseudoElement = PseudoElementType::Selection;
        }
    } else if (at(Token::Colon)) {
        advance();
        simple.type = SelectorType::PseudoClass;
        if (at(Token::Ident)) {
            simple.value = advance().value;
            std::string name = simple.value;

            // Map name to PseudoClassType
            if (name == "hover") simple.pseudoClass = PseudoClassType::Hover;
            else if (name == "active") simple.pseudoClass = PseudoClassType::Active;
            else if (name == "focus") simple.pseudoClass = PseudoClassType::Focus;
            else if (name == "focus-visible") simple.pseudoClass = PseudoClassType::FocusVisible;
            else if (name == "focus-within") simple.pseudoClass = PseudoClassType::FocusWithin;
            else if (name == "visited") simple.pseudoClass = PseudoClassType::Visited;
            else if (name == "link") simple.pseudoClass = PseudoClassType::Link;
            else if (name == "any-link") simple.pseudoClass = PseudoClassType::AnyLink;
            else if (name == "target") simple.pseudoClass = PseudoClassType::Target;
            else if (name == "root") simple.pseudoClass = PseudoClassType::Root;
            else if (name == "empty") simple.pseudoClass = PseudoClassType::Empty;
            else if (name == "first-child") simple.pseudoClass = PseudoClassType::FirstChild;
            else if (name == "last-child") simple.pseudoClass = PseudoClassType::LastChild;
            else if (name == "only-child") simple.pseudoClass = PseudoClassType::OnlyChild;
            else if (name == "first-of-type") simple.pseudoClass = PseudoClassType::FirstOfType;
            else if (name == "last-of-type") simple.pseudoClass = PseudoClassType::LastOfType;
            else if (name == "only-of-type") simple.pseudoClass = PseudoClassType::OnlyOfType;
            else if (name == "enabled") simple.pseudoClass = PseudoClassType::Enabled;
            else if (name == "disabled") simple.pseudoClass = PseudoClassType::Disabled;
            else if (name == "checked") simple.pseudoClass = PseudoClassType::Checked;
            else if (name == "required") simple.pseudoClass = PseudoClassType::Required;
            else if (name == "optional") simple.pseudoClass = PseudoClassType::Optional;
            else if (name == "read-only") simple.pseudoClass = PseudoClassType::ReadOnly;
            else if (name == "read-write") simple.pseudoClass = PseudoClassType::ReadWrite;
            else if (name == "valid") simple.pseudoClass = PseudoClassType::Valid;
            else if (name == "invalid") simple.pseudoClass = PseudoClassType::Invalid;
            else if (name == "placeholder-shown") simple.pseudoClass = PseudoClassType::PlaceholderShown;

            // Functional pseudo-classes
            if (at(Token::ParenOpen)) {
                advance();
                if (name == "nth-child" || name == "nth-last-child" ||
                    name == "nth-of-type" || name == "nth-last-of-type") {
                    if (name == "nth-child") simple.pseudoClass = PseudoClassType::NthChild;
                    else if (name == "nth-last-child") simple.pseudoClass = PseudoClassType::NthLastChild;
                    else if (name == "nth-of-type") simple.pseudoClass = PseudoClassType::NthOfType;
                    else simple.pseudoClass = PseudoClassType::NthLastOfType;
                    simple.nthExpr = parseNthExpression();
                } else if (name == "is" || name == "not" || name == "where" || name == "has") {
                    if (name == "is") simple.pseudoClass = PseudoClassType::Is;
                    else if (name == "not") simple.pseudoClass = PseudoClassType::Not;
                    else if (name == "where") simple.pseudoClass = PseudoClassType::Where;
                    else simple.pseudoClass = PseudoClassType::Has;
                    simple.selectorArgs = parseSelectorArguments();
                } else if (name == "dir" || name == "lang") {
                    simple.pseudoClass = (name == "dir") ? PseudoClassType::Dir : PseudoClassType::Lang;
                    if (at(Token::Ident)) simple.value = advance().value;
                }
                expect(Token::ParenClose);
            }
        }
    }

    return simple;
}

AttributeSelector SelectorParser::parseAttributeSelector() {
    AttributeSelector attr;
    skipWhitespace();
    if (at(Token::Ident)) attr.name = advance().value;
    skipWhitespace();

    if (at(Token::Equals)) {
        advance(); attr.match = AttributeMatch::Exact;
    } else if (at(Token::IncludesMatch)) {
        advance(); attr.match = AttributeMatch::Includes;
    } else if (at(Token::DashMatch)) {
        advance(); attr.match = AttributeMatch::DashMatch;
    } else if (at(Token::PrefixMatch)) {
        advance(); attr.match = AttributeMatch::Prefix;
    } else if (at(Token::SuffixMatch)) {
        advance(); attr.match = AttributeMatch::Suffix;
    } else if (at(Token::SubstringMatch)) {
        advance(); attr.match = AttributeMatch::Substring;
    } else {
        return attr; // [attr] — presence only
    }

    skipWhitespace();
    if (at(Token::String) || at(Token::Ident)) {
        attr.value = advance().value;
    }
    skipWhitespace();

    // Case sensitivity flag
    if (at(Token::Ident) && (peek().value == "i" || peek().value == "I")) {
        advance();
        attr.caseInsensitive = true;
    }

    return attr;
}

NthExpression SelectorParser::parseNthExpression() {
    std::string expr;
    while (!at(Token::ParenClose) && !at(Token::Eof)) {
        expr += advance().value;
    }
    return NthExpression::parse(expr);
}

std::vector<Selector> SelectorParser::parseSelectorArguments() {
    // Parse comma-separated selector list inside ()
    std::vector<Selector> args;
    skipWhitespace();
    while (!at(Token::ParenClose) && !at(Token::Eof)) {
        args.push_back(parseSelector());
        skipWhitespace();
        if (at(Token::Comma)) { advance(); skipWhitespace(); }
    }
    return args;
}

} // namespace Web
} // namespace NXRender
