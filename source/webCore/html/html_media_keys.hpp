/**
 * @file html_media_keys.hpp
 * @brief Encrypted Media Extensions (EME) interfaces
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <cstdint>

namespace Zepra::WebCore {

/**
 * @brief Media key status
 */
enum class MediaKeyStatus {
    Usable,
    Expired,
    Released,
    OutputRestricted,
    OutputDownscaled,
    StatusPending,
    InternalError
};

/**
 * @brief Media key session type
 */
enum class MediaKeySessionType {
    Temporary,
    PersistentLicense
};

/**
 * @brief Media keys system configuration
 */
struct MediaKeySystemConfiguration {
    std::vector<std::string> initDataTypes;
    std::vector<std::string> audioCapabilities;
    std::vector<std::string> videoCapabilities;
    std::string distinctiveIdentifier;  // required, optional, not-allowed
    std::string persistentState;
    std::string sessionTypes;
};

/**
 * @brief Media key session
 */
class MediaKeySession {
public:
    virtual ~MediaKeySession() = default;
    
    virtual std::string sessionId() const = 0;
    virtual double expiration() const = 0;
    
    virtual void generateRequest(const std::string& initDataType,
                                  const std::vector<uint8_t>& initData) = 0;
    virtual void load(const std::string& sessionId) = 0;
    virtual void update(const std::vector<uint8_t>& response) = 0;
    virtual void close() = 0;
    virtual void remove() = 0;
    
    // Events
    std::function<void(const std::vector<uint8_t>&)> onMessage;
    std::function<void()> onKeyStatusChange;
};

/**
 * @brief Media keys
 */
class MediaKeys {
public:
    virtual ~MediaKeys() = default;
    
    virtual std::unique_ptr<MediaKeySession> createSession(
        MediaKeySessionType type = MediaKeySessionType::Temporary) = 0;
    
    virtual bool setServerCertificate(const std::vector<uint8_t>& cert) = 0;
};

/**
 * @brief Media key system access
 */
class MediaKeySystemAccess {
public:
    virtual ~MediaKeySystemAccess() = default;
    
    virtual std::string keySystem() const = 0;
    virtual MediaKeySystemConfiguration getConfiguration() const = 0;
    virtual std::unique_ptr<MediaKeys> createMediaKeys() = 0;
};

/**
 * @brief Encrypted media event
 */
struct MediaEncryptedEvent {
    std::string initDataType;
    std::vector<uint8_t> initData;
};

} // namespace Zepra::WebCore
