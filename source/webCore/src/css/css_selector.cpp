/**
 * @file css_selector.cpp
 * @brief CSS Selector implementation
 */

#include "webcore/css/css_selector.hpp"
#include "webcore/dom.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <functional>

namespace Zepra::WebCore {

// Helper: Get parent as DOMElement
static DOMElement* getParentElement(DOMElement* el) {
    if (!el) return nullptr;
    DOMNode* parent = el->parentNode();
    if (parent && parent->nodeType() == NodeType::Element) {
        return static_cast<DOMElement*>(parent);
    }
    return nullptr;
}

// Helper: Get previous sibling element
static DOMElement* getPreviousElementSibling(DOMElement* el) {
    if (!el) return nullptr;
    DOMNode* sibling = el->previousSibling();
    while (sibling) {
        if (sibling->nodeType() == NodeType::Element) {
            return static_cast<DOMElement*>(sibling);
        }
        sibling = sibling->previousSibling();
    }
    return nullptr;
}

// =============================================================================
// NthExpression
// =============================================================================

NthExpression NthExpression::parse(const std::string& expr) {
    NthExpression result;
    
    std::string s = expr;
    s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
    
    if (s == "odd") {
        result.a = 2;
        result.b = 1;
    } else if (s == "even") {
        result.a = 2;
        result.b = 0;
    } else {
        size_t nPos = s.find('n');
        if (nPos != std::string::npos) {
            if (nPos == 0) {
                result.a = 1;
            } else if (nPos == 1 && s[0] == '-') {
                result.a = -1;
            } else if (nPos == 1 && s[0] == '+') {
                result.a = 1;
            } else {
                result.a = std::stoi(s.substr(0, nPos));
            }
            
            if (nPos + 1 < s.size()) {
                result.b = std::stoi(s.substr(nPos + 1));
            }
        } else {
            result.b = std::stoi(s);
        }
    }
    
    return result;
}

bool NthExpression::matches(int index) const {
    if (a == 0) return index == b;
    int diff = index - b;
    if (a > 0) return diff >= 0 && diff % a == 0;
    return diff <= 0 && diff % a == 0;
}

// =============================================================================
// Selector Specificity
// =============================================================================

bool Selector::Specificity::operator<(const Specificity& other) const {
    if (a != other.a) return a < other.a;
    if (b != other.b) return b < other.b;
    return c < other.c;
}

bool Selector::Specificity::operator>(const Specificity& other) const {
    return other < *this;
}

bool Selector::Specificity::operator==(const Specificity& other) const {
    return a == other.a && b == other.b && c == other.c;
}

// =============================================================================
// SimpleSelector
// =============================================================================

bool SimpleSelector::matches(DOMElement* element) const {
    if (!element) return false;
    
    // Type selector
    if (!isUniversal && !typeSelector.empty()) {
        if (element->tagName() != typeSelector) {
            return false;
        }
    }
    
    // ID selector
    if (!idSelector.empty()) {
        if (element->id() != idSelector) {
            return false;
        }
    }
    
    // Class selectors
    for (const auto& cls : classSelectors) {
        std::vector<std::string> classes = element->classList();
        if (std::find(classes.begin(), classes.end(), cls) == classes.end()) {
            return false;
        }
    }
    
    // Attribute selectors
    for (const auto& attr : attributeSelectors) {
        if (!element->hasAttribute(attr.name)) {
            return false;
        }
        
        std::string value = element->getAttribute(attr.name);
        
        switch (attr.op) {
            case AttributeOperator::Exists:
                break;
            case AttributeOperator::Equals:
                if (value != attr.value) return false;
                break;
            case AttributeOperator::BeginsWith:
                if (value.find(attr.value) != 0) return false;
                break;
            case AttributeOperator::EndsWith:
                if (value.size() < attr.value.size() ||
                    value.substr(value.size() - attr.value.size()) != attr.value) {
                    return false;
                }
                break;
            case AttributeOperator::Contains:
                if (value.find(attr.value) == std::string::npos) return false;
                break;
            case AttributeOperator::WhitespaceSeparated: {
                std::istringstream iss(value);
                std::string word;
                bool found = false;
                while (iss >> word) {
                    if (word == attr.value) { found = true; break; }
                }
                if (!found) return false;
                break;
            }
            case AttributeOperator::HyphenSeparated:
                if (value != attr.value && value.find(attr.value + "-") != 0) {
                    return false;
                }
                break;
        }
    }
    
    return true;
}

void SimpleSelector::addSpecificity(int& a, int& b, int& c) const {
    if (!idSelector.empty()) a++;
    b += classSelectors.size();
    b += attributeSelectors.size();
    b += pseudoClasses.size();
    if (!typeSelector.empty() && !isUniversal) c++;
    if (pseudoElement != PseudoElementType::None) c++;
}

// =============================================================================
// Selector
// =============================================================================

Selector::Specificity Selector::specificity() const {
    Specificity spec;
    for (const auto& compound : compounds) {
        compound.base.addSpecificity(spec.a, spec.b, spec.c);
    }
    return spec;
}

bool Selector::matches(DOMElement* element) const {
    if (compounds.empty()) return false;
    
    DOMElement* current = element;
    
    for (int i = static_cast<int>(compounds.size()) - 1; i >= 0; --i) {
        if (!current) return false;
        
        if (!compounds[i].base.matches(current)) {
            return false;
        }
        
        if (i > 0) {
            SelectorCombinator comb = compounds[i].combinator;
            
            switch (comb) {
                case SelectorCombinator::Descendant:
                    current = getParentElement(current);
                    while (current && !compounds[i-1].base.matches(current)) {
                        current = getParentElement(current);
                    }
                    break;
                    
                case SelectorCombinator::Child:
                    current = getParentElement(current);
                    break;
                    
                case SelectorCombinator::NextSibling:
                    current = getPreviousElementSibling(current);
                    break;
                    
                case SelectorCombinator::SubsequentSibling:
                    current = getPreviousElementSibling(current);
                    while (current && !compounds[i-1].base.matches(current)) {
                        current = getPreviousElementSibling(current);
                    }
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    return true;
}

Selector Selector::parse(const std::string& selectorText) {
    SelectorParser parser;
    return parser.parse(selectorText);
}

// =============================================================================
// SelectorParser
// =============================================================================

Selector SelectorParser::parse(const std::string& input) {
    pos_ = 0;
    input_ = input;
    return parseSelector();
}

std::vector<Selector> SelectorParser::parseList(const std::string& input) {
    std::vector<Selector> selectors;
    std::istringstream iss(input);
    std::string part;
    while (std::getline(iss, part, ',')) {
        size_t start = part.find_first_not_of(" \t\n\r");
        size_t end = part.find_last_not_of(" \t\n\r");
        if (start != std::string::npos) {
            part = part.substr(start, end - start + 1);
            selectors.push_back(parse(part));
        }
    }
    return selectors;
}

void SelectorParser::skipWhitespace() {
    while (pos_ < input_.size() && std::isspace(input_[pos_])) pos_++;
}

char SelectorParser::peek() const {
    return pos_ >= input_.size() ? '\0' : input_[pos_];
}

char SelectorParser::consume() {
    return pos_ >= input_.size() ? '\0' : input_[pos_++];
}

bool SelectorParser::match(char c) {
    skipWhitespace();
    if (peek() == c) { consume(); return true; }
    return false;
}

Selector SelectorParser::parseSelector() {
    Selector sel;
    
    while (pos_ < input_.size()) {
        skipWhitespace();
        if (peek() == '\0') break;
        
        CompoundSelector compound = parseCompoundSelector();
        sel.compounds.push_back(compound);
        
        skipWhitespace();
        char c = peek();
        
        if (c == '>') {
            consume();
            sel.compounds.back().combinator = SelectorCombinator::Child;
        } else if (c == '+') {
            consume();
            sel.compounds.back().combinator = SelectorCombinator::NextSibling;
        } else if (c == '~') {
            consume();
            sel.compounds.back().combinator = SelectorCombinator::SubsequentSibling;
        } else if (std::isalnum(c) || c == '#' || c == '.' || c == '[' || c == '*' || c == ':') {
            sel.compounds.back().combinator = SelectorCombinator::Descendant;
        } else {
            break;
        }
    }
    
    return sel;
}

CompoundSelector SelectorParser::parseCompoundSelector() {
    CompoundSelector compound;
    compound.base = parseSimpleSelector();
    return compound;
}

SimpleSelector SelectorParser::parseSimpleSelector() {
    SimpleSelector sel;
    
    skipWhitespace();
    char c = peek();
    
    if (c == '*') {
        consume();
        sel.isUniversal = true;
    } else if (std::isalpha(c)) {
        sel.typeSelector = parseIdentifier();
    }
    
    while (true) {
        c = peek();
        
        if (c == '#') {
            consume();
            sel.idSelector = parseIdentifier();
        } else if (c == '.') {
            consume();
            sel.classSelectors.push_back(parseIdentifier());
        } else if (c == '[') {
            sel.attributeSelectors.push_back(parseAttributeSelector());
        } else if (c == ':') {
            consume();
            if (peek() == ':') {
                consume();
                sel.pseudoElement = parsePseudoElement();
            } else {
                sel.pseudoClasses.push_back(parsePseudoClass());
            }
        } else {
            break;
        }
    }
    
    return sel;
}

std::string SelectorParser::parseIdentifier() {
    std::string result;
    while (pos_ < input_.size()) {
        char c = input_[pos_];
        if (std::isalnum(c) || c == '-' || c == '_') {
            result += c;
            pos_++;
        } else {
            break;
        }
    }
    return result;
}

AttributeSelector SelectorParser::parseAttributeSelector() {
    AttributeSelector attr;
    
    consume();  // '['
    skipWhitespace();
    attr.name = parseIdentifier();
    skipWhitespace();
    
    char c = peek();
    if (c == '=') {
        consume();
        attr.op = AttributeOperator::Equals;
    } else if (c == '^' && pos_ + 1 < input_.size() && input_[pos_ + 1] == '=') {
        consume(); consume();
        attr.op = AttributeOperator::BeginsWith;
    } else if (c == '$' && pos_ + 1 < input_.size() && input_[pos_ + 1] == '=') {
        consume(); consume();
        attr.op = AttributeOperator::EndsWith;
    } else if (c == '*' && pos_ + 1 < input_.size() && input_[pos_ + 1] == '=') {
        consume(); consume();
        attr.op = AttributeOperator::Contains;
    } else if (c == '~' && pos_ + 1 < input_.size() && input_[pos_ + 1] == '=') {
        consume(); consume();
        attr.op = AttributeOperator::WhitespaceSeparated;
    } else if (c == '|' && pos_ + 1 < input_.size() && input_[pos_ + 1] == '=') {
        consume(); consume();
        attr.op = AttributeOperator::HyphenSeparated;
    }
    
    if (attr.op != AttributeOperator::Exists) {
        skipWhitespace();
        if (peek() == '"' || peek() == '\'') {
            attr.value = parseString();
        } else {
            attr.value = parseIdentifier();
        }
    }
    
    skipWhitespace();
    if (peek() == 'i' || peek() == 'I') {
        consume();
        attr.caseInsensitive = true;
    }
    
    skipWhitespace();
    if (peek() == ']') consume();
    
    return attr;
}

SimpleSelector::PseudoClass SelectorParser::parsePseudoClass() {
    SimpleSelector::PseudoClass pc;
    std::string name = parseIdentifier();
    
    if (name == "hover") pc.type = PseudoClassType::Hover;
    else if (name == "active") pc.type = PseudoClassType::Active;
    else if (name == "focus") pc.type = PseudoClassType::Focus;
    else if (name == "first-child") pc.type = PseudoClassType::FirstChild;
    else if (name == "last-child") pc.type = PseudoClassType::LastChild;
    else if (name == "nth-child") pc.type = PseudoClassType::NthChild;
    else if (name == "enabled") pc.type = PseudoClassType::Enabled;
    else if (name == "disabled") pc.type = PseudoClassType::Disabled;
    else if (name == "checked") pc.type = PseudoClassType::Checked;
    else if (name == "link") pc.type = PseudoClassType::Link;
    else if (name == "visited") pc.type = PseudoClassType::Visited;
    else if (name == "root") pc.type = PseudoClassType::Root;
    else if (name == "empty") pc.type = PseudoClassType::Empty;
    else if (name == "not") pc.type = PseudoClassType::Not;
    else if (name == "is") pc.type = PseudoClassType::Is;
    else if (name == "where") pc.type = PseudoClassType::Where;
    
    if (peek() == '(') {
        consume();
        skipWhitespace();
        std::string arg;
        int depth = 1;
        while (depth > 0 && pos_ < input_.size()) {
            char c = consume();
            if (c == '(') depth++;
            else if (c == ')') depth--;
            if (depth > 0) arg += c;
        }
        pc.argument = arg;
        if (pc.type == PseudoClassType::NthChild || pc.type == PseudoClassType::NthLastChild) {
            pc.nth = NthExpression::parse(arg);
        }
    }
    
    return pc;
}

PseudoElementType SelectorParser::parsePseudoElement() {
    std::string name = parseIdentifier();
    if (name == "before") return PseudoElementType::Before;
    if (name == "after") return PseudoElementType::After;
    if (name == "first-line") return PseudoElementType::FirstLine;
    if (name == "first-letter") return PseudoElementType::FirstLetter;
    if (name == "selection") return PseudoElementType::Selection;
    if (name == "placeholder") return PseudoElementType::Placeholder;
    return PseudoElementType::None;
}

std::string SelectorParser::parseString() {
    char quote = consume();
    std::string result;
    while (pos_ < input_.size() && input_[pos_] != quote) {
        if (input_[pos_] == '\\' && pos_ + 1 < input_.size()) pos_++;
        result += input_[pos_++];
    }
    if (peek() == quote) consume();
    return result;
}

// =============================================================================
// SelectorMatcher
// =============================================================================

bool SelectorMatcher::matches(const Selector& selector, DOMElement* element) {
    return selector.matches(element);
}

std::vector<DOMElement*> SelectorMatcher::querySelectorAll(DOMElement* root, const Selector& selector) {
    std::vector<DOMElement*> results;
    
    std::function<void(DOMNode*)> traverse = [&](DOMNode* node) {
        if (node->nodeType() == NodeType::Element) {
            auto* el = static_cast<DOMElement*>(node);
            if (selector.matches(el)) {
                results.push_back(el);
            }
        }
        for (const auto& child : node->childNodes()) {
            traverse(child.get());
        }
    };
    
    traverse(root);
    return results;
}

DOMElement* SelectorMatcher::querySelector(DOMElement* root, const Selector& selector) {
    std::function<DOMElement*(DOMNode*)> traverse = [&](DOMNode* node) -> DOMElement* {
        if (node->nodeType() == NodeType::Element) {
            auto* el = static_cast<DOMElement*>(node);
            if (selector.matches(el)) {
                return el;
            }
        }
        for (const auto& child : node->childNodes()) {
            if (auto* result = traverse(child.get())) {
                return result;
            }
        }
        return nullptr;
    };
    
    return traverse(root);
}

} // namespace Zepra::WebCore
