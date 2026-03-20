/**
 * @file privacy_settings_ui.hpp
 * @brief Privacy and Site Settings UI components
 */

#pragma once

#include "storage/site_settings.hpp"
#include "storage/permission_manager.hpp"

#include <string>
#include <vector>
#include <functional>

namespace Zepra::UI {

/**
 * @brief Permission toggle item for UI
 */
struct PermissionToggle {
    Storage::PermissionType type;
    std::string name;
    std::string description;
    std::string icon;
    Storage::PermissionState state;
    bool requiresSecureContext;
};

/**
 * @brief Storage item for UI
 */
struct StorageItem {
    std::string type;  // "LocalStorage", "IndexedDB", "Cache", "Cookies"
    size_t size;
    std::string sizeFormatted;
    bool canDelete;
};

/**
 * @brief Privacy Settings Panel
 * 
 * Provides UI model for browser privacy settings.
 */
class PrivacySettingsPanel {
public:
    PrivacySettingsPanel();
    ~PrivacySettingsPanel();
    
    // ===== Global Settings =====
    
    /**
     * @brief Clear all browsing data
     */
    void clearBrowsingData(bool cookies, bool cache, bool localStorage,
                           bool history, bool passwords);
    
    /**
     * @brief Get/Set third-party cookies blocked
     */
    bool isThirdPartyCookiesBlocked() const { return blockThirdPartyCookies_; }
    void setBlockThirdPartyCookies(bool block);
    
    /**
     * @brief Get/Set Do Not Track
     */
    bool isDoNotTrackEnabled() const { return doNotTrack_; }
    void setDoNotTrack(bool enabled);
    
    /**
     * @brief Get/Set Enhanced Tracking Protection
     */
    bool isTrackingProtectionEnabled() const { return trackingProtection_; }
    void setTrackingProtection(bool enabled);
    
    // ===== Site-Specific Settings =====
    
    /**
     * @brief Get all permission types
     */
    std::vector<PermissionToggle> getPermissionToggles(const std::string& origin);
    
    /**
     * @brief Set permission state
     */
    void setPermissionState(const std::string& origin, 
                            Storage::PermissionType type,
                            Storage::PermissionState state);
    
    /**
     * @brief Get storage breakdown for site
     */
    std::vector<StorageItem> getStorageItems(const std::string& origin);
    
    /**
     * @brief Delete storage item
     */
    void deleteStorageItem(const std::string& origin, const std::string& type);
    
    // ===== Sites List =====
    
    /**
     * @brief Get all sites with data/permissions
     */
    std::vector<Storage::SiteSettingsEntry> getAllSites();
    
    /**
     * @brief Search sites
     */
    std::vector<Storage::SiteSettingsEntry> searchSites(const std::string& query);
    
    /**
     * @brief Delete site data
     */
    void deleteSiteData(const std::string& origin);
    
    // ===== Notification Settings =====
    
    /**
     * @brief Get sites with notifications
     */
    struct NotificationSite {
        std::string origin;
        std::string displayName;
        bool enabled;
    };
    std::vector<NotificationSite> getNotificationSites();
    
    /**
     * @brief Toggle notifications for site
     */
    void toggleNotifications(const std::string& origin, bool enabled);
    
    /**
     * @brief Block all notifications
     */
    void blockAllNotifications();
    
    // ===== Cookie Settings =====
    
    /**
     * @brief Cookie setting
     */
    enum class CookieSetting {
        AllowAll,
        BlockThirdParty,
        BlockAll
    };
    
    CookieSetting getCookieSetting() const { return cookieSetting_; }
    void setCookieSetting(CookieSetting setting);
    
    /**
     * @brief Get cookie exceptions
     */
    struct CookieException {
        std::string origin;
        bool allowed;
    };
    std::vector<CookieException> getCookieExceptions();
    void addCookieException(const std::string& origin, bool allowed);
    void removeCookieException(const std::string& origin);
    
    // ===== Persistence =====
    
    bool load(const std::string& path);
    bool save(const std::string& path);
    
    // ===== UI Helpers =====
    
    /**
     * @brief Format bytes for display
     */
    static std::string formatBytes(size_t bytes);
    
    /**
     * @brief Get permission display name
     */
    static std::string getPermissionName(Storage::PermissionType type);
    
    /**
     * @brief Get permission icon name
     */
    static std::string getPermissionIcon(Storage::PermissionType type);
    
private:
    bool blockThirdPartyCookies_ = true;
    bool doNotTrack_ = false;
    bool trackingProtection_ = true;
    CookieSetting cookieSetting_ = CookieSetting::BlockThirdParty;
    
    std::vector<CookieException> cookieExceptions_;
};

/**
 * @brief Global privacy settings panel
 */
PrivacySettingsPanel& getPrivacySettingsPanel();

} // namespace Zepra::UI
