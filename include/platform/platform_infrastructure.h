#pragma once

/**
 * @file platform_infrastructure.h
 * @brief Platform infrastructure abstraction layer
 */

#include <string>

namespace Zepra::Platform {

class PlatformInfrastructure {
public:
    PlatformInfrastructure() = default;
    ~PlatformInfrastructure() = default;
    
    // Platform detection
    static bool isLinux() {
#ifdef __linux__
        return true;
#else
        return false;
#endif
    }
    
    static bool isWindows() {
#ifdef _WIN32
        return true;
#else
        return false;
#endif
    }
    
    static bool isMac() {
#ifdef __APPLE__
        return true;
#else
        return false;
#endif
    }
    
    // Basic system info
    static std::string getPlatformName() {
#ifdef __linux__
        return "Linux";
#elif defined(_WIN32)
        return "Windows";
#elif defined(__APPLE__)
        return "macOS";
#else
        return "Unknown";
#endif
    }
};

} // namespace Zepra::Platform
