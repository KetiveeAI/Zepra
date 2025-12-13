/**
 * @file css_style_sheet.cpp
 * @brief CSSOM StyleSheet implementation
 */

#include "webcore/css/css_style_sheet.hpp"
#include "webcore/css/css_rule.hpp"
#include <algorithm>
#include <sstream>

namespace Zepra::WebCore {

// =============================================================================
// StyleSheetList
// =============================================================================

StyleSheet* StyleSheetList::item(size_t index) const {
    if (index >= sheets_.size()) return nullptr;
    return sheets_[index].get();
}

void StyleSheetList::add(std::shared_ptr<StyleSheet> sheet) {
    sheets_.push_back(std::move(sheet));
}

void StyleSheetList::remove(StyleSheet* sheet) {
    sheets_.erase(
        std::remove_if(sheets_.begin(), sheets_.end(),
            [sheet](const auto& s) { return s.get() == sheet; }),
        sheets_.end());
}

// =============================================================================
// CSSRuleList
// =============================================================================

CSSRule* CSSRuleList::item(size_t index) const {
    if (index >= rules_.size()) return nullptr;
    return rules_[index].get();
}

void CSSRuleList::add(std::unique_ptr<CSSRule> rule) {
    rules_.push_back(std::move(rule));
}

std::unique_ptr<CSSRule> CSSRuleList::remove(size_t index) {
    if (index >= rules_.size()) return nullptr;
    
    auto rule = std::move(rules_[index]);
    rules_.erase(rules_.begin() + index);
    return rule;
}

void CSSRuleList::insertAt(size_t index, std::unique_ptr<CSSRule> rule) {
    if (index >= rules_.size()) {
        rules_.push_back(std::move(rule));
    } else {
        rules_.insert(rules_.begin() + index, std::move(rule));
    }
}

// =============================================================================
// CSSStyleSheet
// =============================================================================

CSSStyleSheet::CSSStyleSheet()
    : cssRules_(std::make_unique<CSSRuleList>()) {}

CSSStyleSheet::~CSSStyleSheet() = default;

size_t CSSStyleSheet::insertRule(const std::string& /*rule*/, size_t index) {
    // Would parse and insert rule
    return index;
}

void CSSStyleSheet::deleteRule(size_t index) {
    if (cssRules_) {
        cssRules_->remove(index);
    }
}

void CSSStyleSheet::replaceSync(const std::string& text) {
    parseContent(text);
}

void CSSStyleSheet::parseContent(const std::string& /*cssText*/) {
    // Would parse CSS and populate rules
}

// =============================================================================
// MediaList
// =============================================================================

void MediaList::setMediaText(const std::string& text) {
    mediaText_ = text;
    queries_.clear();
    
    // Simple parsing: split by comma
    std::istringstream iss(text);
    std::string query;
    while (std::getline(iss, query, ',')) {
        // Trim whitespace
        size_t start = query.find_first_not_of(" \t\n\r");
        size_t end = query.find_last_not_of(" \t\n\r");
        if (start != std::string::npos) {
            queries_.push_back(query.substr(start, end - start + 1));
        }
    }
}

std::string MediaList::item(size_t index) const {
    if (index >= queries_.size()) return "";
    return queries_[index];
}

void MediaList::appendMedium(const std::string& medium) {
    queries_.push_back(medium);
    
    // Rebuild media text
    mediaText_.clear();
    for (size_t i = 0; i < queries_.size(); ++i) {
        if (i > 0) mediaText_ += ", ";
        mediaText_ += queries_[i];
    }
}

void MediaList::deleteMedium(const std::string& medium) {
    queries_.erase(
        std::remove(queries_.begin(), queries_.end(), medium),
        queries_.end());
    
    // Rebuild media text
    mediaText_.clear();
    for (size_t i = 0; i < queries_.size(); ++i) {
        if (i > 0) mediaText_ += ", ";
        mediaText_ += queries_[i];
    }
}

bool MediaList::matches(int viewportWidth, int viewportHeight) const {
    if (queries_.empty()) return true;  // No media query = matches all
    
    for (const auto& query : queries_) {
        // Simple matching for common queries
        if (query == "all" || query == "screen") {
            return true;
        }
        
        // Parse min-width/max-width
        size_t minWidthPos = query.find("min-width:");
        if (minWidthPos != std::string::npos) {
            size_t start = minWidthPos + 10;
            int minWidth = std::stoi(query.substr(start));
            if (viewportWidth >= minWidth) return true;
        }
        
        size_t maxWidthPos = query.find("max-width:");
        if (maxWidthPos != std::string::npos) {
            size_t start = maxWidthPos + 10;
            int maxWidth = std::stoi(query.substr(start));
            if (viewportWidth <= maxWidth) return true;
        }
    }
    
    (void)viewportHeight;
    return false;
}

} // namespace Zepra::WebCore
