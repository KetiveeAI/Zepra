// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once
#include <string>
#include <vector>
#include <memory>

namespace ZepraUI {

struct ExtensionInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string iconPath;
    bool enabled;
};

class ExtensionManagerUI {
public:
    ExtensionManagerUI();
    ~ExtensionManagerUI();

    // Render the extension manager panel
    void render();

    // Set the list of installed extensions
    void setExtensions(const std::vector<ExtensionInfo>& extensions);

    // Callbacks for user actions
    void onEnableExtension(const std::string& extensionName);
    void onDisableExtension(const std::string& extensionName);
    void onRemoveExtension(const std::string& extensionName);
    void onOpenMarketplace();

private:
    std::vector<ExtensionInfo> m_extensions;
};

} // namespace ZepraUI 