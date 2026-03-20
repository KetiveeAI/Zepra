// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file privacy_settings_ui.cpp
 * @brief Privacy settings UI implementation
 */

#include "storage/privacy_settings_ui.hpp"
#include "storage/site_settings.hpp"
#include "storage/permission_manager.hpp"
#include "storage/quota_manager.hpp"
#include "storage/local_storage.hpp"
#include "storage/cache_storage.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace Zepra::UI {

PrivacySettingsPanel::PrivacySettingsPanel() = default;
PrivacySettingsPanel::~PrivacySettingsPanel() = default;

void PrivacySettingsPanel::clearBrowsingData(bool cookies, bool cache, 
                                              bool localStorage, bool history,
                                              bool passwords) {
    auto& ssm = Storage::getSiteSettingsManager();
    
    for (const auto& site : ssm.getAllSites()) {
        if (cookies) {
            ssm.deleteSiteCookies(site.origin);
        }
        if (cache) {
            ssm.deleteSiteCache(site.origin);
        }
        if (localStorage) {
            ssm.deleteSiteStorage(site.origin);
        }
    }
    
    // History and passwords would need their own managers
}

void PrivacySettingsPanel::setBlockThirdPartyCookies(bool block) {
    blockThirdPartyCookies_ = block;
}

void PrivacySettingsPanel::setDoNotTrack(bool enabled) {
    doNotTrack_ = enabled;
}

void PrivacySettingsPanel::setTrackingProtection(bool enabled) {
    trackingProtection_ = enabled;
}

std::vector<PermissionToggle> PrivacySettingsPanel::getPermissionToggles(
    const std::string& origin) {
    
    auto& pm = Storage::getPermissionManager();
    std::string normalized = Storage::SiteSettingsManager::normalizeOrigin(origin);
    
    std::vector<PermissionToggle> toggles = {
        {Storage::PermissionType::Camera, "Camera", 
         "Access your camera", "camera",
         pm.query(Storage::PermissionType::Camera, normalized), true},
         
        {Storage::PermissionType::Microphone, "Microphone",
         "Access your microphone", "mic",
         pm.query(Storage::PermissionType::Microphone, normalized), true},
         
        {Storage::PermissionType::Geolocation, "Location",
         "Access your location", "location",
         pm.query(Storage::PermissionType::Geolocation, normalized), true},
         
        {Storage::PermissionType::Notifications, "Notifications",
         "Show notifications", "notifications",
         pm.query(Storage::PermissionType::Notifications, normalized), false},
         
        {Storage::PermissionType::Clipboard, "Clipboard",
         "Access clipboard", "clipboard",
         pm.query(Storage::PermissionType::Clipboard, normalized), false},
         
        {Storage::PermissionType::ScreenCapture, "Screen Capture",
         "Capture screen contents", "screen",
         pm.query(Storage::PermissionType::ScreenCapture, normalized), true},
         
        {Storage::PermissionType::Midi, "MIDI Devices",
         "Access MIDI devices", "midi",
         pm.query(Storage::PermissionType::Midi, normalized), false},
         
        {Storage::PermissionType::USB, "USB Devices",
         "Access USB devices", "usb",
         pm.query(Storage::PermissionType::USB, normalized), true},
         
        {Storage::PermissionType::Bluetooth, "Bluetooth",
         "Access Bluetooth devices", "bluetooth",
         pm.query(Storage::PermissionType::Bluetooth, normalized), true}
    };
    
    return toggles;
}

void PrivacySettingsPanel::setPermissionState(const std::string& origin,
                                               Storage::PermissionType type,
                                               Storage::PermissionState state) {
    Storage::getSiteSettingsManager().setPermission(origin, type, state);
}

