// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once
#include <string>
#include <functional>

namespace zepra {

enum class VideoBackend {
    CPU,
    OpenGL,
    CUDA
};

class VideoPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();

    // Load and play video
    bool load(const std::string& filePath);
    bool play();
    bool pause();
    bool stop();
    bool seek(double seconds);
    double getDuration() const;
    double getCurrentTime() const;
    bool isPlaying() const;

    // GPU acceleration
    bool enableGpuAcceleration(bool enable = true);
    bool isGpuAccelerationEnabled() const;
    std::string getGpuInfo() const;

    // CUDA/NVDEC support
    bool isCudaAvailable() const;
    bool enableCuda(bool enable = true);
    bool isCudaEnabled() const;
    VideoBackend getActiveBackend() const;

    // Video events
    void setOnFrameCallback(std::function<void()> cb);
    void setOnEndCallback(std::function<void()> cb);

private:
    bool gpuEnabled;
    bool cudaEnabled;
    VideoBackend backend;
    std::function<void()> onFrameCallback;
    std::function<void()> onEndCallback;
    // ... internal state ...
};

} // namespace zepra 