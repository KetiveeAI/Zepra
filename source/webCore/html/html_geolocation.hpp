/**
 * @file html_geolocation.hpp
 * @brief Geolocation API
 */

#pragma once

#include <functional>
#include <cstdint>

namespace Zepra::WebCore {

/**
 * @brief Geolocation coordinates
 */
struct GeolocationCoordinates {
    double latitude = 0;
    double longitude = 0;
    double altitude = 0;
    double accuracy = 0;
    double altitudeAccuracy = 0;
    double heading = 0;
    double speed = 0;
};

/**
 * @brief Geolocation position
 */
struct GeolocationPosition {
    GeolocationCoordinates coords;
    uint64_t timestamp = 0;
};

/**
 * @brief Geolocation error
 */
struct GeolocationError {
    enum Code { PermissionDenied = 1, PositionUnavailable = 2, Timeout = 3 };
    Code code;
    std::string message;
};

/**
 * @brief Geolocation options
 */
struct GeolocationOptions {
    bool enableHighAccuracy = false;
    unsigned long timeout = 0xFFFFFFFF;
    unsigned long maximumAge = 0;
};

/**
 * @brief Geolocation API
 */
class Geolocation {
public:
    using SuccessCallback = std::function<void(const GeolocationPosition&)>;
    using ErrorCallback = std::function<void(const GeolocationError&)>;
    
    void getCurrentPosition(SuccessCallback success,
                            ErrorCallback error = nullptr,
                            const GeolocationOptions& options = {});
    
    int watchPosition(SuccessCallback success,
                      ErrorCallback error = nullptr,
                      const GeolocationOptions& options = {});
    
    void clearWatch(int watchId);
};

} // namespace Zepra::WebCore
