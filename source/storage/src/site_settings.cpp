/**
 * @file site_settings.cpp
 * @brief Site settings implementation with origin isolation
 */

#include "storage/site_settings.hpp"
#include "storage/local_storage.hpp"
#include "storage/session_storage.hpp"
#include "storage/cache_storage.hpp"
#include "storage/quota_manager.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>

namespace fs = std::filesystem;

namespace Zepra::Storage {

// =============================================================================
// SiteSettingsManager Implementation
// =============================================================================

SiteSettingsManager::SiteSettingsManager() = default;
SiteSettingsManager::~SiteSettingsManager() = default;

std::string SiteSettingsManager::normalizeOrigin(const std::string& url) {
    // Extract scheme://host:port
    std::regex originRegex(R"(^(https?://[^/]+))");
    std::smatch match;
    
    if (std::regex_search(url, match, originRegex)) {
        std::string origin = match[1];
        // Lowercase
        std::transform(origin.begin(), origin.end(), origin.begin(), ::tolower);
        return origin;
    }
    
    // If no scheme, assume https
    if (url.find("://") == std::string::npos) {
        return "https://" + url;
    }
    
    return url;
}

bool SiteSettingsManager::isSameOrigin(const std::string& origin1, 
                                        const std::string& origin2) {
    return normalizeOrigin(origin1) == normalizeOrigin(origin2);
}

bool SiteSettingsManager::isCrossOriginAllowed(const std::string& from,
                                                const std::string& to) {
    // Same origin always allowed
    if (isSameOrigin(from, to)) return true;
    
    // TODO: Check CORS permissions
    return false;
}

std::vector<SiteSettingsEntry> SiteSettingsManager::getAllSites() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<SiteSettingsEntry> result;
    result.reserve(sites_.size());
    
    for (const auto& [origin, data] : sites_) {
        result.push_back(data.settings);
    }
    
    // Sort by display name
    std::sort(result.begin(), result.end(), 
        [](const SiteSettingsEntry& a, const SiteSettingsEntry& b) {
            return a.displayName < b.displayName;
        });
    
    return result;
}

SiteSettingsEntry SiteSettingsManager::getSiteSettings(const std::string& origin) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string normalized = normalizeOrigin(origin);
    auto it = sites_.find(normalized);
    
    if (it != sites_.end()) {
        return it->second.settings;
    }
    
    // Create default entry
    SiteSettingsEntry entry;
    entry.origin = normalized;
    
    // Extract display name from origin
    size_t schemeEnd = normalized.find("://");
    if (schemeEnd != std::string::npos) {
        entry.displayName = normalized.substr(schemeEnd + 3);
    } else {
        entry.displayName = normalized;
    }
    
    // Get permissions
    auto& pm = getPermissionManager();
    entry.camera = pm.query(PermissionType::Camera, normalized);
    entry.microphone = pm.query(PermissionType::Microphone, normalized);
    entry.geolocation = pm.query(PermissionType::Geolocation, normalized);
    entry.notifications = pm.query(PermissionType::Notifications, normalized);
    entry.clipboard = pm.query(PermissionType::Clipboard, normalized);
    
    // Get storage usage
    auto& qm = getQuotaManager();
    auto usage = qm.getUsage(normalized);
    entry.storageUsed = usage.total();
    entry.hasLocalStorage = usage.localStorage > 0;
    entry.hasIndexedDB = usage.indexedDB > 0;
    entry.hasCacheStorage = usage.cacheStorage > 0;
    
    return entry;
}

SiteDataSummary SiteSettingsManager::getSiteDataSummary(const std::string& origin) {
    std::string normalized = normalizeOrigin(origin);
    
    SiteDataSummary summary;
    summary.origin = normalized;
    
    // Get storage usage
    auto& qm = getQuotaManager();
    auto usage = qm.getUsage(normalized);
    summary.storageUsed = usage.total();
    summary.cacheSize = usage.cacheStorage;
    
    // Get permissions
    auto& pm = getPermissionManager();
    summary.permissions = pm.getPermissionsForOrigin(normalized);
    
    // Get last visit
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sites_.find(normalized);
    if (it != sites_.end()) {
        summary.lastVisit = it->second.lastVisit;
    }
    
    return summary;
}

void SiteSettingsManager::deleteSiteData(const std::string& origin) {
    std::string normalized = normalizeOrigin(origin);
    
    deleteSiteStorage(normalized);
    deleteSiteCookies(normalized);
    deleteSiteCache(normalized);
    deleteSitePermissions(normalized);
    
    std::lock_guard<std::mutex> lock(mutex_);
    sites_.erase(normalized);
    
    if (onChange_) onChange_(normalized);
}

void SiteSettingsManager::deleteSiteStorage(const std::string& origin) {
    std::string normalized = normalizeOrigin(origin);
    
    // Clear LocalStorage
    getLocalStorage(normalized).clear();
    
    // Clear quota
    getQuotaManager().clearOrigin(normalized);
}

void SiteSettingsManager::deleteSiteCookies(const std::string& origin) {
    // Would need to integrate with CookieManager
    // getCookieManager().deleteCookiesForDomain(extractDomain(origin));
}

void SiteSettingsManager::deleteSiteCache(const std::string& origin) {
    std::string normalized = normalizeOrigin(origin);
    
    // Clear CacheStorage
    auto& cacheStorage = getCacheStorage(normalized);
    for (const auto& name : cacheStorage.keys()) {
        cacheStorage.deleteCache(name);
    }
}

