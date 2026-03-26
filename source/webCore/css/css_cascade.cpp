// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * css_cascade.cpp - CSS Cascade Implementation
 * 
 * Implements the CSS cascade algorithm per W3C spec:
 * https://www.w3.org/TR/css-cascade-5/
 * 
 * 1. Collect matching rules for element
 * 2. Sort by cascade order (origin, specificity, source order)
 * 3. Resolve cascaded values
 */

#include "css/css_engine.hpp"
#include "browser/dom.hpp"
#include <algorithm>
#include <iostream>

namespace Zepra::WebCore {

// ============================================================================
// MatchedRule comparison (cascade ordering)
// ============================================================================

bool MatchedRule::operator<(const MatchedRule& other) const {
    // 1. Origin (lower origins have lower priority)
    // UserAgent < User < Author (normal)
    // Author!important > User!important > UserAgent!important
    if (origin != other.origin) {
        return static_cast<int>(origin) < static_cast<int>(other.origin);
    }
    
    // 2. Specificity (higher specificity wins)
    // Compare (a, b, c) tuple: IDs, Classes, Elements
    if (specificity.a != other.specificity.a) {
        return specificity.a < other.specificity.a;
    }
    if (specificity.b != other.specificity.b) {
        return specificity.b < other.specificity.b;
    }
    if (specificity.c != other.specificity.c) {
        return specificity.c < other.specificity.c;
    }
    
    // 3. Source order (later declarations win)
    return order < other.order;
}

// ============================================================================
// CSSCascade - Rule Collection and Sorting
// ============================================================================

std::vector<MatchedRule> CSSCascade::collectMatchingRules(
    DOMElement* element,
    const std::vector<CSSStyleSheet*>& stylesheets,
    StyleOrigin origin
) {
    std::vector<MatchedRule> matched;
    size_t ruleOrder = 0;
    
    if (!element) return matched;
    
    // Iterate all stylesheets
    for (const auto* sheet : stylesheets) {
        if (!sheet || !sheet->cssRules()) continue;
        
        // Iterate all rules in stylesheet
        CSSRuleList* rules = sheet->cssRules();
        for (size_t i = 0; i < rules->length(); i++) {
            const CSSRule* rule = rules->item(i);
            
            // Only process style rules
            const CSSStyleRule* styleRule = dynamic_cast<const CSSStyleRule*>(rule);
            if (!styleRule) continue;
            
            // Check if selector matches this element
            if (selectorMatches(element, styleRule->selectorText())) {
                MatchedRule match;
                match.rule = styleRule;
                match.origin = origin;
                match.specificity = calculateSpecificity(styleRule->selectorText());
                match.order = ruleOrder++;
                
                matched.push_back(match);
            }
        }
    }
    
    return matched;
}

void CSSCascade::sortByCascade(std::vector<MatchedRule>& rules) {
    // Sort using operator< which implements cascade ordering
    std::sort(rules.begin(), rules.end());
}

std::optional<std::string> CSSCascade::cascadedValue(
    const std::string& property,
    const std::vector<MatchedRule>& rules
) {
    // Iterate rules in reverse (highest priority last due to sort order)
    for (auto it = rules.rbegin(); it != rules.rend(); ++it) {
        if (!it->rule || !it->rule->style()) continue;
        
        std::string value = it->rule->style()->getPropertyValue(property);
        if (!value.empty()) {
            return value;
        }
    }
    
    return std::nullopt;
}

// ============================================================================
// Helper: Selector Matching (simplified)
// ============================================================================

bool CSSCascade::selectorMatches(DOMElement* element, const std::string& selector) {
    if (!element) return false;
    
    std::string sel = selector;
    
    // Trim whitespace
    size_t start = sel.find_first_not_of(" \t\n\r");
    size_t end = sel.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) return false;
    sel = sel.substr(start, end - start + 1);
    
    // Handle comma-separated selector groups: "div, .class, #id"
    // Each group is tested independently — if any matches, the selector matches
    {
        size_t depth = 0;
        size_t groupStart = 0;
        for (size_t i = 0; i <= sel.size(); i++) {
            if (i < sel.size() && sel[i] == '(') depth++;
            else if (i < sel.size() && sel[i] == ')' && depth > 0) depth--;
            else if ((i == sel.size() || sel[i] == ',') && depth == 0) {
                std::string group = sel.substr(groupStart, i - groupStart);
                // Trim the group
                size_t gs = group.find_first_not_of(" \t\n\r");
                size_t ge = group.find_last_not_of(" \t\n\r");
                if (gs != std::string::npos) {
                    group = group.substr(gs, ge - gs + 1);
                    if (selectorGroupMatches(element, group)) return true;
                }
                groupStart = i + 1;
            }
        }
        return false;
    }
}

