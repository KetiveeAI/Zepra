// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#include "../../source/platform/include/config/dual_config.h"
#include "config_manager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>

namespace zepra {

ConfigManager::ConfigManager() : currentType(ConfigFileType::NCF) {}

ConfigManager::~ConfigManager() {}

bool ConfigManager::loadConfig(const std::string& path, ConfigFileType type) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << path << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    currentType = type;
    
    if (type == ConfigFileType::NCF) {
        return parseNCF(content);
    } else if (type == ConfigFileType::TIE) {
        return parseTIE(content);
    }
    
    return false;
}

bool ConfigManager::saveConfig(const std::string& path, ConfigFileType type) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to create config file: " << path << std::endl;
        return false;
    }
    
    std::string content;
    if (type == ConfigFileType::NCF) {
        content = serializeNCF();
    } else if (type == ConfigFileType::TIE) {
        content = serializeTIE();
    }
    
    file << content;
    return true;
}

std::string ConfigManager::getValue(const std::string& section, const std::string& key) const {
    if (currentType == ConfigFileType::NCF) {
        auto sectionIt = ncfData.find(section);
        if (sectionIt != ncfData.end()) {
            auto keyIt = sectionIt->second.find(key);
            if (keyIt != sectionIt->second.end()) {
                return keyIt->second;
            }
        }
    } else if (currentType == ConfigFileType::TIE) {
        std::string fullKey = section + "." + key;
        auto it = tieData.find(fullKey);
        if (it != tieData.end()) {
            return it->second;
        }
    }
    return "";
}

void ConfigManager::setValue(const std::string& section, const std::string& key, const std::string& value) {
    if (currentType == ConfigFileType::NCF) {
        ncfData[section][key] = value;
    } else if (currentType == ConfigFileType::TIE) {
        std::string fullKey = section + "." + key;
        tieData[fullKey] = value;
    }
}

void ConfigManager::removeValue(const std::string& section, const std::string& key) {
    if (currentType == ConfigFileType::NCF) {
        auto sectionIt = ncfData.find(section);
        if (sectionIt != ncfData.end()) {
            sectionIt->second.erase(key);
            if (sectionIt->second.empty()) {
                ncfData.erase(sectionIt);
            }
        }
    } else if (currentType == ConfigFileType::TIE) {
        std::string fullKey = section + "." + key;
        tieData.erase(fullKey);
    }
}

bool ConfigManager::validateConfig(ConfigFileType type) const {
    if (type == ConfigFileType::NCF) {
        // Validate NCF structure - check for required sections
        return !ncfData.empty();
    } else if (type == ConfigFileType::TIE) {
        // Validate TIE structure - check for required fields
        return !tieData.empty();
    }
    return false;
}

std::string ConfigManager::getInstallRecipe() const {
    return installRecipe;
}

bool ConfigManager::processInstallRecipe() {
    if (installRecipe.empty()) {
        return false;
    }
    
    // Parse and execute installation recipe
    std::istringstream iss(installRecipe);
    std::string line;
    
    while (std::getline(iss, line)) {
        // Remove comments
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) continue;
        
        // Process installation commands
        if (line.substr(0, 8) == "install ") {
            std::string package = line.substr(8);
            std::cout << "Installing package: " << package << std::endl;
            // TODO: Implement actual package installation
        } else if (line.substr(0, 7) == "mkdir ") {
            std::string dir = line.substr(7);
            std::cout << "Creating directory: " << dir << std::endl;
            // TODO: Implement directory creation
        } else if (line.substr(0, 5) == "copy ") {
            size_t spacePos = line.find(' ', 5);
            if (spacePos != std::string::npos) {
                std::string src = line.substr(5, spacePos - 5);
                std::string dst = line.substr(spacePos + 1);
                std::cout << "Copying " << src << " to " << dst << std::endl;
                // TODO: Implement file copying
            }
        }
    }
    
    return true;
}

bool ConfigManager::parseNCF(const std::string& content) {
    ncfData.clear();
    
    std::istringstream iss(content);
    std::string line;
    std::string currentSection = "";
    
    while (std::getline(iss, line)) {
        // Remove comments
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) continue;
        
        // Check for section header [section]
        if (line[0] == '[' && line[line.length() - 1] == ']') {
            currentSection = line.substr(1, line.length() - 2);
            ncfData[currentSection] = std::unordered_map<std::string, std::string>();
        }
        // Check for key=value pair
        else if (!currentSection.empty() && line.find('=') != std::string::npos) {
            size_t equalPos = line.find('=');
            std::string key = line.substr(0, equalPos);
            std::string value = line.substr(equalPos + 1);
            
            // Trim key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Remove quotes if present
            if (value.length() >= 2 && value[0] == '"' && value[value.length() - 1] == '"') {
                value = value.substr(1, value.length() - 2);
            }
            
            ncfData[currentSection][key] = value;
        }
    }
    
    return true;
}

bool ConfigManager::parseTIE(const std::string& content) {
    tieData.clear();
    
    // Simple JSON-like parser for TIE files
    std::istringstream iss(content);
    std::string line;
    
    while (std::getline(iss, line)) {
        // Remove comments
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) continue;
        
        // Parse JSON-like key-value pairs
        if (line.find(':') != std::string::npos) {
            size_t colonPos = line.find(':');
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Remove quotes and commas
            if (key.length() >= 2 && key[0] == '"' && key[key.length() - 1] == '"') {
                key = key.substr(1, key.length() - 2);
            }
            if (value.length() >= 2 && value[0] == '"' && value[value.length() - 1] == '"') {
                value = value.substr(1, value.length() - 2);
            }
            if (!value.empty() && value[value.length() - 1] == ',') {
                value = value.substr(0, value.length() - 1);
            }
            
            tieData[key] = value;
            
            // Check for install recipe
            if (key == "install_recipe") {
                installRecipe = value;
            }
        }
    }
    
    return true;
}

std::string ConfigManager::serializeNCF() const {
    std::stringstream ss;
    
    for (const auto& section : ncfData) {
        ss << "[" << section.first << "]" << std::endl;
        for (const auto& keyValue : section.second) {
            ss << keyValue.first << " = " << keyValue.second << std::endl;
        }
        ss << std::endl;
    }
    
    return ss.str();
}

std::string ConfigManager::serializeTIE() const {
    std::stringstream ss;
    
    ss << "{" << std::endl;
    bool first = true;
    for (const auto& keyValue : tieData) {
        if (!first) {
            ss << "," << std::endl;
        }
        ss << "  \"" << keyValue.first << "\": \"" << keyValue.second << "\"";
        first = false;
    }
    ss << std::endl << "}" << std::endl;
    
    return ss.str();
}

} // namespace zepra 