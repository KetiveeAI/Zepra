/**
 * @file settings_store.h
 * @brief Browser preferences and settings
 */

#pragma once

#include <string>
#include <map>
#include <variant>

namespace ZepraBrowser {

using SettingValue = std::variant<bool, int, std::string>;

enum class SearchEngine {
    Ketivee,
    Google,
    Bing,
    DuckDuckGo,
    Yahoo
};

struct Settings {
    // Search
    SearchEngine defaultSearchEngine = SearchEngine::Ketivee;
    
    // Privacy
    bool trackHistory = true;
    bool acceptCookies = true;
    bool doNotTrack = false;
    
    // Appearance
    bool darkMode = false;
    int fontSize = 14;
    std::string fontFamily = "Arial";
    
    // Downloads
    std::string downloadPath = "~/Downloads";
    bool askWhereToSave = true;
    
    // Advanced
    bool enableJavaScript = true;
    bool enableImages = true;
    bool enablePlugins = false;
};

/**
 * SettingsStore - Browser preferences
 */
class SettingsStore {
public:
    static SettingsStore& instance();
    
    // Get/Set
    Settings getSettings() const;
    void setSettings(const Settings& settings);
    
    // Individual settings
    SearchEngine getSearchEngine() const;
    void setSearchEngine(SearchEngine engine);
    
    bool isDarkMode() const;
    void setDarkMode(bool enabled);
    
    // Persistence
    bool load(const std::string& filepath);
    bool save(const std::string& filepath);
    
private:
    SettingsStore();
    ~SettingsStore();
    SettingsStore(const SettingsStore&) = delete;
    SettingsStore& operator=(const SettingsStore&) = delete;
    
    Settings settings_;
    std::string filepath_;
};

} // namespace ZepraBrowser
