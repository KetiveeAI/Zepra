// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file searchbox.cpp
 * @brief Search/Address bar implementation
 */

#include "../../source/zepraEngine/include/engine/ui/searchbox.h"
#include <algorithm>
#include <regex>
#include <cctype>

namespace zepra {
namespace ui {

// URL validation regex patterns
static const std::regex URL_PATTERN(
    R"(^(https?://)?([a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?\.)+[a-zA-Z]{2,}(/.*)?$)",
    std::regex::icase
);

static const std::regex LOCALHOST_PATTERN(
    R"(^(https?://)?localhost(:\d+)?(/.*)?$)",
    std::regex::icase
);

static const std::regex IP_PATTERN(
    R"(^(https?://)?(\d{1,3}\.){3}\d{1,3}(:\d+)?(/.*)?$)",
    std::regex::icase
);

// Implementation details
struct SearchBox::Impl {
    SearchBoxConfig config;
    std::string text;
    std::string displayUrl;
    bool focused = false;
    bool voiceActive = false;
    int cursorPosition = 0;
    int selectionStart = -1;
    int selectionEnd = -1;
    
    float x = 0, y = 0, width = 400, height = 48;
    
    std::vector<Suggestion> suggestions;
    int selectedSuggestion = -1;
    bool showSuggestions = false;
    
    SubmitCallback submitCallback;
    VoiceCallback voiceCallback;
    FocusCallback focusCallback;
    SuggestionCallback suggestionCallback;
    
