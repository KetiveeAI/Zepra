/**
 * @file permission_manager.hpp
 * @brief Site permissions management (camera, microphone, location, notifications)
 */

#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <vector>
#include <chrono>

namespace Zepra::Storage {

/**
 * @brief Permission types
 */
enum class PermissionType {
    Camera,
    Microphone,
    Geolocation,
    Notifications,
    Clipboard,
    Midi,
    Sensors,
    Bluetooth,
    USB,
    Serial,
    HID,
    ScreenCapture,
    PersistentStorage,
    BackgroundSync,
    Fullscreen,
    PaymentHandler,
    IdleDetection
};

/**
 * @brief Permission state
 */
enum class PermissionState {
    Prompt,     // User hasn't decided yet
    Granted,    // User allowed
    Denied      // User denied
};

/**
 * @brief Permission request info
 */
struct PermissionRequest {
    PermissionType type;
    std::string origin;
    std::chrono::system_clock::time_point requestTime;
    PermissionState state = PermissionState::Prompt;
};

/**
 * @brief Stored permission
 */
struct StoredPermission {
    PermissionType type;
    std::string origin;
    PermissionState state;
    std::chrono::system_clock::time_point grantedAt;
    std::chrono::system_clock::time_point expiresAt;  // 0 = never
    bool persistent = true;  // Survives browser restart
};

/**
 * @brief Permission manager callback types
 */
using PermissionCallback = std::function<void(PermissionState)>;
using PermissionUIHandler = std::function<void(const PermissionRequest&, PermissionCallback)>;

/**
 * @brief Permission Manager
 * 
 * Handles requesting, storing, and querying permissions.
 */
class PermissionManager {
public:
    PermissionManager();
    ~PermissionManager();
    
    // ===== Query Permissions =====
    
    /**
     * @brief Query permission state
     */
    PermissionState query(PermissionType type, const std::string& origin);
    
    /**
     * @brief Check if permission granted
     */
    bool isGranted(PermissionType type, const std::string& origin);
    
    /**
     * @brief Check if permission denied
     */
    bool isDenied(PermissionType type, const std::string& origin);
    
    // ===== Request Permissions =====
    
    /**
     * @brief Request permission (async, shows UI)
     */
    void request(PermissionType type, const std::string& origin, 
                 PermissionCallback callback);
    
    /**
     * @brief Request multiple permissions
     */
    void requestMultiple(const std::vector<PermissionType>& types,
                         const std::string& origin,
                         std::function<void(std::vector<PermissionState>)> callback);
    
    // ===== Manage Permissions =====
    
    /**
     * @brief Grant permission (for settings UI)
     */
    void grant(PermissionType type, const std::string& origin);
    
    /**
     * @brief Deny permission
     */
    void deny(PermissionType type, const std::string& origin);
    
    /**
     * @brief Reset permission to prompt
     */
    void reset(PermissionType type, const std::string& origin);
    
    /**
     * @brief Reset all permissions for origin
     */
    void resetOrigin(const std::string& origin);
    
    /**
     * @brief Reset all permissions
     */
    void resetAll();
    
    // ===== Get All Permissions =====
    
    /**
     * @brief Get all permissions for origin
     */
    std::vector<StoredPermission> getPermissionsForOrigin(const std::string& origin);
    
    /**
     * @brief Get all stored permissions
     */
    std::vector<StoredPermission> getAllPermissions();
    
    // ===== UI Handler =====
    
    /**
     * @brief Set UI handler for permission prompts
     */
    void setUIHandler(PermissionUIHandler handler) { uiHandler_ = std::move(handler); }
    
    // ===== Persistence =====
    
    /**
     * @brief Load permissions from disk
     */
    bool load(const std::string& path);
    
    /**
     * @brief Save permissions to disk
     */
    bool save(const std::string& path);
    
    // ===== Utilities =====
    
    /**
     * @brief Convert permission type to string
     */
    static std::string typeToString(PermissionType type);
    
    /**
     * @brief Parse permission type from string
     */
    static PermissionType stringToType(const std::string& str);
    
    /**
     * @brief Check if permission requires secure context (HTTPS)
     */
    static bool requiresSecureContext(PermissionType type);
    
private:
    std::string makeKey(PermissionType type, const std::string& origin) const;
    void storePermission(PermissionType type, const std::string& origin, 
                         PermissionState state);
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, StoredPermission> permissions_;
    PermissionUIHandler uiHandler_;
};

/**
 * @brief Global permission manager
 */
PermissionManager& getPermissionManager();

} // namespace Zepra::Storage
