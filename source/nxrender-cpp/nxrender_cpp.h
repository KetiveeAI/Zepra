/**
 * @file nxrender.h
 * @brief NXRender - C++ Rendering Framework for ZepraBrowser
 * 
 * Main header that includes all NXRender components.
 * Based on Firefox Gecko rendering model but no code tacken from there files.
 * 
 * @copyright 2024 KetiveeAI
 */

#pragma once

#include <functional>

// Core types
#include "nxgfx/color.h"
#include "nxgfx/primitives.h"
#include "nxgfx/context.h"
#include "nxgfx/text.h"

// Compositor
#include "core/surface.h"
#include "core/layer.h"
#include "core/compositor.h"

// Widgets
#include "widgets/widget.h"
#include "widgets/button.h"
#include "widgets/label.h"
#include "widgets/textfield.h"
#include "widgets/container.h"

// Layout
#include "layout/constraints.h"
#include "layout/layout.h"
#include "layout/flexbox.h"

// Theme
#include "theme/colors.h"
#include "theme/theme.h"

// Input
#include "input/events.h"
#include "input/mouse.h"
#include "input/keyboard.h"

namespace NXRender {

/**
 * @brief Initialize NXRender framework
 * @param width Window width
 * @param height Window height
 * @return true if initialization successful
 */
bool init(int width, int height);

/**
 * @brief Shutdown NXRender and release resources
 */
void shutdown();

/**
 * @brief Get the main compositor
 */
Compositor* compositor();

/**
 * @brief Get the current theme
 */
Theme* theme();

/**
 * @brief Event callback type
 */
using EventCallback = std::function<void(const Event&)>;

/**
 * @brief Set the main event handler
 */
void setEventHandler(EventCallback handler);

/**
 * @brief Render callback type
 */
using RenderCallback = std::function<void()>;

/**
 * @brief Set the main render callback
 */
void setRenderCallback(RenderCallback callback);

/**
 * @brief Process pending events
 */
void processEvents();

/**
 * @brief Render one frame
 */
void render();

/**
 * @brief Run the main loop (blocking)
 */
void run();

} // namespace NXRender