    Impl() : config() {}
    explicit Impl(const SearchBoxConfig& cfg) : config(cfg) {}
};

SearchBox::SearchBox() : impl_(std::make_unique<Impl>()) {}

SearchBox::SearchBox(const SearchBoxConfig& config) 
    : impl_(std::make_unique<Impl>(config)) {}

SearchBox::~SearchBox() = default;

void SearchBox::setConfig(const SearchBoxConfig& config) {
    impl_->config = config;
}

SearchBoxConfig SearchBox::getConfig() const {
    return impl_->config;
}

void SearchBox::setText(const std::string& text) {
    impl_->text = text;
    impl_->cursorPosition = static_cast<int>(text.length());
    impl_->selectionStart = -1;
    impl_->selectionEnd = -1;
    
    // Fetch suggestions if callback set
    if (impl_->suggestionCallback && impl_->config.enableAutocomplete) {
        impl_->suggestions = impl_->suggestionCallback(text);
        impl_->selectedSuggestion = -1;
        impl_->showSuggestions = !impl_->suggestions.empty();
    }
}

std::string SearchBox::getText() const {
    return impl_->text;
}

void SearchBox::clear() {
    impl_->text.clear();
    impl_->cursorPosition = 0;
    impl_->selectionStart = -1;
    impl_->selectionEnd = -1;
    impl_->suggestions.clear();
    impl_->showSuggestions = false;
}

void SearchBox::selectAll() {
    impl_->selectionStart = 0;
    impl_->selectionEnd = static_cast<int>(impl_->text.length());
    impl_->cursorPosition = impl_->selectionEnd;
}

void SearchBox::focus() {
    if (!impl_->focused) {
        impl_->focused = true;
        if (impl_->focusCallback) {
            impl_->focusCallback(true);
        }
    }
}

void SearchBox::blur() {
    if (impl_->focused) {
        impl_->focused = false;
        impl_->showSuggestions = false;
        if (impl_->focusCallback) {
            impl_->focusCallback(false);
        }
    }
}

bool SearchBox::isFocused() const {
    return impl_->focused;
}

void SearchBox::setDisplayUrl(const std::string& url) {
    impl_->displayUrl = url;
}

std::string SearchBox::getDisplayUrl() const {
    return impl_->displayUrl;
}

void SearchBox::setSuggestions(const std::vector<Suggestion>& suggestions) {
    impl_->suggestions = suggestions;
    impl_->selectedSuggestion = -1;
    impl_->showSuggestions = !suggestions.empty() && impl_->focused;
}

void SearchBox::clearSuggestions() {
    impl_->suggestions.clear();
    impl_->selectedSuggestion = -1;
    impl_->showSuggestions = false;
}

void SearchBox::setSuggestionCallback(SuggestionCallback callback) {
    impl_->suggestionCallback = std::move(callback);
}

void SearchBox::setSubmitCallback(SubmitCallback callback) {
    impl_->submitCallback = std::move(callback);
}

void SearchBox::setVoiceCallback(VoiceCallback callback) {
    impl_->voiceCallback = std::move(callback);
}

void SearchBox::setFocusCallback(FocusCallback callback) {
    impl_->focusCallback = std::move(callback);
}

void SearchBox::setBounds(float x, float y, float width, float height) {
    impl_->x = x;
    impl_->y = y;
    impl_->width = width;
    impl_->height = height;
}

void SearchBox::render() {
    // Note: Actual rendering is done by the browser's OpenGL code
    // This is a logical component that provides state and callbacks
}

bool SearchBox::handleKeyPress(int keyCode, bool ctrl, bool shift, bool alt) {
    if (!impl_->focused) return false;
    
    // Handle Enter key
    if (keyCode == '\r' || keyCode == '\n') {
        if (impl_->showSuggestions && impl_->selectedSuggestion >= 0) {
            // Use selected suggestion
            const auto& suggestion = impl_->suggestions[impl_->selectedSuggestion];
            impl_->text = suggestion.url.empty() ? suggestion.text : suggestion.url;
        }
        
        if (impl_->submitCallback && !impl_->text.empty()) {
            impl_->submitCallback(impl_->text, isUrl());
        }
        impl_->showSuggestions = false;
        return true;
    }
    
    // Handle Escape
    if (keyCode == 27) { // ESC
        if (impl_->showSuggestions) {
            impl_->showSuggestions = false;
        } else {
            blur();
        }
        return true;
    }
    
    // Handle arrow keys for suggestion navigation
    if (impl_->showSuggestions) {
        if (keyCode == 0xFF54) { // Down
            impl_->selectedSuggestion = std::min(
                impl_->selectedSuggestion + 1,
                static_cast<int>(impl_->suggestions.size()) - 1
            );
            return true;
        }
        if (keyCode == 0xFF52) { // Up
            impl_->selectedSuggestion = std::max(impl_->selectedSuggestion - 1, -1);
            return true;
        }
    }
    
    // Handle Ctrl+A (select all)
    if (ctrl && (keyCode == 'a' || keyCode == 'A')) {
        selectAll();
        return true;
    }
    
    // Handle Ctrl+C (copy)
    if (ctrl && (keyCode == 'c' || keyCode == 'C')) {
        // Copy handled by platform
        return true;
    }
    
    // Handle Ctrl+V (paste)
    if (ctrl && (keyCode == 'v' || keyCode == 'V')) {
        // Paste handled by platform
        return true;
    }
    
    // Handle Ctrl+X (cut)
    if (ctrl && (keyCode == 'x' || keyCode == 'X')) {
        // Cut handled by platform
        return true;
    }
    
    // Handle Backspace
    if (keyCode == 0xFF08) {
        if (impl_->selectionStart >= 0 && impl_->selectionStart != impl_->selectionEnd) {
            // Delete selection
            int start = std::min(impl_->selectionStart, impl_->selectionEnd);
            int end = std::max(impl_->selectionStart, impl_->selectionEnd);
            impl_->text.erase(start, end - start);
            impl_->cursorPosition = start;
            impl_->selectionStart = -1;
            impl_->selectionEnd = -1;
        } else if (impl_->cursorPosition > 0) {
            impl_->text.erase(impl_->cursorPosition - 1, 1);
            impl_->cursorPosition--;
        }
        
        // Update suggestions
        if (impl_->suggestionCallback && impl_->config.enableAutocomplete) {
            impl_->suggestions = impl_->suggestionCallback(impl_->text);
            impl_->selectedSuggestion = -1;
            impl_->showSuggestions = !impl_->suggestions.empty();
        }
        return true;
    }
    
    // Handle Delete
    if (keyCode == 0xFFFF) {
        if (impl_->selectionStart >= 0 && impl_->selectionStart != impl_->selectionEnd) {
            // Delete selection
            int start = std::min(impl_->selectionStart, impl_->selectionEnd);
            int end = std::max(impl_->selectionStart, impl_->selectionEnd);
            impl_->text.erase(start, end - start);
            impl_->cursorPosition = start;
            impl_->selectionStart = -1;
            impl_->selectionEnd = -1;
        } else if (impl_->cursorPosition < static_cast<int>(impl_->text.length())) {
            impl_->text.erase(impl_->cursorPosition, 1);
        }
        return true;
    }
    
    // Handle Left/Right arrows
    if (keyCode == 0xFF51) { // Left
        if (impl_->cursorPosition > 0) {
            impl_->cursorPosition--;
        }
        if (!shift) {
            impl_->selectionStart = -1;
            impl_->selectionEnd = -1;
        }
        return true;
    }
    if (keyCode == 0xFF53) { // Right
        if (impl_->cursorPosition < static_cast<int>(impl_->text.length())) {
            impl_->cursorPosition++;
        }
        if (!shift) {
            impl_->selectionStart = -1;
            impl_->selectionEnd = -1;
        }
        return true;
    }
    
    // Handle Home/End
    if (keyCode == 0xFF50) { // Home
        impl_->cursorPosition = 0;
        if (!shift) {
            impl_->selectionStart = -1;
            impl_->selectionEnd = -1;
        }
        return true;
    }
    if (keyCode == 0xFF57) { // End
        impl_->cursorPosition = static_cast<int>(impl_->text.length());
        if (!shift) {
            impl_->selectionStart = -1;
            impl_->selectionEnd = -1;
        }
        return true;
    }
    
    return false;
}

bool SearchBox::handleTextInput(const std::string& text) {
    if (!impl_->focused) return false;
    
    // Delete selection if any
    if (impl_->selectionStart >= 0 && impl_->selectionStart != impl_->selectionEnd) {
        int start = std::min(impl_->selectionStart, impl_->selectionEnd);
        int end = std::max(impl_->selectionStart, impl_->selectionEnd);
        impl_->text.erase(start, end - start);
        impl_->cursorPosition = start;
        impl_->selectionStart = -1;
        impl_->selectionEnd = -1;
    }
    
    // Insert text at cursor
    impl_->text.insert(impl_->cursorPosition, text);
    impl_->cursorPosition += static_cast<int>(text.length());
    
    // Update suggestions
    if (impl_->suggestionCallback && impl_->config.enableAutocomplete) {
        impl_->suggestions = impl_->suggestionCallback(impl_->text);
        impl_->selectedSuggestion = -1;
        impl_->showSuggestions = !impl_->suggestions.empty();
    }
    
    return true;
}

bool SearchBox::handleMouseClick(float x, float y) {
    float right = impl_->x + impl_->width;
    float bottom = impl_->y + impl_->height;
    
    // Check if click is within bounds
    if (x >= impl_->x && x <= right && y >= impl_->y && y <= bottom) {
        focus();
        
        // Check if mic icon clicked (right side)
        if (impl_->config.showMicIcon) {
            float micX = right - impl_->height;
            if (x >= micX) {
                if (impl_->voiceCallback) {
                    impl_->voiceCallback();
                }
                return true;
            }
        }
        
        // TODO: Calculate cursor position from x coordinate
        return true;
    }
    
    // Click outside - close suggestions
    impl_->showSuggestions = false;
    return false;
}

bool SearchBox::handleMouseMove(float x, float y) {
    // Handle hover states for icons
    return false;
}

bool SearchBox::isValid() const {
    return !impl_->text.empty();
}

bool SearchBox::isUrl() const {
    return ::zepra::ui::isValidUrl(impl_->text);
}

bool SearchBox::isVoiceActive() const {
    return impl_->voiceActive;
}

// Utility functions
bool isValidUrl(const std::string& text) {
    if (text.empty()) return false;
    
    // Check for common URL patterns
    if (std::regex_match(text, URL_PATTERN)) return true;
    if (std::regex_match(text, LOCALHOST_PATTERN)) return true;
    if (std::regex_match(text, IP_PATTERN)) return true;
    
    // Check for internal URLs
    if (text.find("zepra://") == 0) return true;
    if (text.find("file://") == 0) return true;
    
    return false;
}

std::string normalizeUrl(const std::string& url) {
    std::string result = url;
    
    // Trim whitespace
    auto start = result.find_first_not_of(" \t\n\r");
    auto end = result.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    result = result.substr(start, end - start + 1);
    
    // Add https:// if no protocol
    if (isValidUrl(result)) {
        if (result.find("://") == std::string::npos) {
            result = "https://" + result;
        }
    }
    
    return result;
}

std::string formatDisplayUrl(const std::string& url) {
    std::string display = url;
    
    // Remove protocol for display
    size_t pos = display.find("://");
    if (pos != std::string::npos) {
        display = display.substr(pos + 3);
    }
    
    // Remove trailing slash
    if (!display.empty() && display.back() == '/') {
        display.pop_back();
    }
    
    // Remove www.
    if (display.find("www.") == 0) {
        display = display.substr(4);
    }
    
    return display;
}

} // namespace ui
} // namespace zepra
