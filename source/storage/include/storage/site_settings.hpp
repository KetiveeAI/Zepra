/**
 * @file site_settings.hpp
 * @brief Site-specific settings UI and data management
 * 
 * Provides UI for managing per-site permissions, storage, and privacy settings.
 */

#pragma once

#include "storage/permission_manager.hpp"
#include "storage/local_storage.hpp"
#include "storage/quota_manager.hpp"

#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace Zepra::Storage {

/**
 * @brief Site data summary
 */
struct SiteDataSummary {
    std::string origin;
    size_t storageUsed = 0;
    size_t cookieCount = 0;
    size_t cacheSize = 0;
    std::chrono::system_clock::time_point lastVisit;
    std::vector<StoredPermission> permissions;
};

/**
 * @brief Site settings entry for UI
 */
struct SiteSettingsEntry {
    std::string origin;
    std::string displayName;  // e.g., "example.com" from "https://example.com"
    std::string favicon;      // Path to cached favicon
    
    // Permission states
    PermissionState camera = PermissionState::Prompt;
    PermissionState microphone = PermissionState::Prompt;
    PermissionState geolocation = PermissionState::Prompt;
    PermissionState notifications = PermissionState::Prompt;
    PermissionState clipboard = PermissionState::Prompt;
    
    // Storage
    size_t storageUsed = 0;
    bool hasLocalStorage = false;
    bool hasIndexedDB = false;
    bool hasCacheStorage = false;
    bool hasCookies = false;
    
    // Content settings
    bool popupsAllowed = false;
    bool javascriptEnabled = true;
    bool imagesEnabled = true;
    bool soundEnabled = true;
};

/**
 * @brief Site Settings Manager
 * 
 * Manages per-site settings, permissions, and data.
 * Enforces origin isolation for storage.
 */
class SiteSettingsManager {
public:
    SiteSettingsManager();
    ~SiteSettingsManager();
    
    // ===== Origin Isolation =====
    
    /**
     * @brief Normalize origin from URL
     */
    static std::string normalizeOrigin(const std::string& url);
    
    /**
     * @brief Check if origin matches (for same-origin policy)
     */
    static bool isSameOrigin(const std::string& origin1, const std::string& origin2);
    
    /**
     * @brief Check if cross-origin access is allowed
     */
    bool isCrossOriginAllowed(const std::string& from, const std::string& to);
    
    // ===== Site Data Management =====
    
    /**
     * @brief Get all sites with stored data
     */
    std::vector<SiteSettingsEntry> getAllSites();
    
    /**
     * @brief Get settings for specific site
     */
    SiteSettingsEntry getSiteSettings(const std::string& origin);
    
    /**
     * @brief Get data summary for site
     */
    SiteDataSummary getSiteDataSummary(const std::string& origin);
    
    /**
     * @brief Delete all data for site
     */
    void deleteSiteData(const std::string& origin);
    
    /**
     * @brief Delete specific data types for site
     */
    void deleteSiteStorage(const std::string& origin);
    void deleteSiteCookies(const std::string& origin);
    void deleteSiteCache(const std::string& origin);
    void deleteSitePermissions(const std::string& origin);
    
    /**
     * @brief Delete all site data
     */
    void deleteAllSiteData();
    
    // ===== Permission Management =====
    
    /**
     * @brief Set permission for site
     */
    void setPermission(const std::string& origin, PermissionType type, 
                       PermissionState state);
    
    /**
     * @brief Block permission for site
     */
    void blockPermission(const std::string& origin, PermissionType type);
    
    /**
     * @brief Allow permission for site
     */
    void allowPermission(const std::string& origin, PermissionType type);
    
    /**
     * @brief Reset permission to ask
     */
    void resetPermission(const std::string& origin, PermissionType type);
    
    /**
     * @brief Get sites with specific permission
     */
    std::vector<std::string> getSitesWithPermission(PermissionType type, 
                                                     PermissionState state);
    
    // ===== Notification Settings =====
    
    /**
     * @brief Enable notifications for site
     */
    void enableNotifications(const std::string& origin);
    
    /**
     * @brief Disable notifications for site
     */
    void disableNotifications(const std::string& origin);
    
    /**
     * @brief Check if notifications enabled
     */
    bool areNotificationsEnabled(const std::string& origin);
    
    /**
     * @brief Get all sites with notifications enabled
     */
    std::vector<std::string> getNotificationSites();
    
    // ===== Content Settings =====
    
    /**
     * @brief Set JavaScript enabled for site
     */
    void setJavaScriptEnabled(const std::string& origin, bool enabled);
    
    /**
     * @brief Set images enabled for site
     */
    void setImagesEnabled(const std::string& origin, bool enabled);
    
    /**
     * @brief Set popups allowed for site
     */
    void setPopupsAllowed(const std::string& origin, bool allowed);
    
    /**
     * @brief Set sound enabled for site
     */
    void setSoundEnabled(const std::string& origin, bool enabled);
    
    // ===== Persistence =====
    
    /**
     * @brief Load settings from disk
     */
    bool load(const std::string& path);
    
    /**
     * @brief Save settings to disk
     */
    bool save(const std::string& path);
    
    // ===== UI Callbacks =====
    
    /**
     * @brief Callback when settings change
     */
    using SettingsChangeCallback = std::function<void(const std::string& origin)>;
    void setOnSettingsChange(SettingsChangeCallback callback);
    
private:
    struct SiteData {
        SiteSettingsEntry settings;
        std::chrono::system_clock::time_point lastVisit;
    };
    
    std::mutex mutex_;
    std::unordered_map<std::string, SiteData> sites_;
    SettingsChangeCallback onChange_;
};

/**
 * @brief Global site settings manager
 */
SiteSettingsManager& getSiteSettingsManager();

// =============================================================================
// Storage Isolation Helper
// =============================================================================

/**
 * @brief Storage access guard - enforces same-origin policy
 */
class StorageAccessGuard {
public:
    explicit StorageAccessGuard(const std::string& origin);
    
    /**
     * @brief Check if storage access allowed
     */
    bool canAccess(const std::string& storageOrigin) const;
    
    /**
     * @brief Get guarded LocalStorage
     */
    LocalStorage* getLocalStorage();
    
    /**
     * @brief Get guarded SessionStorage
     */
    class SessionStorage* getSessionStorage(const std::string& tabId);
    
private:
    std::string origin_;
};

} // namespace Zepra::Storage
