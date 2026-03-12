// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once
#include "../../source/integration/include/common/types.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace zepra {

// Config file types
enum class ConfigFileType {
    NCF, // INI-style
    TIE  // JSON-style
};

// ConfigManager handles both .ncf and .tie files
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    // Load config file
    bool loadConfig(const std::string& path, ConfigFileType type);
    // Save config file
    bool saveConfig(const std::string& path, ConfigFileType type) const;
    // Get value
    std::string getValue(const std::string& section, const std::string& key) const;
    // Set value
    void setValue(const std::string& section, const std::string& key, const std::string& value);
    // Remove value
    void removeValue(const std::string& section, const std::string& key);
    // Validate config
    bool validateConfig(ConfigFileType type) const;
    // For .tie: get install recipe
    std::string getInstallRecipe() const;
    // For .tie: process install automation
    bool processInstallRecipe();

private:
    ConfigFileType currentType;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> ncfData;
    std::unordered_map<std::string, std::string> tieData;
    std::string installRecipe;
    // Internal helpers
    bool parseNCF(const std::string& content);
    bool parseTIE(const std::string& content);
    std::string serializeNCF() const;
    std::string serializeTIE() const;
};

} // namespace zepra 