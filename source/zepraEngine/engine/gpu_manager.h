// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
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