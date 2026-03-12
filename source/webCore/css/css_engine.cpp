// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file css_engine.cpp
 * @brief CSS engine minimal stub implementation
 */

#include "css/css_engine.hpp"

namespace Zepra::WebCore {

// Forward declarations
class DOMElement;
class DOMDocument;

// MatchedRule comparison
bool MatchedRule::operator<(const MatchedRule& other) const {
    if (static_cast<int>(origin) != static_cast<int>(other.origin))
        return static_cast<int>(origin) < static_cast<int>(other.origin);
    return order < other.order;
}

// CSSCascade - minimal stubs
std::vector<MatchedRule> CSSCascade::collectMatchingRules(
    DOMElement*, const std::vector<CSSStyleSheet*>&, StyleOrigin) {
    return {};
}

void CSSCascade::sortByCascade(std::vector<MatchedRule>& rules) {
    std::sort(rules.begin(), rules.end());
}

std::optional<std::string> CSSCascade::cascadedValue(
    const std::string&, const std::vector<MatchedRule>&) {
    return std::nullopt;
}

bool CSSCascade::selectorMatches(DOMElement*, const std::string&) {
    return false;
}

Selector::Specificity CSSCascade::calculateSpecificity(const std::string&) {
    return {};
}

// StyleResolver - minimal stubs
StyleResolver::StyleResolver() {}
StyleResolver::~StyleResolver() {}

void StyleResolver::addStyleSheet(std::shared_ptr<CSSStyleSheet>, StyleOrigin) {}
void StyleResolver::addUserAgentStyleSheet() {}

CSSComputedStyle StyleResolver::computeStyle(DOMElement*, const CSSComputedStyle*) {
    return CSSComputedStyle();
}

const CSSComputedStyle* StyleResolver::getComputedStyle(DOMElement* element) {
    auto it = styleCache_.find(element);
    if (it != styleCache_.end()) return &it->second;
    styleCache_[element] = CSSComputedStyle();
    return &styleCache_[element];
}

void StyleResolver::invalidateElement(DOMElement* element) {
    styleCache_.erase(element);
}

void StyleResolver::invalidateAll() {
    styleCache_.clear();
}

void StyleResolver::applyDeclarations(CSSComputedStyle&, const CSSStyleDeclaration*, const CSSComputedStyle*) {}
void StyleResolver::applyProperty(CSSComputedStyle&, const std::string&, const std::string&, const CSSComputedStyle*) {}

CSSLength StyleResolver::parseLength(const std::string&) {
    return CSSLength();
}

CSSColor StyleResolver::parseColor(const std::string&) {
    return CSSColor();
}

// CSSParser - minimal stubs
std::unique_ptr<CSSStyleSheet> CSSParser::parse(const std::string&) {
    return std::make_unique<CSSStyleSheet>();
}

std::unique_ptr<CSSStyleDeclaration> CSSParser::parseInlineStyle(const std::string&) {
    return std::make_unique<CSSStyleDeclaration>();
}

Selector CSSParser::parseSelector(const std::string&) {
    return Selector();
}

std::unique_ptr<MediaList> CSSParser::parseMediaQuery(const std::string&) {
    return std::make_unique<MediaList>();
}

void CSSParser::skipWhitespace() {}
void CSSParser::skipComment() {}
char CSSParser::peek() const { return '\0'; }
char CSSParser::consume() { return '\0'; }
bool CSSParser::match(char) { return false; }
bool CSSParser::eof() const { return true; }

std::unique_ptr<CSSRule> CSSParser::parseRule() { return nullptr; }
std::unique_ptr<CSSStyleRule> CSSParser::parseStyleRule() { return nullptr; }
std::unique_ptr<CSSMediaRule> CSSParser::parseMediaRule() { return nullptr; }
std::unique_ptr<CSSFontFaceRule> CSSParser::parseFontFaceRule() { return nullptr; }
std::unique_ptr<CSSKeyframesRule> CSSParser::parseKeyframesRule() { return nullptr; }
std::unique_ptr<CSSImportRule> CSSParser::parseImportRule() { return nullptr; }
std::string CSSParser::parseSelector_() { return ""; }
std::string CSSParser::parseDeclarationBlock() { return ""; }
void CSSParser::parseDeclarations(CSSStyleDeclaration*) {}
std::string CSSParser::parsePropertyValue() { return ""; }
std::string CSSParser::parseString() { return ""; }
std::string CSSParser::parseIdentifier() { return ""; }
std::string CSSParser::parseUrl() { return ""; }

// CSSEngine - minimal stubs
CSSEngine::CSSEngine() {}
CSSEngine::~CSSEngine() {}

void CSSEngine::initialize(DOMDocument* doc) { document_ = doc; }
void CSSEngine::addStyleSheet(const std::string&, StyleOrigin) {}
void CSSEngine::addStyleSheetFromUrl(const std::string&, StyleOrigin) {}
void CSSEngine::computeStyles() {}

const CSSComputedStyle* CSSEngine::getComputedStyle(DOMElement* element) {
    return resolver_.getComputedStyle(element);
}

void CSSEngine::invalidate(DOMElement*) {}
bool CSSEngine::supports(const std::string&, const std::string&) { return true; }
bool CSSEngine::supportsCondition(const std::string&) { return true; }
std::string CSSEngine::escape(const std::string& ident) { return ident; }
bool CSSEngine::supportsPropertySyntax(const std::string&) { return true; }

} // namespace Zepra::WebCore