void SiteSettingsManager::deleteSitePermissions(const std::string& origin) {
    std::string normalized = normalizeOrigin(origin);
    getPermissionManager().resetOrigin(normalized);
}

void SiteSettingsManager::deleteAllSiteData() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& [origin, data] : sites_) {
        deleteSiteStorage(origin);
        deleteSiteCookies(origin);
        deleteSiteCache(origin);
        deleteSitePermissions(origin);
    }
    
    sites_.clear();
    clearAllLocalStorage();
}

void SiteSettingsManager::setPermission(const std::string& origin,
                                         PermissionType type,
                                         PermissionState state) {
    std::string normalized = normalizeOrigin(origin);
    auto& pm = getPermissionManager();
    
    switch (state) {
        case PermissionState::Granted:
            pm.grant(type, normalized);
            break;
        case PermissionState::Denied:
            pm.deny(type, normalized);
            break;
        case PermissionState::Prompt:
            pm.reset(type, normalized);
            break;
    }
    
    if (onChange_) onChange_(normalized);
}

void SiteSettingsManager::blockPermission(const std::string& origin,
                                           PermissionType type) {
    setPermission(origin, type, PermissionState::Denied);
}

void SiteSettingsManager::allowPermission(const std::string& origin,
                                           PermissionType type) {
    setPermission(origin, type, PermissionState::Granted);
}

void SiteSettingsManager::resetPermission(const std::string& origin,
                                           PermissionType type) {
    setPermission(origin, type, PermissionState::Prompt);
}

std::vector<std::string> SiteSettingsManager::getSitesWithPermission(
    PermissionType type, PermissionState state) {
    
    auto& pm = getPermissionManager();
    auto all = pm.getAllPermissions();
    
    std::vector<std::string> result;
    for (const auto& perm : all) {
        if (perm.type == type && perm.state == state) {
            result.push_back(perm.origin);
        }
    }
    
    return result;
}

void SiteSettingsManager::enableNotifications(const std::string& origin) {
    allowPermission(origin, PermissionType::Notifications);
}

void SiteSettingsManager::disableNotifications(const std::string& origin) {
    blockPermission(origin, PermissionType::Notifications);
}

bool SiteSettingsManager::areNotificationsEnabled(const std::string& origin) {
    return getPermissionManager().isGranted(PermissionType::Notifications,
                                            normalizeOrigin(origin));
}

std::vector<std::string> SiteSettingsManager::getNotificationSites() {
    return getSitesWithPermission(PermissionType::Notifications, 
                                   PermissionState::Granted);
}

void SiteSettingsManager::setJavaScriptEnabled(const std::string& origin, 
                                                bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string normalized = normalizeOrigin(origin);
    sites_[normalized].settings.javascriptEnabled = enabled;
    if (onChange_) onChange_(normalized);
}

void SiteSettingsManager::setImagesEnabled(const std::string& origin, 
                                            bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string normalized = normalizeOrigin(origin);
    sites_[normalized].settings.imagesEnabled = enabled;
    if (onChange_) onChange_(normalized);
}

void SiteSettingsManager::setPopupsAllowed(const std::string& origin, 
                                            bool allowed) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string normalized = normalizeOrigin(origin);
    sites_[normalized].settings.popupsAllowed = allowed;
    if (onChange_) onChange_(normalized);
}

void SiteSettingsManager::setSoundEnabled(const std::string& origin, 
                                           bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string normalized = normalizeOrigin(origin);
    sites_[normalized].settings.soundEnabled = enabled;
    if (onChange_) onChange_(normalized);
}

bool SiteSettingsManager::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    std::lock_guard<std::mutex> lock(mutex_);
    sites_.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string origin;
        int js, img, popup, sound;
        
        if (std::getline(iss, origin, '\t') &&
            (iss >> js >> img >> popup >> sound)) {
            SiteData data;
            data.settings.origin = origin;
            data.settings.javascriptEnabled = js != 0;
            data.settings.imagesEnabled = img != 0;
            data.settings.popupsAllowed = popup != 0;
            data.settings.soundEnabled = sound != 0;
            sites_[origin] = data;
        }
    }
    
    return true;
}

bool SiteSettingsManager::save(const std::string& path) {
    fs::path dir = fs::path(path).parent_path();
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }
    
    std::ofstream file(path);
    if (!file.is_open()) return false;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& [origin, data] : sites_) {
        file << origin << '\t'
             << data.settings.javascriptEnabled << '\t'
             << data.settings.imagesEnabled << '\t'
             << data.settings.popupsAllowed << '\t'
             << data.settings.soundEnabled << '\n';
    }
    
    return true;
}

void SiteSettingsManager::setOnSettingsChange(SettingsChangeCallback callback) {
    onChange_ = std::move(callback);
}

SiteSettingsManager& getSiteSettingsManager() {
    static SiteSettingsManager instance;
    return instance;
}

// =============================================================================
// StorageAccessGuard Implementation
// =============================================================================

StorageAccessGuard::StorageAccessGuard(const std::string& origin)
    : origin_(SiteSettingsManager::normalizeOrigin(origin)) {}

bool StorageAccessGuard::canAccess(const std::string& storageOrigin) const {
    return SiteSettingsManager::isSameOrigin(origin_, storageOrigin);
}

LocalStorage* StorageAccessGuard::getLocalStorage() {
    return &Zepra::Storage::getLocalStorage(origin_);
}

SessionStorage* StorageAccessGuard::getSessionStorage(const std::string& tabId) {
    return &Zepra::Storage::getSessionStorage(origin_, tabId);
}

} // namespace Zepra::Storage