// Match a single selector group (no commas) — handles combinators
bool CSSCascade::selectorGroupMatches(DOMElement* element, const std::string& selector) {
    if (!element || selector.empty()) return false;
    
    // Tokenize selector into parts separated by combinators
    // "div > .class p.item" → ["div", ">", ".class", " ", "p.item"]
    std::vector<std::string> parts;
    std::vector<char> combinators; // ' ' = descendant, '>' = child
    
    size_t i = 0;
    size_t len = selector.size();
    while (i < len) {
        // Skip whitespace
        while (i < len && (selector[i] == ' ' || selector[i] == '\t')) i++;
        if (i >= len) break;
        
        // Check for combinator
        if (!parts.empty() && (selector[i] == '>' || selector[i] == '+' || selector[i] == '~')) {
            char comb = selector[i];
            i++;
            while (i < len && (selector[i] == ' ' || selector[i] == '\t')) i++;
            combinators.push_back(comb);
            continue;
        }
        
        // Read compound selector part
        size_t partStart = i;
        while (i < len && selector[i] != ' ' && selector[i] != '\t' && 
               selector[i] != '>' && selector[i] != '+' && selector[i] != '~') {
            if (selector[i] == '[') {
                while (i < len && selector[i] != ']') i++;
                if (i < len) i++;
            } else if (selector[i] == '(') {
                int depth = 1;
                i++;
                while (i < len && depth > 0) {
                    if (selector[i] == '(') depth++;
                    else if (selector[i] == ')') depth--;
                    i++;
                }
            } else {
                i++;
            }
        }
        
        if (i > partStart) {
            std::string part = selector.substr(partStart, i - partStart);
            if (!parts.empty() && combinators.size() < parts.size()) {
                combinators.push_back(' '); // implicit descendant combinator
            }
            parts.push_back(part);
        }
    }
    
    if (parts.empty()) return false;
    
    // Single compound selector — no combinators
    if (parts.size() == 1) {
        return compoundSelectorMatches(element, parts[0]);
    }
    
    // Match rightmost part against element, then walk up for combinators
    if (!compoundSelectorMatches(element, parts.back())) return false;
    
    DOMElement* current = element;
    for (int p = (int)parts.size() - 2; p >= 0; p--) {
        char comb = (p < (int)combinators.size()) ? combinators[p] : ' ';
        
        if (comb == '>') {
            // Child combinator — parent must match
            DOMNode* parentNode = current->parentNode();
            current = parentNode ? dynamic_cast<DOMElement*>(parentNode) : nullptr;
            if (!current || !compoundSelectorMatches(current, parts[p])) return false;
        } else {
            // Descendant combinator — any ancestor must match
            DOMNode* parentNode = current->parentNode();
            DOMElement* ancestor = parentNode ? dynamic_cast<DOMElement*>(parentNode) : nullptr;
            bool found = false;
            while (ancestor) {
                if (compoundSelectorMatches(ancestor, parts[p])) {
                    current = ancestor;
                    found = true;
                    break;
                }
                parentNode = ancestor->parentNode();
                ancestor = parentNode ? dynamic_cast<DOMElement*>(parentNode) : nullptr;
            }
            if (!found) return false;
        }
    }
    
    return true;
}

