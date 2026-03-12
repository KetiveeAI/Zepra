// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file css_style_sheet.cpp
 * @brief CSS StyleSheet implementation stubs
 */

#include "css/css_style_sheet.hpp"
#include "css/css_rule.hpp"

namespace Zepra::WebCore {

// StyleSheetList
StyleSheet* StyleSheetList::item(size_t index) const {
    if (index < sheets_.size()) {
        return sheets_[index].get();
    }
    return nullptr;
}

void StyleSheetList::add(std::shared_ptr<StyleSheet> sheet) {
    sheets_.push_back(std::move(sheet));
}

void StyleSheetList::remove(StyleSheet* sheet) {
    sheets_.erase(
        std::remove_if(sheets_.begin(), sheets_.end(),
            [sheet](const std::shared_ptr<StyleSheet>& s) { return s.get() == sheet; }),
        sheets_.end()
    );
}

// CSSRuleList
CSSRule* CSSRuleList::item(size_t index) const {
    if (index < rules_.size()) {
        return rules_[index].get();
    }
    return nullptr;
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

// CSSStyleSheet
CSSStyleSheet::CSSStyleSheet()
    : cssRules_(std::make_unique<CSSRuleList>()) {}

CSSStyleSheet::~CSSStyleSheet() = default;

size_t CSSStyleSheet::insertRule(const std::string& rule, size_t index) {
    // Stub: would parse and insert rule
    return index;
}

void CSSStyleSheet::deleteRule(size_t index) {
    if (cssRules_) {
        cssRules_->remove(index);
    }
}

void CSSStyleSheet::replaceSync(const std::string& text) {
    // Clear and reparse
    cssRules_ = std::make_unique<CSSRuleList>();
    parseContent(text);
}

void CSSStyleSheet::parseContent(const std::string& cssText) {
    // Stub: would parse CSS text into rules
}

// MediaList
void MediaList::setMediaText(const std::string& text) {
    mediaText_ = text;
    queries_.clear();
    // Parse media queries from text - stub
}

std::string MediaList::item(size_t index) const {
    if (index < queries_.size()) {
        return queries_[index];
    }
    return "";
}

void MediaList::appendMedium(const std::string& medium) {
    queries_.push_back(medium);
    if (!mediaText_.empty()) mediaText_ += ", ";
    mediaText_ += medium;
}

void MediaList::deleteMedium(const std::string& medium) {
    queries_.erase(
        std::remove(queries_.begin(), queries_.end(), medium),
        queries_.end()
    );
}

bool MediaList::matches(int viewportWidth, int viewportHeight) const {
    // Stub: assume all media queries match
    return true;
}

} // namespace Zepra::WebCore
