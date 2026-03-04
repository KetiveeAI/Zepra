/**
 * @file settings_store.cpp  
 * @brief Settings implementation
 */

#include "storage/settings_store.h"
#include <iostream>
#include <fstream>

namespace ZepraBrowser {

SettingsStore& SettingsStore::instance() {
    static SettingsStore instance;
    return instance;
}

SettingsStore::SettingsStore() {
    std::cout << "[SettingsStore] Initialized with defaults" << std::endl;
}

SettingsStore::~SettingsStore() = default;

Settings SettingsStore::getSettings() const {
    return settings_;
}

void SettingsStore::setSettings(const Settings& settings) {
    settings_ = settings;
}

SearchEngine SettingsStore::getSearchEngine() const {
    return settings_.defaultSearchEngine;
}

void SettingsStore::setSearchEngine(SearchEngine engine) {
    settings_.defaultSearchEngine = engine;
    std::cout << "[SettingsStore] Search engine changed: " << (int)engine << std::endl;
}

bool SettingsStore::isDarkMode() const {
    return settings_.darkMode;
}

void SettingsStore::setDarkMode(bool enabled) {
    settings_.darkMode = enabled;
    std::cout << "[SettingsStore] Dark mode: " << (enabled ? "ON" : "OFF") << std::endl;
}

bool SettingsStore::load(const std::string& filepath) {
    filepath_ = filepath;
    // TODO: JSON loading
    std::cout << "[SettingsStore] Load from: " << filepath << std::endl;
    return true;
}

bool SettingsStore::save(const std::string& filepath) {
    filepath_ = filepath;
    // TODO: JSON saving
    std::cout << "[SettingsStore] Save to: " << filepath << std::endl;
    return true;
}

} // namespace ZepraBrowser
