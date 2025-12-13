/**
 * @file permission_manager.cpp
 * @brief Site permission manager implementation
 */

#include "storage/permission_manager.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace Zepra::Storage {

// =============================================================================
// PermissionManager Implementation
// =============================================================================

PermissionManager::PermissionManager() = default;
PermissionManager::~PermissionManager() = default;

std::string PermissionManager::makeKey(PermissionType type, 
                                        const std::string& origin) const {
    return typeToString(type) + "|" + origin;
}

PermissionState PermissionManager::query(PermissionType type, 
                                          const std::string& origin) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeKey(type, origin);
    auto it = permissions_.find(key);
    
    if (it != permissions_.end()) {
        // Check expiration
        if (it->second.expiresAt != std::chrono::system_clock::time_point{}) {
            if (std::chrono::system_clock::now() > it->second.expiresAt) {
                permissions_.erase(it);
                return PermissionState::Prompt;
            }
        }
        return it->second.state;
    }
    
    return PermissionState::Prompt;
}

bool PermissionManager::isGranted(PermissionType type, const std::string& origin) {
    return query(type, origin) == PermissionState::Granted;
}

bool PermissionManager::isDenied(PermissionType type, const std::string& origin) {
    return query(type, origin) == PermissionState::Denied;
}

void PermissionManager::request(PermissionType type, const std::string& origin,
                                 PermissionCallback callback) {
    // Check if already decided
    PermissionState current = query(type, origin);
    if (current != PermissionState::Prompt) {
        if (callback) callback(current);
        return;
    }
    
    // Check if requires secure context
    if (requiresSecureContext(type)) {
        // Check origin is https
        if (origin.substr(0, 5) != "https") {
            if (callback) callback(PermissionState::Denied);
            return;
        }
    }
    
    // Show UI if handler set
    if (uiHandler_) {
        PermissionRequest request;
        request.type = type;
        request.origin = origin;
        request.requestTime = std::chrono::system_clock::now();
        
        uiHandler_(request, [this, type, origin, callback](PermissionState state) {
            storePermission(type, origin, state);
            if (callback) callback(state);
        });
    } else {
        // No UI, default deny
        if (callback) callback(PermissionState::Denied);
    }
}

void PermissionManager::requestMultiple(const std::vector<PermissionType>& types,
                                         const std::string& origin,
                                         std::function<void(std::vector<PermissionState>)> callback) {
    std::vector<PermissionState> results;
    results.reserve(types.size());
    
    // For now, request each sequentially
    for (PermissionType type : types) {
        results.push_back(query(type, origin));
    }
    
    if (callback) callback(results);
}

void PermissionManager::grant(PermissionType type, const std::string& origin) {
    storePermission(type, origin, PermissionState::Granted);
}

void PermissionManager::deny(PermissionType type, const std::string& origin) {
    storePermission(type, origin, PermissionState::Denied);
}

void PermissionManager::reset(PermissionType type, const std::string& origin) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = makeKey(type, origin);
    permissions_.erase(key);
}

void PermissionManager::resetOrigin(const std::string& origin) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto it = permissions_.begin(); it != permissions_.end();) {
        if (it->second.origin == origin) {
            it = permissions_.erase(it);
        } else {
            ++it;
        }
    }
}

void PermissionManager::resetAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    permissions_.clear();
}

std::vector<StoredPermission> PermissionManager::getPermissionsForOrigin(
    const std::string& origin) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<StoredPermission> result;
    for (const auto& [key, perm] : permissions_) {
        if (perm.origin == origin) {
            result.push_back(perm);
        }
    }
    return result;
}

std::vector<StoredPermission> PermissionManager::getAllPermissions() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<StoredPermission> result;
    result.reserve(permissions_.size());
    for (const auto& [key, perm] : permissions_) {
        result.push_back(perm);
    }
    return result;
}

void PermissionManager::storePermission(PermissionType type, 
                                         const std::string& origin,
                                         PermissionState state) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string key = makeKey(type, origin);
    
    StoredPermission perm;
    perm.type = type;
    perm.origin = origin;
    perm.state = state;
    perm.grantedAt = std::chrono::system_clock::now();
    perm.persistent = true;
    
    permissions_[key] = perm;
}

