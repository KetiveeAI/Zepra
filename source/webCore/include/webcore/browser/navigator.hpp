/**
 * @file navigator.hpp
 * @brief Navigator interface for browser information
 *
 * Provides information about the browser and system.
 *
 * @see https://developer.mozilla.org/en-US/docs/Web/API/Navigator
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Zepra::WebCore {

// Forward declarations
class Window;
class Clipboard;
class Geolocation;
class MediaDevices;

/**
 * @brief Network connection type
 */
enum class ConnectionType {
    Unknown,
    Ethernet,
    Wifi,
    Cellular2G,
    Cellular3G,
    Cellular4G,
    Cellular5G,
    None
};

/**
 * @brief Network connection information
 */
class NetworkInformation {
public:
    NetworkInformation();
    
    /// Connection type
    std::string type() const;
    
    /// Effective connection type (slow-2g, 2g, 3g, 4g)
    std::string effectiveType() const;
    
    /// Downlink speed in Mbps
    double downlink() const;
    
    /// Round-trip time in ms
    unsigned int rtt() const;
    
    /// Save-data mode enabled
    bool saveData() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Navigator provides browser and system information
 */
class Navigator {
public:
    explicit Navigator(Window* window);
    ~Navigator();

    // =========================================================================
    // Standard Properties
    // =========================================================================

    /// Browser code name (always "Mozilla")
    std::string appCodeName() const;

    /// Browser name
    std::string appName() const;

    /// Browser version
    std::string appVersion() const;

    /// Full user agent string
    std::string userAgent() const;

    /// Platform (e.g. "Linux x86_64")
    std::string platform() const;

    /// Browser product name
    std::string product() const;

    /// Vendor name
    std::string vendor() const;

    /// Whether cookies are enabled
    bool cookieEnabled() const;

    /// Browser language (e.g. "en-US")
    std::string language() const;

    /// All preferred languages
    std::vector<std::string> languages() const;

    /// Whether browser is online
    bool onLine() const;

    /// Hardware concurrency (logical CPU cores)
    unsigned int hardwareConcurrency() const;

    /// Device memory in GB
    double deviceMemory() const;

    /// Maximum touch points
    unsigned int maxTouchPoints() const;

    /// PDF viewer enabled
    bool pdfViewerEnabled() const;

    /// Webdriver mode
    bool webdriver() const;

    // =========================================================================
    // Sub-interfaces
    // =========================================================================

    /// Clipboard access
    Clipboard* clipboard();

    /// Network connection info
    NetworkInformation* connection();

    /// Geolocation
    Geolocation* geolocation();

    /// Media devices
    MediaDevices* mediaDevices();

    // =========================================================================
    // Methods
    // =========================================================================

    /// Check if browser can share
    bool canShare() const;

    /// Send beacon data
    bool sendBeacon(const std::string& url, const std::string& data);

    /// Vibrate device
    bool vibrate(unsigned int duration);
    bool vibrate(const std::vector<unsigned int>& pattern);

    /// Register protocol handler
    void registerProtocolHandler(const std::string& scheme, 
                                  const std::string& url,
                                  const std::string& title = "");

    /// Unregister protocol handler
    void unregisterProtocolHandler(const std::string& scheme,
                                    const std::string& url);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    Window* window_;
};

/**
 * @brief Clipboard interface
 */
class Clipboard {
public:
    Clipboard();
    ~Clipboard();

    /// Read text from clipboard
    void readText(std::function<void(const std::string&)> callback);

    /// Write text to clipboard
    void writeText(const std::string& text, std::function<void(bool)> callback = nullptr);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Geolocation interface
 */
class Geolocation {
public:
    Geolocation();
    ~Geolocation();

    struct Position {
        double latitude;
        double longitude;
        double altitude;
        double accuracy;
        double altitudeAccuracy;
        double heading;
        double speed;
        long long timestamp;
    };

    /// Get current position
    void getCurrentPosition(
        std::function<void(const Position&)> success,
        std::function<void(int code, const std::string& message)> error = nullptr);

    /// Watch position changes
    int watchPosition(
        std::function<void(const Position&)> success,
        std::function<void(int code, const std::string& message)> error = nullptr);

    /// Stop watching position
    void clearWatch(int watchId);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief MediaDevices interface
 */
class MediaDevices {
public:
    MediaDevices();
    ~MediaDevices();

    struct MediaDeviceInfo {
        std::string deviceId;
        std::string groupId;
        std::string kind;  // "audioinput", "audiooutput", "videoinput"
        std::string label;
    };

    /// Enumerate available devices
    void enumerateDevices(std::function<void(const std::vector<MediaDeviceInfo>&)> callback);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Zepra::WebCore