std::vector<StorageItem> PrivacySettingsPanel::getStorageItems(
    const std::string& origin) {
    
    std::string normalized = Storage::SiteSettingsManager::normalizeOrigin(origin);
    auto usage = Storage::getQuotaManager().getUsage(normalized);
    
    std::vector<StorageItem> items;
    
    if (usage.localStorage > 0) {
        items.push_back({
            "LocalStorage",
            usage.localStorage,
            formatBytes(usage.localStorage),
            true
        });
    }
    
    if (usage.indexedDB > 0) {
        items.push_back({
            "IndexedDB",
            usage.indexedDB,
            formatBytes(usage.indexedDB),
            true
        });
    }
    
    if (usage.cacheStorage > 0) {
        items.push_back({
            "Cache Storage",
            usage.cacheStorage,
            formatBytes(usage.cacheStorage),
            true
        });
    }
    
    return items;
}

void PrivacySettingsPanel::deleteStorageItem(const std::string& origin,
                                              const std::string& type) {
    std::string normalized = Storage::SiteSettingsManager::normalizeOrigin(origin);
    
    if (type == "LocalStorage") {
        Storage::getLocalStorage(normalized).clear();
    } else if (type == "Cache Storage") {
        auto& cache = Storage::getCacheStorage(normalized);
        for (const auto& name : cache.keys()) {
            cache.deleteCache(name);
        }
    }
    // IndexedDB would need its own implementation
}

std::vector<Storage::SiteSettingsEntry> PrivacySettingsPanel::getAllSites() {
    return Storage::getSiteSettingsManager().getAllSites();
}

std::vector<Storage::SiteSettingsEntry> PrivacySettingsPanel::searchSites(
    const std::string& query) {
    
    auto all = getAllSites();
    std::vector<Storage::SiteSettingsEntry> results;
    
    std::string queryLower = query;
    std::transform(queryLower.begin(), queryLower.end(), 
                   queryLower.begin(), ::tolower);
    
    for (const auto& site : all) {
        std::string nameLower = site.displayName;
        std::transform(nameLower.begin(), nameLower.end(), 
                       nameLower.begin(), ::tolower);
        
        if (nameLower.find(queryLower) != std::string::npos) {
            results.push_back(site);
        }
    }
    
    return results;
}

void PrivacySettingsPanel::deleteSiteData(const std::string& origin) {
    Storage::getSiteSettingsManager().deleteSiteData(origin);
}

std::vector<PrivacySettingsPanel::NotificationSite> 
PrivacySettingsPanel::getNotificationSites() {
    auto& pm = Storage::getPermissionManager();
    auto all = pm.getAllPermissions();
    
    std::vector<NotificationSite> sites;
    
    for (const auto& perm : all) {
        if (perm.type == Storage::PermissionType::Notifications) {
            NotificationSite site;
            site.origin = perm.origin;
            
            // Extract display name
            size_t schemeEnd = perm.origin.find("://");
            site.displayName = (schemeEnd != std::string::npos) 
                ? perm.origin.substr(schemeEnd + 3) 
                : perm.origin;
            
            site.enabled = (perm.state == Storage::PermissionState::Granted);
            sites.push_back(site);
        }
    }
    
    return sites;
}

void PrivacySettingsPanel::toggleNotifications(const std::string& origin, 
                                                bool enabled) {
    if (enabled) {
        Storage::getSiteSettingsManager().enableNotifications(origin);
    } else {
        Storage::getSiteSettingsManager().disableNotifications(origin);
    }
}

void PrivacySettingsPanel::blockAllNotifications() {
    auto sites = getNotificationSites();
    for (const auto& site : sites) {
        toggleNotifications(site.origin, false);
    }
}

void PrivacySettingsPanel::setCookieSetting(CookieSetting setting) {
    cookieSetting_ = setting;
    
    switch (setting) {
        case CookieSetting::AllowAll:
            blockThirdPartyCookies_ = false;
            break;
        case CookieSetting::BlockThirdParty:
            blockThirdPartyCookies_ = true;
            break;
        case CookieSetting::BlockAll:
            blockThirdPartyCookies_ = true;
            break;
    }
}

std::vector<PrivacySettingsPanel::CookieException> 
PrivacySettingsPanel::getCookieExceptions() {
    return cookieExceptions_;
}

void PrivacySettingsPanel::addCookieException(const std::string& origin, 
                                               bool allowed) {
    std::string normalized = Storage::SiteSettingsManager::normalizeOrigin(origin);
    
    // Remove existing
    cookieExceptions_.erase(
        std::remove_if(cookieExceptions_.begin(), cookieExceptions_.end(),
            [&](const CookieException& e) { return e.origin == normalized; }),
        cookieExceptions_.end());
    
    cookieExceptions_.push_back({normalized, allowed});
}