bool PermissionManager::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    std::lock_guard<std::mutex> lock(mutex_);
    permissions_.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string typeStr, origin, stateStr;
        
        if (std::getline(iss, typeStr, '\t') &&
            std::getline(iss, origin, '\t') &&
            std::getline(iss, stateStr, '\t')) {
            
            StoredPermission perm;
            perm.type = stringToType(typeStr);
            perm.origin = origin;
            perm.state = (stateStr == "granted") ? PermissionState::Granted :
                         (stateStr == "denied") ? PermissionState::Denied :
                         PermissionState::Prompt;
            perm.grantedAt = std::chrono::system_clock::now();
            perm.persistent = true;
            
            std::string key = makeKey(perm.type, perm.origin);
            permissions_[key] = perm;
        }
    }
    
    return true;
}

bool PermissionManager::save(const std::string& path) {
    fs::path dir = fs::path(path).parent_path();
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }
    
    std::ofstream file(path);
    if (!file.is_open()) return false;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& [key, perm] : permissions_) {
        if (!perm.persistent) continue;
        
        std::string stateStr = (perm.state == PermissionState::Granted) ? "granted" :
                               (perm.state == PermissionState::Denied) ? "denied" : "prompt";
        
        file << typeToString(perm.type) << '\t' 
             << perm.origin << '\t'
             << stateStr << '\n';
    }
    
    return true;
}

std::string PermissionManager::typeToString(PermissionType type) {
    switch (type) {
        case PermissionType::Camera: return "camera";
        case PermissionType::Microphone: return "microphone";
        case PermissionType::Geolocation: return "geolocation";
        case PermissionType::Notifications: return "notifications";
        case PermissionType::Clipboard: return "clipboard-write";
        case PermissionType::Midi: return "midi";
        case PermissionType::Sensors: return "accelerometer";
        case PermissionType::Bluetooth: return "bluetooth";
        case PermissionType::USB: return "usb";
        case PermissionType::Serial: return "serial";
        case PermissionType::HID: return "hid";
        case PermissionType::ScreenCapture: return "display-capture";
        case PermissionType::PersistentStorage: return "persistent-storage";
        case PermissionType::BackgroundSync: return "background-sync";
        case PermissionType::Fullscreen: return "fullscreen";
        case PermissionType::PaymentHandler: return "payment-handler";
        case PermissionType::IdleDetection: return "idle-detection";
    }
    return "unknown";
}

PermissionType PermissionManager::stringToType(const std::string& str) {
    if (str == "camera") return PermissionType::Camera;
    if (str == "microphone") return PermissionType::Microphone;
    if (str == "geolocation") return PermissionType::Geolocation;
    if (str == "notifications") return PermissionType::Notifications;
    if (str == "clipboard-write") return PermissionType::Clipboard;
    if (str == "midi") return PermissionType::Midi;
    if (str == "accelerometer") return PermissionType::Sensors;
    if (str == "bluetooth") return PermissionType::Bluetooth;
    if (str == "usb") return PermissionType::USB;
    if (str == "serial") return PermissionType::Serial;
    if (str == "hid") return PermissionType::HID;
    if (str == "display-capture") return PermissionType::ScreenCapture;
    if (str == "persistent-storage") return PermissionType::PersistentStorage;
    if (str == "background-sync") return PermissionType::BackgroundSync;
    if (str == "fullscreen") return PermissionType::Fullscreen;
    if (str == "payment-handler") return PermissionType::PaymentHandler;
    if (str == "idle-detection") return PermissionType::IdleDetection;
    return PermissionType::Camera;  // Default
}

bool PermissionManager::requiresSecureContext(PermissionType type) {
    switch (type) {
        case PermissionType::Camera:
        case PermissionType::Microphone:
        case PermissionType::Geolocation:
        case PermissionType::Bluetooth:
        case PermissionType::USB:
        case PermissionType::Serial:
        case PermissionType::HID:
        case PermissionType::ScreenCapture:
        case PermissionType::PaymentHandler:
        case PermissionType::IdleDetection:
            return true;
        default:
            return false;
    }
}

// =============================================================================
// Global Instance
// =============================================================================

PermissionManager& getPermissionManager() {
    static PermissionManager instance;
    return instance;
}

} // namespace Zepra::Storage
