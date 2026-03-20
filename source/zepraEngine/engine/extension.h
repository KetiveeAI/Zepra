// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once
#include <string>

namespace ZepraEngine {

class IExtension {
public:
    virtual ~IExtension() = default;

    // Called when the extension is loaded
    virtual void onLoad() = 0;

    // Called when the extension is enabled by the user
    virtual void onEnable() = 0;

    // Called when the extension is disabled by the user
    virtual void onDisable() = 0;

    // Called when the extension is unloaded (browser shutdown or removal)
    virtual void onUnload() = 0;

    // Extension metadata
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::string getAuthor() const = 0;
};

// Factory function signature for dynamic loading
extern "C" IExtension* createExtension();

} // namespace ZepraEngine 