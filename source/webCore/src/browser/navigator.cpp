/**
 * @file navigator.cpp
 * @brief Navigator, Clipboard, Geolocation, MediaDevices implementation
 */

#include "webcore/browser/navigator.hpp"
#include <chrono>
#include <functional>
#include <unordered_map>

namespace Zepra::WebCore {

// =============================================================================
// NetworkInformation
// =============================================================================

class NetworkInformation::Impl {
public:
    ConnectionType type = ConnectionType::Unknown;
    std::string effectiveType = "4g";
    double downlink = 10.0;
    unsigned int rtt = 50;
    bool saveData = false;
};

NetworkInformation::NetworkInformation() : impl_(std::make_unique<Impl>()) {}

std::string NetworkInformation::type() const {
    switch (impl_->type) {
        case ConnectionType::Ethernet: return "ethernet";
        case ConnectionType::Wifi: return "wifi";
        case ConnectionType::Cellular2G: return "cellular";
        case ConnectionType::Cellular3G: return "cellular";
        case ConnectionType::Cellular4G: return "cellular";
        case ConnectionType::Cellular5G: return "cellular";
        case ConnectionType::None: return "none";
        default: return "unknown";
    }
}

std::string NetworkInformation::effectiveType() const { return impl_->effectiveType; }
double NetworkInformation::downlink() const { return impl_->downlink; }
unsigned int NetworkInformation::rtt() const { return impl_->rtt; }
bool NetworkInformation::saveData() const { return impl_->saveData; }

// =============================================================================
// Navigator::Impl
// =============================================================================

class Navigator::Impl {
public:
    std::unique_ptr<Clipboard> clipboard;
    std::unique_ptr<NetworkInformation> connection;
    std::unique_ptr<Geolocation> geolocation;
    std::unique_ptr<MediaDevices> mediaDevices;
    
    std::string userAgent = "Mozilla/5.0 (X11; Linux x86_64) ZepraBrowser/1.0";
    std::string platform = "Linux x86_64";
    std::string language = "en-US";
    std::vector<std::string> languages = {"en-US", "en"};
    bool online = true;
    unsigned int hardwareConcurrency = 4;
    double deviceMemory = 8.0;
    unsigned int maxTouchPoints = 0;
};

// =============================================================================
// Navigator
// =============================================================================

Navigator::Navigator(Window* window)
    : impl_(std::make_unique<Impl>()), window_(window) {
    impl_->clipboard = std::make_unique<Clipboard>();
    impl_->connection = std::make_unique<NetworkInformation>();
    impl_->geolocation = std::make_unique<Geolocation>();
    impl_->mediaDevices = std::make_unique<MediaDevices>();
}

Navigator::~Navigator() = default;

std::string Navigator::appCodeName() const { return "Mozilla"; }
std::string Navigator::appName() const { return "ZepraBrowser"; }
std::string Navigator::appVersion() const { return "1.0"; }
std::string Navigator::userAgent() const { return impl_->userAgent; }
std::string Navigator::platform() const { return impl_->platform; }
std::string Navigator::product() const { return "Gecko"; }
std::string Navigator::vendor() const { return "ZepraBrowser"; }
bool Navigator::cookieEnabled() const { return true; }
std::string Navigator::language() const { return impl_->language; }
std::vector<std::string> Navigator::languages() const { return impl_->languages; }
bool Navigator::onLine() const { return impl_->online; }
unsigned int Navigator::hardwareConcurrency() const { return impl_->hardwareConcurrency; }
double Navigator::deviceMemory() const { return impl_->deviceMemory; }
unsigned int Navigator::maxTouchPoints() const { return impl_->maxTouchPoints; }
bool Navigator::pdfViewerEnabled() const { return true; }
bool Navigator::webdriver() const { return false; }

Clipboard* Navigator::clipboard() { return impl_->clipboard.get(); }
NetworkInformation* Navigator::connection() { return impl_->connection.get(); }
Geolocation* Navigator::geolocation() { return impl_->geolocation.get(); }
MediaDevices* Navigator::mediaDevices() { return impl_->mediaDevices.get(); }

bool Navigator::canShare() const { return false; }

bool Navigator::sendBeacon(const std::string& /*url*/, const std::string& /*data*/) {
    // Would send data via networking layer
    return true;
}

bool Navigator::vibrate(unsigned int /*duration*/) {
    // Would trigger vibration on mobile
    return false;
}

bool Navigator::vibrate(const std::vector<unsigned int>& /*pattern*/) {
    return false;
}

void Navigator::registerProtocolHandler(const std::string& /*scheme*/, 
                                         const std::string& /*url*/,
                                         const std::string& /*title*/) {
    // Would register custom protocol handler
}

void Navigator::unregisterProtocolHandler(const std::string& /*scheme*/,
                                           const std::string& /*url*/) {
    // Would unregister protocol handler
}

// =============================================================================
// Clipboard
// =============================================================================

class Clipboard::Impl {
public:
    std::string text;
};

Clipboard::Clipboard() : impl_(std::make_unique<Impl>()) {}
Clipboard::~Clipboard() = default;

void Clipboard::readText(std::function<void(const std::string&)> callback) {
    if (callback) {
        callback(impl_->text);
    }
}

void Clipboard::writeText(const std::string& text, std::function<void(bool)> callback) {
    impl_->text = text;
    if (callback) {
        callback(true);
    }
}

// =============================================================================
// Geolocation
// =============================================================================

class Geolocation::Impl {
public:
    int nextWatchId = 1;
    std::unordered_map<int, std::function<void(const Position&)>> watches;
};

Geolocation::Geolocation() : impl_(std::make_unique<Impl>()) {}
Geolocation::~Geolocation() = default;

void Geolocation::getCurrentPosition(
    std::function<void(const Position&)> success,
    std::function<void(int, const std::string&)> error) {
    
    // In production, would query system location services
    // For now, return a default position or error
    
    if (error) {
        error(1, "Geolocation not supported in this build");
    }
    (void)success;
}

int Geolocation::watchPosition(
    std::function<void(const Position&)> success,
    std::function<void(int, const std::string&)> error) {
    
    int id = impl_->nextWatchId++;
    impl_->watches[id] = success;
    
    (void)error;
    return id;
}

void Geolocation::clearWatch(int watchId) {
    impl_->watches.erase(watchId);
}

// =============================================================================
// MediaDevices
// =============================================================================

class MediaDevices::Impl {
public:
    std::vector<MediaDeviceInfo> devices;
};

MediaDevices::MediaDevices() : impl_(std::make_unique<Impl>()) {}
MediaDevices::~MediaDevices() = default;

void MediaDevices::enumerateDevices(std::function<void(const std::vector<MediaDeviceInfo>&)> callback) {
    if (callback) {
        callback(impl_->devices);
    }
}

} // namespace Zepra::WebCore
