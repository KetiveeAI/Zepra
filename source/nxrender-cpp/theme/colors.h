// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file colors.h
 * @brief Predefined color constants
 */

#pragma once

#include "../nxgfx/color.h"

namespace NXRender {
namespace Colors {

// Material Design colors
constexpr Color Red50(0xFFEBEE);
constexpr Color Red100(0xFFCDD2);
constexpr Color Red500(0xF44336);
constexpr Color Red900(0xB71C1C);

constexpr Color Blue50(0xE3F2FD);
constexpr Color Blue100(0xBBDEFB);
constexpr Color Blue500(0x2196F3);
constexpr Color Blue700(0x1976D2);
constexpr Color Blue900(0x0D47A1);

constexpr Color Green50(0xE8F5E9);
constexpr Color Green500(0x4CAF50);
constexpr Color Green700(0x388E3C);

constexpr Color Grey50(0xFAFAFA);
constexpr Color Grey100(0xF5F5F5);
constexpr Color Grey200(0xEEEEEE);
constexpr Color Grey300(0xE0E0E0);
constexpr Color Grey400(0xBDBDBD);
constexpr Color Grey500(0x9E9E9E);
constexpr Color Grey600(0x757575);
constexpr Color Grey700(0x616161);
constexpr Color Grey800(0x424242);
constexpr Color Grey900(0x212121);

// Common web colors
constexpr Color White(255, 255, 255);
constexpr Color Black(0, 0, 0);
constexpr Color Transparent(0, 0, 0, 0);

// System colors
constexpr Color LinkBlue(0x0066CC);
constexpr Color ErrorRed(0xD32F2F);
constexpr Color SuccessGreen(0x388E3C);
constexpr Color WarningOrange(0xF57C00);
constexpr Color InfoBlue(0x1976D2);

} // namespace Colors
} // namespace NXRender