// Match a compound selector ("div.class#id[attr]") against a single element
bool CSSCascade::compoundSelectorMatches(DOMElement* element, const std::string& compound) {
    if (!element || compound.empty()) return false;
    
    // Universal selector
    if (compound == "*") return true;
    
    size_t i = 0;
    size_t len = compound.size();
    
    while (i < len) {
        if (compound[i] == '#') {
            // ID selector
            i++;
            size_t s = i;
            while (i < len && (std::isalnum(compound[i]) || compound[i] == '-' || compound[i] == '_')) i++;
            std::string id = compound.substr(s, i - s);
            if (element->getAttribute("id") != id) return false;
        }
        else if (compound[i] == '.') {
            // Class selector
            i++;
            size_t s = i;
            while (i < len && (std::isalnum(compound[i]) || compound[i] == '-' || compound[i] == '_')) i++;
            std::string className = compound.substr(s, i - s);
            std::string classes = element->getAttribute("class");
            // Word boundary match
            size_t pos = 0;
            bool found = false;
            while ((pos = classes.find(className, pos)) != std::string::npos) {
                bool startOK = (pos == 0 || classes[pos-1] == ' ');
                bool endOK = (pos + className.length() >= classes.length() || 
                              classes[pos + className.length()] == ' ');
                if (startOK && endOK) { found = true; break; }
                pos++;
            }
            if (!found) return false;
        }
        else if (compound[i] == '[') {
            // Attribute selector
            i++;
            size_t s = i;
            while (i < len && compound[i] != '=' && compound[i] != ']' && compound[i] != '~' && compound[i] != '|') i++;
            std::string attrName = compound.substr(s, i - s);
            
            if (i < len && compound[i] == ']') {
                // [attr] — has attribute
                i++;
                if (element->getAttribute(attrName).empty() && !element->hasAttribute(attrName)) return false;
            } else if (i < len && compound[i] == '=') {
                // [attr=value]
                i++;
                std::string attrVal;
                if (i < len && (compound[i] == '"' || compound[i] == '\'')) {
                    char q = compound[i++];
                    size_t vs = i;
                    while (i < len && compound[i] != q) i++;
                    attrVal = compound.substr(vs, i - vs);
                    if (i < len) i++;
                } else {
                    size_t vs = i;
                    while (i < len && compound[i] != ']') i++;
                    attrVal = compound.substr(vs, i - vs);
                }
                if (i < len && compound[i] == ']') i++;
                if (element->getAttribute(attrName) != attrVal) return false;
            } else {
                // Skip to end of attribute selector
                while (i < len && compound[i] != ']') i++;
                if (i < len) i++;
            }
        }
        else if (compound[i] == ':') {
            // Pseudo-class/element — skip for now (matches anything)
            i++;
            if (i < len && compound[i] == ':') i++;
            while (i < len && (std::isalnum(compound[i]) || compound[i] == '-')) i++;
            if (i < len && compound[i] == '(') {
                int depth = 1;
                i++;
                while (i < len && depth > 0) {
                    if (compound[i] == '(') depth++;
                    else if (compound[i] == ')') depth--;
                    i++;
                }
            }
        }
        else if (std::isalnum(compound[i]) || compound[i] == '-' || compound[i] == '_') {
            // Tag selector
            size_t s = i;
            while (i < len && (std::isalnum(compound[i]) || compound[i] == '-' || compound[i] == '_')) i++;
            std::string tag = compound.substr(s, i - s);
            std::string elemTag = element->tagName();
            // Case-insensitive comparison
            std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
            std::transform(elemTag.begin(), elemTag.end(), elemTag.begin(), ::tolower);
            if (elemTag != tag) return false;
        }
        else {
            i++;
        }
    }
    
    return true;
}

Selector::Specificity CSSCascade::calculateSpecificity(const std::string& selector) {
    Selector::Specificity spec = {0, 0, 0};
    
    size_t i = 0;
    while (i < selector.length()) {
        char c = selector[i];
        
        if (c == '#') {
            // ID selector (a)
            spec.a++;
            i++;
            while (i < selector.length() && 
                   (std::isalnum(selector[i]) || selector[i] == '-' || selector[i] == '_')) {
                i++;
            }
        } else if (c == '.') {
            // Class selector (b)
            spec.b++;
            i++;
            while (i < selector.length() && 
                   (std::isalnum(selector[i]) || selector[i] == '-' || selector[i] == '_')) {
                i++;
            }
        } else if (c == '[') {
            // Attribute selector (counts as b)
            spec.b++;
            while (i < selector.length() && selector[i] != ']') i++;
            if (i < selector.length()) i++;
        } else if (c == ':') {
            // Pseudo-class or pseudo-element
            i++;
            if (i < selector.length() && selector[i] == ':') {
                // Pseudo-element (counts as c)
                spec.c++;
                i++;
            } else {
                // Pseudo-class (counts as b, except :not, :is, :where)
                spec.b++;
            }
            while (i < selector.length() && std::isalpha(selector[i])) i++;
        } else if (std::isalpha(c)) {
            // Element selector (c)
            spec.c++;
            while (i < selector.length() && 
                   (std::isalnum(selector[i]) || selector[i] == '-')) {
                i++;
            }
        } else {
            i++;
        }
    }
    
    return spec;
}

} // namespace Zepra::WebCore
