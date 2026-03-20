// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file searchbox.h
 * @brief Search/Address bar component with autocomplete and voice input support
 */

#ifndef ZEPRA_UI_SEARCHBOX_H
#define ZEPRA_UI_SEARCHBOX_H

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace zepra {
namespace ui {

// Forward declarations
class Dropdown;

/**
 * Autocomplete suggestion item
 */
struct Suggestion {
    std::string text;
    std::string url;
    std::string icon;      // Path to icon SVG
    std::string category;  // "history", "bookmark", "search"
    int priority;
};

/**
 * SearchBox configuration
 */
struct SearchBoxConfig {
    std::string placeholder = "Search or enter URL";
    bool showSearchIcon = true;
    bool showMicIcon = true;
    bool enableAutocomplete = true;
    int maxSuggestions = 8;
    float borderRadius = 24.0f;
    float height = 48.0f;
};

/**
 * SearchBox - Browser address/search bar component
 * 
 * Features:
 * - URL/search input with validation
 * - Autocomplete dropdown
 * - Search and mic icons
 * - Focus/blur animations
 * - Keyboard navigation
 */
class SearchBox {
public:
    using SubmitCallback = std::function<void(const std::string& text, bool isUrl)>;
    using VoiceCallback = std::function<void()>;
    using FocusCallback = std::function<void(bool focused)>;
    using SuggestionCallback = std::function<std::vector<Suggestion>(const std::string& query)>;

    SearchBox();
    explicit SearchBox(const SearchBoxConfig& config);
    ~SearchBox();

    // Configuration
    void setConfig(const SearchBoxConfig& config);
    SearchBoxConfig getConfig() const;

    // Text operations
    void setText(const std::string& text);
    std::string getText() const;
    void clear();
    void selectAll();

    // Focus management
    void focus();
    void blur();
    bool isFocused() const;

    // URL display helpers
    void setDisplayUrl(const std::string& url);
    std::string getDisplayUrl() const;

    // Autocomplete
    void setSuggestions(const std::vector<Suggestion>& suggestions);
    void clearSuggestions();
    void setSuggestionCallback(SuggestionCallback callback);

    // Callbacks
    void setSubmitCallback(SubmitCallback callback);
    void setVoiceCallback(VoiceCallback callback);
    void setFocusCallback(FocusCallback callback);

    // Rendering
    void setBounds(float x, float y, float width, float height);
    void render();
    
    // Event handling
    bool handleKeyPress(int keyCode, bool ctrl, bool shift, bool alt);
    bool handleTextInput(const std::string& text);
    bool handleMouseClick(float x, float y);
    bool handleMouseMove(float x, float y);

    // State
    bool isValid() const;       // True if valid URL or search query
    bool isUrl() const;         // True if input appears to be a URL
    bool isVoiceActive() const; // True if voice input is active

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// URL utility functions
bool isValidUrl(const std::string& text);
std::string normalizeUrl(const std::string& url);
std::string formatDisplayUrl(const std::string& url);

} // namespace ui
} // namespace zepra

#endif // ZEPRA_UI_SEARCHBOX_H