void PrivacySettingsPanel::removeCookieException(const std::string& origin) {
    std::string normalized = Storage::SiteSettingsManager::normalizeOrigin(origin);
    
    cookieExceptions_.erase(
        std::remove_if(cookieExceptions_.begin(), cookieExceptions_.end(),
            [&](const CookieException& e) { return e.origin == normalized; }),
        cookieExceptions_.end());
}

bool PrivacySettingsPanel::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key, value;
        
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            if (key == "blockThirdPartyCookies") {
                blockThirdPartyCookies_ = (value == "1");
            } else if (key == "doNotTrack") {
                doNotTrack_ = (value == "1");
            } else if (key == "trackingProtection") {
                trackingProtection_ = (value == "1");
            } else if (key == "cookieSetting") {
                cookieSetting_ = static_cast<CookieSetting>(std::stoi(value));
            }
        }
    }
    
    return true;
}

bool PrivacySettingsPanel::save(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    
    file << "blockThirdPartyCookies=" << (blockThirdPartyCookies_ ? "1" : "0") << '\n';
    file << "doNotTrack=" << (doNotTrack_ ? "1" : "0") << '\n';
    file << "trackingProtection=" << (trackingProtection_ ? "1" : "0") << '\n';
    file << "cookieSetting=" << static_cast<int>(cookieSetting_) << '\n';
    
    return true;
}

std::string PrivacySettingsPanel::formatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024 && unit < 3) {
        size /= 1024;
        unit++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(unit > 0 ? 1 : 0) << size << " " << units[unit];
    return oss.str();
}

std::string PrivacySettingsPanel::getPermissionName(Storage::PermissionType type) {
    switch (type) {
        case Storage::PermissionType::Camera: return "Camera";
        case Storage::PermissionType::Microphone: return "Microphone";
        case Storage::PermissionType::Geolocation: return "Location";
        case Storage::PermissionType::Notifications: return "Notifications";
        case Storage::PermissionType::Clipboard: return "Clipboard";
        case Storage::PermissionType::ScreenCapture: return "Screen Capture";
        case Storage::PermissionType::Midi: return "MIDI";
        case Storage::PermissionType::Bluetooth: return "Bluetooth";
        case Storage::PermissionType::USB: return "USB";
        case Storage::PermissionType::Serial: return "Serial Port";
        case Storage::PermissionType::HID: return "HID Device";
        case Storage::PermissionType::Sensors: return "Motion Sensors";
        case Storage::PermissionType::PersistentStorage: return "Persistent Storage";
        case Storage::PermissionType::BackgroundSync: return "Background Sync";
        case Storage::PermissionType::Fullscreen: return "Fullscreen";
        case Storage::PermissionType::PaymentHandler: return "Payment";
        case Storage::PermissionType::IdleDetection: return "Idle Detection";
    }
    return "Unknown";
}

std::string PrivacySettingsPanel::getPermissionIcon(Storage::PermissionType type) {
    switch (type) {
        case Storage::PermissionType::Camera: return "videocam";
        case Storage::PermissionType::Microphone: return "mic";
        case Storage::PermissionType::Geolocation: return "location_on";
        case Storage::PermissionType::Notifications: return "notifications";
        case Storage::PermissionType::Clipboard: return "content_paste";
        case Storage::PermissionType::ScreenCapture: return "screen_share";
        case Storage::PermissionType::Midi: return "piano";
        case Storage::PermissionType::Bluetooth: return "bluetooth";
        case Storage::PermissionType::USB: return "usb";
        case Storage::PermissionType::Serial: return "settings_input_hdmi";
        case Storage::PermissionType::HID: return "keyboard";
        case Storage::PermissionType::Sensors: return "sensors";
        default: return "settings";
    }
}

PrivacySettingsPanel& getPrivacySettingsPanel() {
    static PrivacySettingsPanel instance;
    return instance;
}

} // namespace Zepra::UI
