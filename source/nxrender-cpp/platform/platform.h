// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file platform.h
 * @brief Platform abstraction interface for NXRender
 */

#pragma once

#include <string>

namespace NXRender {

class Platform {
public:
    virtual ~Platform() = default;

    /**
     * @brief Initialize platform window and context
     * @return true if successful
     */
    virtual bool init(int width, int height, const std::string& title) = 0;

    /**
     * @brief Cleanup platform resources
     */
    virtual void shutdown() = 0;

    /**
     * @brief Polls for pending events from the OS
     * This will dispatch events to the internal event system
     */
    virtual void pollEvents() = 0;

    /**
     * @brief Swap buffers (if double buffered)
     */
    virtual void swapBuffers() = 0;
    
    /**
     * @brief Set window title
     */
    virtual void setTitle(const std::string& title) = 0;
};

// Factory function implemented by backend
Platform* createPlatform();

} // namespace NXRender
