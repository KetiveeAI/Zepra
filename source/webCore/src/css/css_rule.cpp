/**
 * @file css_rule.cpp
 * @brief CSS Rule hierarchy implementation
 */

#include "webcore/css/css_rule.hpp"
#include "webcore/css/css_style_sheet.hpp"
#include <sstream>

namespace Zepra::WebCore {

// =============================================================================
// CSSStyleRule
// =============================================================================

CSSStyleRule::CSSStyleRule()
    : style_(std::make_unique<CSSStyleDeclaration>(this)) {}

CSSStyleRule::~CSSStyleRule() = default;

void CSSStyleRule::setSelectorText(const std::string& selector) {
    selectorText_ = selector;
}

std::string CSSStyleRule::cssText() const {
    std::ostringstream oss;
    oss << selectorText_ << " { " << style_->cssText() << "}";
    return oss.str();
}

void CSSStyleRule::setCssText(const std::string& text) {
    // Find the opening brace
    size_t bracePos = text.find('{');
    if (bracePos == std::string::npos) return;
    
    // Extract selector
    selectorText_ = text.substr(0, bracePos);
    size_t start = selectorText_.find_first_not_of(" \t\n\r");
    size_t end = selectorText_.find_last_not_of(" \t\n\r");
    if (start != std::string::npos) {
        selectorText_ = selectorText_.substr(start, end - start + 1);
    }
    
    // Extract declarations
    size_t closePos = text.rfind('}');
    if (closePos != std::string::npos && closePos > bracePos) {
        std::string decls = text.substr(bracePos + 1, closePos - bracePos - 1);
        style_->setCssText(decls);
    }
}

// =============================================================================
// CSSMediaRule
// =============================================================================

CSSMediaRule::CSSMediaRule()
    : media_(std::make_unique<MediaList>())
    , cssRules_(std::make_unique<CSSRuleList>()) {}

CSSMediaRule::~CSSMediaRule() = default;

void CSSMediaRule::setConditionText(const std::string& condition) {
    conditionText_ = condition;
    media_->setMediaText(condition);
}

size_t CSSMediaRule::insertRule(const std::string& /*rule*/, size_t index) {
    return index;
}

void CSSMediaRule::deleteRule(size_t index) {
    cssRules_->remove(index);
}

std::string CSSMediaRule::cssText() const {
    std::ostringstream oss;
    oss << "@media " << conditionText_ << " {\n";
    for (size_t i = 0; i < cssRules_->length(); ++i) {
        oss << "  " << cssRules_->item(i)->cssText() << "\n";
    }
    oss << "}";
    return oss.str();
}

void CSSMediaRule::setCssText(const std::string& /*text*/) {
    // Would parse @media rule
}

// =============================================================================
// CSSFontFaceRule
// =============================================================================

CSSFontFaceRule::CSSFontFaceRule()
    : style_(std::make_unique<CSSStyleDeclaration>(this)) {}

CSSFontFaceRule::~CSSFontFaceRule() = default;

std::string CSSFontFaceRule::cssText() const {
    std::ostringstream oss;
    oss << "@font-face { " << style_->cssText() << "}";
    return oss.str();
}

void CSSFontFaceRule::setCssText(const std::string& text) {
    size_t bracePos = text.find('{');
    size_t closePos = text.rfind('}');
    if (bracePos != std::string::npos && closePos != std::string::npos) {
        std::string decls = text.substr(bracePos + 1, closePos - bracePos - 1);
        style_->setCssText(decls);
    }
}

// =============================================================================
// CSSKeyframesRule
// =============================================================================

CSSKeyframesRule::CSSKeyframesRule()
    : cssRules_(std::make_unique<CSSRuleList>()) {}

CSSKeyframesRule::~CSSKeyframesRule() = default;

CSSKeyframeRule* CSSKeyframesRule::findRule(const std::string& key) const {
    for (size_t i = 0; i < cssRules_->length(); ++i) {
        auto* rule = dynamic_cast<CSSKeyframeRule*>(cssRules_->item(i));
        if (rule && rule->keyText() == key) {
            return rule;
        }
    }
    return nullptr;
}

void CSSKeyframesRule::appendRule(const std::string& /*rule*/) {
    // Would parse and add keyframe
}

void CSSKeyframesRule::deleteRule(const std::string& key) {
    for (size_t i = 0; i < cssRules_->length(); ++i) {
        auto* rule = dynamic_cast<CSSKeyframeRule*>(cssRules_->item(i));
        if (rule && rule->keyText() == key) {
            cssRules_->remove(i);
            return;
        }
    }
}

std::string CSSKeyframesRule::cssText() const {
    std::ostringstream oss;
    oss << "@keyframes " << name_ << " {\n";
    for (size_t i = 0; i < cssRules_->length(); ++i) {
        oss << "  " << cssRules_->item(i)->cssText() << "\n";
    }
    oss << "}";
    return oss.str();
}

void CSSKeyframesRule::setCssText(const std::string& /*text*/) {
    // Would parse @keyframes rule
}

// =============================================================================
// CSSKeyframeRule
// =============================================================================

CSSKeyframeRule::CSSKeyframeRule()
    : style_(std::make_unique<CSSStyleDeclaration>(this)) {}

CSSKeyframeRule::~CSSKeyframeRule() = default;

std::string CSSKeyframeRule::cssText() const {
    std::ostringstream oss;
    oss << keyText_ << " { " << style_->cssText() << "}";
    return oss.str();
}

void CSSKeyframeRule::setCssText(const std::string& text) {
    size_t bracePos = text.find('{');
    if (bracePos == std::string::npos) return;
    
    keyText_ = text.substr(0, bracePos);
    size_t start = keyText_.find_first_not_of(" \t\n\r");
    size_t end = keyText_.find_last_not_of(" \t\n\r");
    if (start != std::string::npos) {
        keyText_ = keyText_.substr(start, end - start + 1);
    }
    
    size_t closePos = text.rfind('}');
    if (closePos != std::string::npos) {
        std::string decls = text.substr(bracePos + 1, closePos - bracePos - 1);
        style_->setCssText(decls);
    }
}

// =============================================================================
// CSSImportRule
// =============================================================================

CSSImportRule::CSSImportRule()
    : media_(std::make_unique<MediaList>()) {}

CSSImportRule::~CSSImportRule() = default;

std::string CSSImportRule::cssText() const {
    std::ostringstream oss;
    oss << "@import url(\"" << href_ << "\")";
    if (media_->length() > 0) {
        oss << " " << media_->mediaText();
    }
    oss << ";";
    return oss.str();
}

void CSSImportRule::setCssText(const std::string& /*text*/) {
    // Would parse @import rule
}

// =============================================================================
// CSSSupportsRule
// =============================================================================

CSSSupportsRule::CSSSupportsRule()
    : cssRules_(std::make_unique<CSSRuleList>()) {}

CSSSupportsRule::~CSSSupportsRule() = default;

std::string CSSSupportsRule::cssText() const {
    std::ostringstream oss;
    oss << "@supports " << conditionText_ << " {\n";
    for (size_t i = 0; i < cssRules_->length(); ++i) {
        oss << "  " << cssRules_->item(i)->cssText() << "\n";
    }
    oss << "}";
    return oss.str();
}

void CSSSupportsRule::setCssText(const std::string& /*text*/) {
    // Would parse @supports rule
}

} // namespace Zepra::WebCore
