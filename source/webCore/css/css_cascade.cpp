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
#include "webcore/browser/dom.hpp"
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
    
    // Split compound selectors
    // Tag selector: "div", "p", etc.
    // Class selector: ".class-name"
    // ID selector: "#id-name"
    // Universal: "*"
    
    std::string sel = selector;
    
    // Trim whitespace
    size_t start = sel.find_first_not_of(" \t\n\r");
    size_t end = sel.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) return false;
    sel = sel.substr(start, end - start + 1);
    
    // Universal selector matches everything
    if (sel == "*") return true;
    
    // ID selector
    if (sel[0] == '#') {
        std::string id = sel.substr(1);
        return element->getAttribute("id") == id;
    }
    
    // Class selector
    if (sel[0] == '.') {
        std::string className = sel.substr(1);
        std::string classes = element->getAttribute("class");
        
        // Check if className is in the space-separated class list
        size_t pos = 0;
        while ((pos = classes.find(className, pos)) != std::string::npos) {
            bool startOK = (pos == 0 || classes[pos-1] == ' ');
            bool endOK = (pos + className.length() >= classes.length() || 
                          classes[pos + className.length()] == ' ');
            if (startOK && endOK) return true;
            pos++;
        }
        return false;
    }
    
    // Tag selector (case insensitive)
    std::string tag = element->tagName();
    std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
    std::transform(sel.begin(), sel.end(), sel.begin(), ::tolower);
    return tag == sel;
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
