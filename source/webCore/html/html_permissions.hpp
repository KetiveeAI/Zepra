/**
 * @file html_permissions.hpp
 * @brief Permissions API
 */

#pragma once

#include <string>
#include <functional>
#include <vector>

namespace Zepra::WebCore {

/**
 * @brief Permission state
 */
enum class PermissionState {
    Granted,
    Denied,
    Prompt
};

/**
 * @brief Permission name
 */
enum class PermissionName {
    Geolocation,
    Notifications,
    Push,
    Midi,
    Camera,
    Microphone,
    SpeakerSelection,
    DeviceInfo,
    BackgroundFetch,
    BackgroundSync,
    Bluetooth,
    PersistentStorage,
    AmbientLightSensor,
    Accelerometer,
    Gyroscope,
    Magnetometer,
    ClipboardRead,
    ClipboardWrite,
    DisplayCapture,
    NFC,
    IdleDetection,
    WakeLock
};

/**
 * @brief Permission descriptor
 */
struct PermissionDescriptor {
    PermissionName name;
    bool userVisibleOnly = false;  // For push
    bool sysex = false;  // For midi
};

/**
 * @brief Permission status
 */
class PermissionStatus {
public:
    PermissionStatus(PermissionName name, PermissionState state);
    ~PermissionStatus() = default;
    
    PermissionState state() const { return state_; }
    PermissionName name() const { return name_; }
    
    std::function<void()> onChange;
    
private:
    PermissionName name_;
    PermissionState state_;
};

/**
 * @brief Permissions API
 */
class Permissions {
public:
    Permissions() = default;
    ~Permissions() = default;
    
    void query(const PermissionDescriptor& descriptor,
               std::function<void(PermissionStatus*)> callback);
    
    void request(const PermissionDescriptor& descriptor,
                 std::function<void(PermissionStatus*)> callback);
    
    void revoke(const PermissionDescriptor& descriptor,
                std::function<void(PermissionStatus*)> callback);
    
private:
    std::vector<std::unique_ptr<PermissionStatus>> statuses_;
};

} // namespace Zepra::WebCore
