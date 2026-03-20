// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file devtools_icons.h
 * @brief SVG-based icons for DevTools panel tabs
 * 
 * Icons from NeolyxOS icon library - compact inline SVG paths
 */

#ifndef DEVTOOLS_ICONS_H
#define DEVTOOLS_ICONS_H

namespace zepra {
namespace devtools_icons {

// Terminal/Console icon - for Console tab
constexpr const char* CONSOLE_SVG = R"(
<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<rect x="3" y="4" width="18" height="16" rx="2" stroke="currentColor" stroke-width="2"/>
<path d="M7 10L10 13L7 16" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
<path d="M13 16H17" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
</svg>
)";

// Network icon - for Network tab
constexpr const char* NETWORK_SVG = R"(
<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<circle cx="12" cy="12" r="3" stroke="currentColor" stroke-width="2"/>
<path d="M12 2V5M12 19V22M2 12H5M19 12H22M4.9 4.9L7 7M17 17L19.1 19.1M4.9 19.1L7 17M17 7L19.1 4.9" stroke="currentColor" stroke-width="2"/>
</svg>
)";

// Elements/Code icon - for Elements tab  
constexpr const char* ELEMENTS_SVG = R"(
<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M16 18L22 12L16 6M8 6L2 12L8 18" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)";

// Lock/Security icon - for Security tab
constexpr const char* SECURITY_SVG = R"(
<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<rect x="5" y="11" width="14" height="11" rx="2" stroke="currentColor" stroke-width="2"/>
<path d="M8 11V7C8 4.8 9.8 3 12 3C14.2 3 16 4.8 16 7V11" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
<circle cx="12" cy="16" r="1.5" fill="currentColor"/>
</svg>
)";

// Chart/Performance icon - for Performance tab
constexpr const char* PERFORMANCE_SVG = R"(
<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M3 3V18C3 19.7 4.3 21 6 21H21" stroke="currentColor" stroke-width="2"/>
<path d="M7 14L10 10L13 13L18 8" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)";

// Close button icon
constexpr const char* CLOSE_SVG = R"(
<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M15 9L9 15M9 9L15 15" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
</svg>
)";

// Filter icon - for network filtering
constexpr const char* FILTER_SVG = R"(
<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M22 3H2L10 12.5V19L14 21V12.5L22 3Z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)";

// Trash/Clear icon - for clear logs/requests  
constexpr const char* CLEAR_SVG = R"(
<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M3 6H21M19 6V20C19 21.1 18.1 22 17 22H7C5.9 22 5 21.1 5 20V6M8 6V4C8 2.9 8.9 2 10 2H14C15.1 2 16 2.9 16 4V6" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
<path d="M10 11V17M14 11V17" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
</svg>
)";

// Search icon - for search/filter input
constexpr const char* SEARCH_SVG = R"(
<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<circle cx="11" cy="11" r="7" stroke="currentColor" stroke-width="2"/>
<path d="M16.5 16.5L21 21" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"/>
</svg>
)";

// Warning/Alert icon - for security warnings
constexpr const char* WARNING_SVG = R"(
<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M10.3 3.9L2.1 18C1.9 18.3 1.8 18.7 1.8 19C1.8 20.1 2.7 21 3.8 21H20.2C20.5 21 20.9 20.9 21.2 20.7C22.1 20.2 22.4 19 21.9 18.1L13.7 3.9C13.5 3.6 13.2 3.3 12.9 3.2C12 2.6 10.8 2.9 10.3 3.9Z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)";

// Info icon - for info messages
constexpr const char* INFO_SVG = R"(
<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<circle cx="12" cy="12" r="10" stroke="currentColor" stroke-width="2"/>
<path d="M12 16V12M12 8H12.01" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
</svg>
)";

// Bug icon - for errors
constexpr const char* BUG_SVG = R"(
<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M12 2C10 2 8.5 3.5 8.5 5.5V7H15.5V5.5C15.5 3.5 14 2 12 2Z" stroke="currentColor" stroke-width="2"/>
<path d="M20 12H22M2 12H4M8 19V22M16 19V22" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
</svg>
)";

// Copy icon
constexpr const char* COPY_SVG = R"(
<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<rect x="9" y="9" width="13" height="13" rx="2" stroke="currentColor" stroke-width="2"/>
<path d="M5 15H4C2.9 15 2 14.1 2 13V4C2 2.9 2.9 2 4 2H13C14.1 2 15 2.9 15 4V5" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)";

// Download icon
constexpr const char* DOWNLOAD_SVG = R"(
<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M12 3V15M8 11L12 15L16 11" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
<path d="M3 16V18C3 19.7 4.3 21 6 21H18C19.7 21 21 19.7 21 18V16" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
</svg>
)";

} // namespace devtools_icons
} // namespace zepra

#endif // DEVTOOLS_ICONS_H
