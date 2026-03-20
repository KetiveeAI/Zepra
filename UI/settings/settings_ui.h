// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#ifndef ZEPRA_SETTINGS_UI_H
#define ZEPRA_SETTINGS_UI_H

/**
 * @file settings_ui.h
 * @brief Modern browser settings panel with sections
 * 
 * Integrates with ZebraScript's secure storage for passwords
 * and encrypted user data.
 */

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace zepra {
namespace ui {

/**
 * @brief Settings section identifiers
 */
enum class SettingsSection {
    General,
    Privacy,
    Security,
    Passwords,
    Audio,          // NXAudio integration
    Video,          // Video processing
    Appearance,
    Downloads,
    Search,
    Advanced
};

/**
 * @brief Setting item types
 */
enum class SettingType {
    Toggle,
    Select,
    Text,
    Number,
    Button,
    Section
};

/**
 * @brief Individual setting item
 */
struct SettingItem {
    std::string id;
    std::string label;
    std::string description;
    SettingType type;
    std::string value;
    std::vector<std::string> options; // For Select type
    std::function<void(const std::string&)> onChange;
};

/**
 * @brief Saved password entry for display
 */
struct SavedPassword {
    std::string site;
    std::string username;
    std::string displayPassword; // Masked or revealed
    int64_t lastUsed;
    bool isRevealed;
};

/**
 * @brief Password manager panel within settings
 */
class PasswordManagerPanel {
public:
    PasswordManagerPanel();
    ~PasswordManagerPanel();
    
    // Vault operations
    bool unlockVault(const std::string& masterPassword);
    void lockVault();
    bool isUnlocked() const;
    
    // Password management
    std::vector<SavedPassword> getPasswords() const;
    std::vector<SavedPassword> searchPasswords(const std::string& query) const;
    
    bool revealPassword(const std::string& site, const std::string& username);
    bool hidePassword(const std::string& site, const std::string& username);
    bool deletePassword(const std::string& site, const std::string& username);
    bool editPassword(const std::string& site, const std::string& username,
                      const std::string& newPassword);
    
    // Password generation
    std::string generatePassword(int length = 16, bool symbols = true);
    int checkStrength(const std::string& password);
    
    // Export/Import
    bool exportPasswords(const std::string& filepath);
    bool importPasswords(const std::string& filepath);
    
    // UI state
    void render();
    
private:
    bool m_isUnlocked;
    std::string m_searchQuery;
    std::vector<SavedPassword> m_cachedPasswords;
    
    void refreshPasswordList();
};

/**
 * @brief Modern settings UI with tabbed sections
 */
class SettingsUI {
public:
    SettingsUI();
    ~SettingsUI();

    // Visibility
    void render();
    void show();
    void hide();
    bool isVisible() const;
    
    // Navigation
    void switchSection(SettingsSection section);
    SettingsSection currentSection() const;
    
    // Settings access
    std::string getSetting(const std::string& key) const;
    void setSetting(const std::string& key, const std::string& value);
    void resetToDefaults();
    
    // Sub-panels
    PasswordManagerPanel& passwordManager();
    
    // Callbacks
    void setOnSettingChanged(std::function<void(const std::string&, const std::string&)> cb);
    
    // Persistence
    bool saveSettings();
    bool loadSettings();

private:
    bool m_isVisible;
    SettingsSection m_currentSection;
    std::vector<SettingItem> m_settings;
    std::unique_ptr<PasswordManagerPanel> m_passwordManager;
    std::function<void(const std::string&, const std::string&)> m_onSettingChanged;
    
    void initializeSettings();
    void renderSidebar();
    void renderContent();
    void renderGeneralSection();
    void renderPrivacySection();
    void renderSecuritySection();
    void renderPasswordsSection();
    void renderAudioSection();
    void renderVideoSection();      // Video processing
    void renderAppearanceSection();
    void renderDownloadsSection();
    void renderSearchSection();
    void renderAdvancedSection();
};

} // namespace ui
} // namespace zepra

#endif // ZEPRA_SETTINGS_UI_H
