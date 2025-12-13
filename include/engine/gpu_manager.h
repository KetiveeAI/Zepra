#pragma once
#include <string>

namespace zepra {

class GpuManager {
public:
    GpuManager();
    ~GpuManager();

    bool isGpuAvailable() const;
    bool enableGpuAcceleration(bool enable = true);
    bool isGpuAccelerationEnabled() const;
    std::string getGpuInfo() const;

    // Video decode support
    bool isVideoDecodeSupported() const;
};

} // namespace zepra 