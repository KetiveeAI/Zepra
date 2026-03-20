// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file settings_ui.cpp
 * @brief Modern browser settings implementation
 * 
 * Integrates with ZebraScript's password vault for secure
 * credential management. Uses placeholder rendering that
 * can be connected to ImGui, SDL, or Qt.
 */

#include "ui/settings_ui.h"

// Include ZebraScript secure storage integration
// Adjust path based on your build setup
#include "zeprascript/browser/password_vault.hpp"
#include "zeprascript/browser/secure_storage.hpp"

// Browser audio integration
#include "engine/browser_audio.h"
#include "engine/audio_equalizer.h"
#include "engine/video_processor.h"

#include <iostream>
#include <algorithm>
#include <sstream>
#include <fstream>

namespace zepra {
namespace ui {

// =============================================================================
// PasswordManagerPanel Implementation
// =============================================================================

PasswordManagerPanel::PasswordManagerPanel()
    : m_isUnlocked(false) {
}

PasswordManagerPanel::~PasswordManagerPanel() {
    lockVault();
}

bool PasswordManagerPanel::unlockVault(const std::string& masterPassword) {
    auto& vault = Zepra::Browser::getPasswordVault();
    m_isUnlocked = vault.unlock(masterPassword);
    
    if (m_isUnlocked) {
        refreshPasswordList();
    }
    
    return m_isUnlocked;
}

void PasswordManagerPanel::lockVault() {
    if (m_isUnlocked) {
        auto& vault = Zepra::Browser::getPasswordVault();
        vault.lock();
        m_isUnlocked = false;
        m_cachedPasswords.clear();
    }
}

bool PasswordManagerPanel::isUnlocked() const {
    return m_isUnlocked;
}

void PasswordManagerPanel::refreshPasswordList() {
    m_cachedPasswords.clear();
    
    if (!m_isUnlocked) return;
    
    auto& vault = Zepra::Browser::getPasswordVault();
    auto sites = vault.getSites();
    
    for (const auto& site : sites) {
        auto creds = vault.getCredentials(site);
        for (const auto& cred : creds) {
            SavedPassword sp;
            sp.site = site;
            sp.username = cred.username;
            sp.displayPassword = "••••••••";  // Masked
            sp.lastUsed = cred.lastUsed;
            sp.isRevealed = false;
            m_cachedPasswords.push_back(sp);
        }
    }
    
    // Sort by last used (most recent first)
    std::sort(m_cachedPasswords.begin(), m_cachedPasswords.end(),
        [](const SavedPassword& a, const SavedPassword& b) {
            return a.lastUsed > b.lastUsed;
        });
}

std::vector<SavedPassword> PasswordManagerPanel::getPasswords() const {
    if (m_searchQuery.empty()) {
        return m_cachedPasswords;
    }
    return searchPasswords(m_searchQuery);
}

std::vector<SavedPassword> PasswordManagerPanel::searchPasswords(
    const std::string& query) const {
    
    std::vector<SavedPassword> results;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), 
                   lowerQuery.begin(), ::tolower);
    
    for (const auto& pwd : m_cachedPasswords) {
        std::string lowerSite = pwd.site;
        std::transform(lowerSite.begin(), lowerSite.end(), 
                       lowerSite.begin(), ::tolower);
        
        std::string lowerUser = pwd.username;
        std::transform(lowerUser.begin(), lowerUser.end(), 
                       lowerUser.begin(), ::tolower);
        
        if (lowerSite.find(lowerQuery) != std::string::npos ||
            lowerUser.find(lowerQuery) != std::string::npos) {
            results.push_back(pwd);
        }
    }
    
    return results;
}

