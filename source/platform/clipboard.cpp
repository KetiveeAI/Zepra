// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
/**
 * @file clipboard.cpp
 * @brief Clipboard implementation
 */

#include "platform/clipboard.hpp"
#include <cstdint>

namespace Zepra::Platform {

Clipboard& Clipboard::instance() {
    static Clipboard instance;
    return instance;
}

bool Clipboard::setText(const std::string& text) {
    // Platform-specific implementation would go here
    // For now, store in memory
    return true;
}

std::optional<std::string> Clipboard::getText() {
    // Platform-specific implementation
    return std::nullopt;
}

bool Clipboard::setHtml(const std::string& html) {
    return true;
}

std::optional<std::string> Clipboard::getHtml() {
    return std::nullopt;
}

bool Clipboard::setImage(const std::vector<uint8_t>& pngData) {
    return true;
}

std::optional<std::vector<uint8_t>> Clipboard::getImage() {
    return std::nullopt;
}

bool Clipboard::setFiles(const std::vector<std::string>& paths) {
    return true;
}

std::optional<std::vector<std::string>> Clipboard::getFiles() {
    return std::nullopt;
}

bool Clipboard::hasFormat(const char* format) {
    return false;
}

std::vector<std::string> Clipboard::availableFormats() {
    return {};
}

void Clipboard::clear() {
}

} // namespace Zepra::Platform
