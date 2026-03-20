// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "ui/extension_manager_ui.h"
#include <iostream>

namespace ZepraUI {

ExtensionManagerUI::ExtensionManagerUI() {}
ExtensionManagerUI::~ExtensionManagerUI() {}

void ExtensionManagerUI::render() {
    std::cout << "[Extension Manager UI] Rendering panel...\n";
    for (const auto& ext : m_extensions) {
        std::cout << " - " << ext.name << " (" << ext.version << ") by " << ext.author
                  << (ext.enabled ? " [Enabled]" : " [Disabled]") << std::endl;
    }
    std::cout << "[Extension Manager UI] End of list.\n";
}

void ExtensionManagerUI::setExtensions(const std::vector<ExtensionInfo>& extensions) {
    m_extensions = extensions;
}

void ExtensionManagerUI::onEnableExtension(const std::string& extensionName) {
    std::cout << "[Extension Manager UI] Enable: " << extensionName << std::endl;
}

void ExtensionManagerUI::onDisableExtension(const std::string& extensionName) {
    std::cout << "[Extension Manager UI] Disable: " << extensionName << std::endl;
}

void ExtensionManagerUI::onRemoveExtension(const std::string& extensionName) {
    std::cout << "[Extension Manager UI] Remove: " << extensionName << std::endl;
}

void ExtensionManagerUI::onOpenMarketplace() {
    std::cout << "[Extension Manager UI] Open Marketplace" << std::endl;
}

} // namespace ZepraUI 