bool PasswordManagerPanel::revealPassword(const std::string& site, 
                                           const std::string& username) {
    if (!m_isUnlocked) return false;
    
    auto& vault = Zepra::Browser::getPasswordVault();
    auto creds = vault.getCredentials(site);
    
    for (const auto& cred : creds) {
        if (cred.username == username) {
            // Find in cache and reveal
            for (auto& sp : m_cachedPasswords) {
                if (sp.site == site && sp.username == username) {
                    sp.displayPassword = cred.password;
                    sp.isRevealed = true;
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool PasswordManagerPanel::hidePassword(const std::string& site,
                                         const std::string& username) {
    for (auto& sp : m_cachedPasswords) {
        if (sp.site == site && sp.username == username) {
            sp.displayPassword = "••••••••";
            sp.isRevealed = false;
            return true;
        }
    }
    return false;
}

bool PasswordManagerPanel::deletePassword(const std::string& site,
                                           const std::string& username) {
    if (!m_isUnlocked) return false;
    
    auto& vault = Zepra::Browser::getPasswordVault();
    bool success = vault.deleteCredential(site, username);
    
    if (success) {
        refreshPasswordList();
    }
    
    return success;
}

bool PasswordManagerPanel::editPassword(const std::string& site,
                                         const std::string& username,
                                         const std::string& newPassword) {
    if (!m_isUnlocked) return false;
    
    auto& vault = Zepra::Browser::getPasswordVault();
    
    Zepra::Browser::Credential cred;
    cred.site = site;
    cred.username = username;
    cred.password = newPassword;
    
    bool success = vault.saveCredential(cred);
    
    if (success) {
        refreshPasswordList();
    }
    
    return success;
}

std::string PasswordManagerPanel::generatePassword(int length, bool symbols) {
    return Zepra::Browser::PasswordVault::generatePassword(length, symbols);
}

int PasswordManagerPanel::checkStrength(const std::string& password) {
    return Zepra::Browser::PasswordVault::passwordStrength(password);
}

bool PasswordManagerPanel::exportPasswords(const std::string& filepath) {
    if (!m_isUnlocked) return false;
    
    std::ofstream file(filepath);
    if (!file) return false;
    
    // CSV export
    file << "Site,Username,Password\n";
    
    auto& vault = Zepra::Browser::getPasswordVault();
    auto sites = vault.getSites();
    
    for (const auto& site : sites) {
        auto creds = vault.getCredentials(site);
        for (const auto& cred : creds) {
            file << "\"" << site << "\","
                 << "\"" << cred.username << "\","
                 << "\"" << cred.password << "\"\n";
        }
    }
    
    return true;
}

bool PasswordManagerPanel::importPasswords(const std::string& filepath) {
    if (!m_isUnlocked) return false;
    
    std::ifstream file(filepath);
    if (!file) return false;
    
    std::string line;
    std::getline(file, line); // Skip header
    
    auto& vault = Zepra::Browser::getPasswordVault();
    
    while (std::getline(file, line)) {
        // Simple CSV parsing
        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string field;
        
        while (std::getline(ss, field, ',')) {
            // Remove quotes
            if (!field.empty() && field.front() == '"' && field.back() == '"') {
                field = field.substr(1, field.length() - 2);
            }
            fields.push_back(field);
        }
        
        if (fields.size() >= 3) {
            Zepra::Browser::Credential cred;
            cred.site = fields[0];
            cred.username = fields[1];
            cred.password = fields[2];
            vault.saveCredential(cred);
        }
    }
    
    refreshPasswordList();
    return true;
}

void PasswordManagerPanel::render() {
    // Placeholder rendering - connect to your UI framework
    std::cout << "\n=== Password Manager ===" << std::endl;
    
    if (!m_isUnlocked) {
        std::cout << "[LOCKED] Enter master password to unlock" << std::endl;
        return;
    }
    
    std::cout << "Saved Passwords: " << m_cachedPasswords.size() << std::endl;
    std::cout << "------------------------" << std::endl;
    
    for (const auto& pwd : getPasswords()) {
        std::cout << "• " << pwd.site << " | " << pwd.username 
                  << " | " << pwd.displayPassword << std::endl;
    }
}

// =============================================================================
// SettingsUI Implementation
// =============================================================================

SettingsUI::SettingsUI()
    : m_isVisible(false)
    , m_currentSection(SettingsSection::General)
    , m_passwordManager(std::make_unique<PasswordManagerPanel>()) {
    initializeSettings();
}

SettingsUI::~SettingsUI() {
    saveSettings();
}

void SettingsUI::initializeSettings() {
    // General settings
    m_settings.push_back({
        "homepage", "Homepage", "Page to show when opening new tabs",
        SettingType::Text, "about:newtab", {}, nullptr
    });
    
    m_settings.push_back({
        "startup.restore_session", "Restore previous session",
        "Restore tabs from last session on startup",
        SettingType::Toggle, "false", {}, nullptr
    });
    
    // Privacy settings
    m_settings.push_back({
        "privacy.do_not_track", "Send Do Not Track",
        "Request sites not to track you",
        SettingType::Toggle, "true", {}, nullptr
    });
    
    m_settings.push_back({
        "privacy.cookies", "Cookie Policy",
        "How to handle cookies",
        SettingType::Select, "allow_all",
        {"allow_all", "block_third_party", "block_all"}, nullptr
    });
    
    // Security settings
    m_settings.push_back({
        "security.https_only", "HTTPS-Only Mode",
        "Always use secure connections when available",
        SettingType::Toggle, "true", {}, nullptr
    });
    
    m_settings.push_back({
        "security.safe_browsing", "Safe Browsing",
        "Block dangerous sites and downloads",
        SettingType::Toggle, "true", {}, nullptr
    });
    
    // Appearance settings
    m_settings.push_back({
        "appearance.theme", "Theme",
        "Browser color theme",
        SettingType::Select, "system",
        {"light", "dark", "system"}, nullptr
    });
    
    m_settings.push_back({
        "appearance.font_size", "Default Font Size",
        "Default font size for web content",
        SettingType::Number, "16", {}, nullptr
    });
    
    // Downloads settings
    m_settings.push_back({
        "downloads.location", "Download Location",
        "Where to save downloaded files",
        SettingType::Text, "~/Downloads", {}, nullptr
    });
    
    m_settings.push_back({
        "downloads.ask_location", "Always ask where to save",
        "Prompt for download location each time",
        SettingType::Toggle, "false", {}, nullptr
    });
    
    // Search settings
    m_settings.push_back({
        "search.engine", "Default Search Engine",
        "Search engine for address bar",
        SettingType::Select, "ketivee",
        {"ketivee", "google", "duckduckgo", "bing"}, nullptr
    });
    
    // Audio settings
    m_settings.push_back({
        "audio.master_volume", "Master Volume",
        "Overall browser audio volume",
        SettingType::Number, "100", {}, nullptr
    });
    
    m_settings.push_back({
        "audio.quality", "Audio Quality",
        "Audio processing quality",
        SettingType::Select, "balanced",
        {"eco", "balanced", "high", "ultra"}, nullptr
    });
    
    m_settings.push_back({
        "audio.spatial", "Spatial Audio",
        "Enable 3D/spatial audio for immersive experience",
        SettingType::Toggle, "true", {}, nullptr
    });
    
    m_settings.push_back({
        "audio.hrtf", "HRTF (Head Tracking)",
        "Enable head-related transfer function for realistic audio",
        SettingType::Toggle, "true", {}, nullptr
    });
    
    m_settings.push_back({
        "audio.power_mode", "Power Mode",
        "Balance audio quality vs battery life",
        SettingType::Select, "balanced",
        {"battery", "balanced", "performance"}, nullptr
    });
}

void SettingsUI::render() {
    if (!m_isVisible) return;
    
    // Placeholder rendering - replace with actual UI framework calls
    std::cout << "\n╔═══════════════════════════════════════════╗" << std::endl;
    std::cout << "║           ZEPRA BROWSER SETTINGS           ║" << std::endl;
    std::cout << "╠═══════════════════════════════════════════╣" << std::endl;
    
    renderSidebar();
    renderContent();
    
    std::cout << "╚═══════════════════════════════════════════╝" << std::endl;
}

void SettingsUI::renderSidebar() {
    std::cout << "║ Sections:                                  ║" << std::endl;
    
    const char* sections[] = {
        "General", "Privacy", "Security", "Passwords",
        "Audio", "Video", "Appearance", "Downloads", "Search", "Advanced"
    };
    
    for (int i = 0; i < 10; ++i) {
        bool isCurrent = (static_cast<int>(m_currentSection) == i);
        std::cout << "║   " << (isCurrent ? "▶ " : "  ") 
                  << sections[i] << std::endl;
    }
    
    std::cout << "╠═══════════════════════════════════════════╣" << std::endl;
}

void SettingsUI::renderContent() {
    switch (m_currentSection) {
        case SettingsSection::General:
            renderGeneralSection();
            break;
        case SettingsSection::Privacy:
            renderPrivacySection();
            break;
        case SettingsSection::Security:
            renderSecuritySection();
            break;
        case SettingsSection::Passwords:
            renderPasswordsSection();
            break;
        case SettingsSection::Audio:
            renderAudioSection();
            break;
        case SettingsSection::Video:
            renderVideoSection();
            break;
        case SettingsSection::Appearance:
            renderAppearanceSection();
            break;
        case SettingsSection::Downloads:
            renderDownloadsSection();
            break;
        case SettingsSection::Search:
            renderSearchSection();
            break;
        case SettingsSection::Advanced:
            renderAdvancedSection();
            break;
    }
}

void SettingsUI::renderGeneralSection() {
    std::cout << "║ GENERAL SETTINGS                           ║" << std::endl;
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ Homepage: " << getSetting("homepage") << std::endl;
    std::cout << "║ Restore Session: " << getSetting("startup.restore_session") << std::endl;
}

void SettingsUI::renderPrivacySection() {
    std::cout << "║ PRIVACY SETTINGS                           ║" << std::endl;
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ Do Not Track: " << getSetting("privacy.do_not_track") << std::endl;
    std::cout << "║ Cookies: " << getSetting("privacy.cookies") << std::endl;
}

void SettingsUI::renderSecuritySection() {
    std::cout << "║ SECURITY SETTINGS                          ║" << std::endl;
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ HTTPS-Only: " << getSetting("security.https_only") << std::endl;
    std::cout << "║ Safe Browsing: " << getSetting("security.safe_browsing") << std::endl;
}

void SettingsUI::renderPasswordsSection() {
    std::cout << "║ PASSWORDS                                  ║" << std::endl;
    m_passwordManager->render();
}

void SettingsUI::renderAudioSection() {
    std::cout << "║ AUDIO SETTINGS (NXAudio)                   ║" << std::endl;
    std::cout << "║                                            ║" << std::endl;
    
    // Get browser audio instance
    auto& audio = zepra::audio::getBrowserAudio();
    
    // Volume
    std::cout << "║ Master Volume: " << getSetting("audio.master_volume") << "%" << std::endl;
    
    // Quality
    std::cout << "║ Audio Quality: " << getSetting("audio.quality") << std::endl;
    
    // Spatial Audio
    std::cout << "║ Spatial Audio: " << getSetting("audio.spatial") << std::endl;
    std::cout << "║ HRTF Enabled: " << getSetting("audio.hrtf") << std::endl;
    
    // Power mode
    std::cout << "║ Power Mode: " << getSetting("audio.power_mode") << std::endl;
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─── Platform Info ───                      ║" << std::endl;
    
    // Platform capabilities
    if (audio.isInitialized()) {
        auto stats = audio.getStats();
        std::cout << "║ SIMD Active: " << (stats.simdActive ? "Yes" : "No") << std::endl;
        std::cout << "║ Platform: " << (audio.isArmPlatform() ? "ARM" : "x86") << std::endl;
        
        if (!audio.isArmPlatform()) {
            std::cout << "║ AVX2 Support: " << (audio.hasAVX2Support() ? "Yes" : "No") << std::endl;
        } else {
            std::cout << "║ NEON Support: " << (audio.hasNeonSupport() ? "Yes" : "No") << std::endl;
        }
        
        std::cout << "║ Latency: " << stats.latencyMs << "ms" << std::endl;
    } else {
        std::cout << "║ [Audio not initialized]                    ║" << std::endl;
    }
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─── Audio Quality Presets ───              ║" << std::endl;
    std::cout << "║   Eco       - Low power, basic processing  ║" << std::endl;
    std::cout << "║   Balanced  - Good quality, moderate power ║" << std::endl;
    std::cout << "║   High      - Full quality, spatial audio  ║" << std::endl;
    std::cout << "║   Ultra     - Maximum quality, HRTF        ║" << std::endl;
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─────────────────────────────────────────  ║" << std::endl;
    std::cout << "║          ADVANCED AUDIO CONTROLS           ║" << std::endl;
    std::cout << "║ ─────────────────────────────────────────  ║" << std::endl;
    
    // Get equalizer instance
    auto& eq = zepra::audio::getAudioEqualizer();
    auto eqSettings = eq.getEqualizer();
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─── One-Click Presets ───                  ║" << std::endl;
    std::cout << "║ Current: " << zepra::audio::getPresetName(eq.getCurrentPreset()) << std::endl;
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ [Default] [Music] [Movie] [Gaming]         ║" << std::endl;
    std::cout << "║ [Pop] [Rock] [Classical] [Jazz] [EDM]      ║" << std::endl;
    std::cout << "║ [Late Night] [Headphones] [Speakers]       ║" << std::endl;
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─── 10-Band Equalizer ───                  ║" << std::endl;
    std::cout << "║                                            ║" << std::endl;
    
    // Display EQ bands as ASCII sliders
    const char* bandNames[] = {"32Hz", "64Hz", "125Hz", "250Hz", "500Hz", 
                                "1kHz", "2kHz", "4kHz", "8kHz", "16kHz"};
    
    for (int i = 0; i < 10; ++i) {
        float gain = eqSettings.bands[i];
        int pos = static_cast<int>((gain + 12.0f) / 24.0f * 20.0f);  // -12 to +12 -> 0 to 20
        pos = std::max(0, std::min(20, pos));
        
        std::cout << "║ " << bandNames[i];
        if (i < 4) std::cout << " ";  // Padding for alignment
        std::cout << " [";
        for (int j = 0; j < 20; ++j) {
            if (j == 10) std::cout << "|";  // Center marker
            else if (j == pos) std::cout << "●";
            else std::cout << "─";
        }
        std::cout << "] " << (gain >= 0 ? "+" : "") << gain << "dB" << std::endl;
    }
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─── Quick Controls ───                     ║" << std::endl;
    std::cout << "║ Bass:   " << (eq.getBass() >= 0 ? "+" : "") << eq.getBass() << " dB" << std::endl;
    std::cout << "║ Mid:    " << (eq.getMid() >= 0 ? "+" : "") << eq.getMid() << " dB" << std::endl;
    std::cout << "║ Treble: " << (eq.getTreble() >= 0 ? "+" : "") << eq.getTreble() << " dB" << std::endl;
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─── Bass Boost ───                         ║" << std::endl;
    auto bbSettings = eq.getBassBoost();
    std::cout << "║ Enabled: " << (bbSettings.enabled ? "On" : "Off") << std::endl;
    std::cout << "║ Amount: " << static_cast<int>(bbSettings.amount * 100) << "%" << std::endl;
    std::cout << "║ Frequency: " << bbSettings.frequency << " Hz" << std::endl;
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─── Surround / Spatial ───                 ║" << std::endl;
    auto surroundSettings = eq.getSurround();
    std::cout << "║ Enabled: " << (surroundSettings.enabled ? "On" : "Off") << std::endl;
    std::cout << "║ Stereo Width: " << static_cast<int>(surroundSettings.width * 100) << "%" << std::endl;
    std::cout << "║ Virtual Surround: " << (surroundSettings.virtualizer ? "On" : "Off") << std::endl;
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─── Pitch & Speed ───                      ║" << std::endl;
    auto pitchSettings = eq.getPitch();
    std::cout << "║ Pitch: " << (pitchSettings.pitch >= 0 ? "+" : "") 
              << pitchSettings.pitch << " semitones" << std::endl;
    std::cout << "║ Speed: " << pitchSettings.speed << "x" << std::endl;
    std::cout << "║ Preserve Pitch: " << (pitchSettings.preservePitch ? "On" : "Off") << std::endl;
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─── Dynamics (Compressor) ───              ║" << std::endl;
    auto dynSettings = eq.getDynamics();
    std::cout << "║ Compressor: " << (dynSettings.enabled ? "On" : "Off") << std::endl;
    std::cout << "║ Threshold: " << dynSettings.threshold << " dB" << std::endl;
    std::cout << "║ Ratio: " << dynSettings.ratio << ":1" << std::endl;
    std::cout << "║ Limiter: " << (dynSettings.limiterEnabled ? "On" : "Off") << std::endl;
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─── Enhancements ───                       ║" << std::endl;
    auto enhSettings = eq.getEnhancement();
    std::cout << "║ Voice Enhance: " << (enhSettings.voiceEnhance ? "On" : "Off") << std::endl;
    std::cout << "║ Dialogue Boost: " << (enhSettings.dialogueBoost ? "On" : "Off") << std::endl;
    std::cout << "║ Loudness Norm: " << (enhSettings.loudnessNorm ? "On" : "Off") << std::endl;
    std::cout << "║ Crossfeed (Headphones): " << (enhSettings.crossfeed ? "On" : "Off") << std::endl;
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ [Save Custom Preset] [Reset to Default]    ║" << std::endl;
}

void SettingsUI::renderVideoSection() {
    std::cout << "║ VIDEO SETTINGS                             ║" << std::endl;
    std::cout << "║                                            ║" << std::endl;
    
    auto& video = zepra::video::getVideoProcessor();
    
    std::cout << "║ ─── Hardware Decode ───                    ║" << std::endl;
    std::cout << "║ Hardware Decode: " << (video.isHardwareDecodeEnabled() ? "Enabled" : "Disabled") << std::endl;
    
    auto hwDecoders = video.getHardwareDecoders();
    if (!hwDecoders.empty()) {
        std::cout << "║ Available Decoders:" << std::endl;
        for (const auto& hw : hwDecoders) {
            std::cout << "║   • " << hw.deviceName;
            std::cout << " (H.264:" << (hw.supportsH264 ? "✓" : "✗");
            std::cout << " H.265:" << (hw.supportsH265 ? "✓" : "✗");
            std::cout << " VP9:" << (hw.supportsVP9 ? "✓" : "✗");
            std::cout << " AV1:" << (hw.supportsAV1 ? "✓" : "✗") << ")" << std::endl;
        }
    } else {
        std::cout << "║   [No hardware decoders found]             ║" << std::endl;
    }
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─── One-Click Presets ───                  ║" << std::endl;
    std::cout << "║ Current: " << zepra::video::VideoProcessor::presetName(video.getCurrentPreset()) << std::endl;
    std::cout << "║ [Default] [Cinema] [Vivid] [Natural]       ║" << std::endl;
    std::cout << "║ [Gaming] [Reading] [Night] [HDR Sim]       ║" << std::endl;
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─── Video Adjustments ───                  ║" << std::endl;
    auto adj = video.getAdjustments();
    
    // Brightness slider
    int brightnessPos = static_cast<int>((adj.brightness + 1.0f) / 2.0f * 20.0f);
    std::cout << "║ Brightness [";
    for (int i = 0; i < 20; ++i) std::cout << (i == brightnessPos ? "●" : "─");
    std::cout << "] " << (adj.brightness >= 0 ? "+" : "") << adj.brightness << std::endl;
    
    // Contrast slider
    int contrastPos = static_cast<int>(adj.contrast / 2.0f * 20.0f);
    std::cout << "║ Contrast   [";
    for (int i = 0; i < 20; ++i) std::cout << (i == contrastPos ? "●" : "─");
    std::cout << "] " << adj.contrast << std::endl;
    
    // Saturation slider
    int satPos = static_cast<int>(adj.saturation / 2.0f * 20.0f);
    std::cout << "║ Saturation [";
    for (int i = 0; i < 20; ++i) std::cout << (i == satPos ? "●" : "─");
    std::cout << "] " << adj.saturation << std::endl;
    
    // Sharpness slider
    int sharpPos = static_cast<int>(adj.sharpness * 20.0f);
    std::cout << "║ Sharpness  [";
    for (int i = 0; i < 20; ++i) std::cout << (i == sharpPos ? "●" : "─");
    std::cout << "] " << adj.sharpness << std::endl;
    
    // Gamma slider
    int gammaPos = static_cast<int>((adj.gamma - 0.1f) / 3.9f * 20.0f);
    std::cout << "║ Gamma      [";
    for (int i = 0; i < 20; ++i) std::cout << (i == gammaPos ? "●" : "─");
    std::cout << "] " << adj.gamma << std::endl;
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─── AI Enhancement ───                     ║" << std::endl;
    auto ai = video.getAIEnhancement();
    std::cout << "║ AI Available: " << (video.isAIAvailable() ? "Yes" : "No") << std::endl;
    std::cout << "║ AI Upscaling: " << (ai.upscalingEnabled ? "On" : "Off") << std::endl;
    std::cout << "║ AI Denoise: " << (ai.denoiseEnabled ? "On" : "Off");
    if (ai.denoiseEnabled) std::cout << " (" << static_cast<int>(ai.denoiseStrength * 100) << "%)";
    std::cout << std::endl;
    std::cout << "║ Frame Interpolation: " << (ai.frameInterpolation ? "On" : "Off");
    if (ai.frameInterpolation) std::cout << " (" << ai.targetFPS << " FPS)";
    std::cout << std::endl;
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─── Audio/Video Sync ───                   ║" << std::endl;
    std::cout << "║ A/V Offset: " << video.getAudioVideoOffset() << " ms" << std::endl;
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ ─── Power Mode ───                         ║" << std::endl;
    std::cout << "║ Low Power Mode: " << (video.isLowPowerMode() ? "On" : "Off") << std::endl;
    
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ [Reset to Default]                         ║" << std::endl;
}

void SettingsUI::renderAppearanceSection() {
    std::cout << "║ APPEARANCE SETTINGS                        ║" << std::endl;
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ Theme: " << getSetting("appearance.theme") << std::endl;
    std::cout << "║ Font Size: " << getSetting("appearance.font_size") << std::endl;
}

void SettingsUI::renderDownloadsSection() {
    std::cout << "║ DOWNLOADS SETTINGS                         ║" << std::endl;
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ Location: " << getSetting("downloads.location") << std::endl;
    std::cout << "║ Ask Location: " << getSetting("downloads.ask_location") << std::endl;
}

void SettingsUI::renderSearchSection() {
    std::cout << "║ SEARCH SETTINGS                            ║" << std::endl;
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ Default Engine: " << getSetting("search.engine") << std::endl;
}

void SettingsUI::renderAdvancedSection() {
    std::cout << "║ ADVANCED SETTINGS                          ║" << std::endl;
    std::cout << "║                                            ║" << std::endl;
    std::cout << "║ [Reset to Defaults]                        ║" << std::endl;
    std::cout << "║ [Clear Browsing Data]                      ║" << std::endl;
    std::cout << "║ [Developer Options]                        ║" << std::endl;
}

void SettingsUI::show() {
    m_isVisible = true;
}

void SettingsUI::hide() {
    m_isVisible = false;
}

bool SettingsUI::isVisible() const {
    return m_isVisible;
}

void SettingsUI::switchSection(SettingsSection section) {
    m_currentSection = section;
}

SettingsSection SettingsUI::currentSection() const {
    return m_currentSection;
}

std::string SettingsUI::getSetting(const std::string& key) const {
    for (const auto& setting : m_settings) {
        if (setting.id == key) {
            return setting.value;
        }
    }
    return "";
}

void SettingsUI::setSetting(const std::string& key, const std::string& value) {
    for (auto& setting : m_settings) {
        if (setting.id == key) {
            setting.value = value;
            if (setting.onChange) {
                setting.onChange(value);
            }
            if (m_onSettingChanged) {
                m_onSettingChanged(key, value);
            }
            return;
        }
    }
}

void SettingsUI::resetToDefaults() {
    initializeSettings();
}

PasswordManagerPanel& SettingsUI::passwordManager() {
    return *m_passwordManager;
}

void SettingsUI::setOnSettingChanged(
    std::function<void(const std::string&, const std::string&)> cb) {
    m_onSettingChanged = cb;
}

bool SettingsUI::saveSettings() {
    // Save to JSON file
    std::ofstream file("zepra_settings.json");
    if (!file) return false;
    
    file << "{\n";
    for (size_t i = 0; i < m_settings.size(); ++i) {
        file << "  \"" << m_settings[i].id << "\": \"" 
             << m_settings[i].value << "\"";
        if (i < m_settings.size() - 1) file << ",";
        file << "\n";
    }
    file << "}\n";
    
    return true;
}

bool SettingsUI::loadSettings() {
    std::ifstream file("zepra_settings.json");
    if (!file) return false;
    
    // Simple JSON parsing (production would use proper JSON parser)
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    // Parse key-value pairs
    for (auto& setting : m_settings) {
        std::string needle = "\"" + setting.id + "\": \"";
        size_t pos = content.find(needle);
        if (pos != std::string::npos) {
            pos += needle.length();
            size_t end = content.find("\"", pos);
            if (end != std::string::npos) {
                setting.value = content.substr(pos, end - pos);
            }
        }
    }
    
    return true;
}

} // namespace ui
} // namespace zepra
