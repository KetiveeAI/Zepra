// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
#pragma once

#include "common/types.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <fstream>
#include <sstream>
#include <regex>

namespace zepra {

// Configuration Phase Enum
enum class ConfigPhase {
    BOOTLOADER,     // Early boot - NCF files only
    KERNEL,         // Kernel initialization
    SYSTEM,         // System services
    USER,           // User applications
    RUNTIME         // Runtime configuration
};

// Configuration File Types
enum class ConfigType {
    NCF,            // Network Configuration Format (INI-style)
    TIE             // Tied Installation Environment (JSON-based)
};

// NCF Parser (INI-style for bootloader)
class NCFParser {
public:
    NCFParser();
    ~NCFParser();
    
    // Parse NCF file
    bool parseFile(const String& filePath);
    bool parseString(const String& content);
    
    // Get values
    String getValue(const String& section, const String& key, const String& defaultValue = "") const;
    int getIntValue(const String& section, const String& key, int defaultValue = 0) const;
    bool getBoolValue(const String& section, const String& key, bool defaultValue = false) const;
    double getDoubleValue(const String& section, const String& key, double defaultValue = 0.0) const;
    
    // Set values
    void setValue(const String& section, const String& key, const String& value);
    void setIntValue(const String& section, const String& key, int value);
    void setBoolValue(const String& section, const String& key, bool value);
    void setDoubleValue(const String& section, const String& key, double value);
    
    // Section management
    std::vector<String> getSections() const;
    std::vector<String> getKeys(const String& section) const;
    bool hasSection(const String& section) const;
    bool hasKey(const String& section, const String& key) const;
    
    // File operations
    bool saveToFile(const String& filePath) const;
    String toString() const;
    
    // Validation
    bool validate() const;
    std::vector<String> getErrors() const;
    
private:
    struct Section {
        String name;
        std::unordered_map<String, String> keyValuePairs;
        std::vector<String> comments;
    };
    
    std::unordered_map<String, Section> sections;
    std::vector<String> errors;
    std::vector<String> comments;
    
    void parseLine(const String& line);
    void addError(const String& error);
    String escapeValue(const String& value) const;
    String unescapeValue(const String& value) const;
};

// TIE Parser (JSON-based for rich configurations)
class TIEParser {
public:
    TIEParser();
    ~TIEParser();
    
    // Parse TIE file
    bool parseFile(const String& filePath);
    bool parseString(const String& content);
    
    // Get values with JSON path support
    String getValue(const String& path, const String& defaultValue = "") const;
    int getIntValue(const String& path, int defaultValue = 0) const;
    bool getBoolValue(const String& path, bool defaultValue = false) const;
    double getDoubleValue(const String& path, double defaultValue = 0.0) const;
    std::vector<String> getArrayValue(const String& path) const;
    std::unordered_map<String, String> getObjectValue(const String& path) const;
    
    // Set values
    void setValue(const String& path, const String& value);
    void setIntValue(const String& path, int value);
    void setBoolValue(const String& path, bool value);
    void setDoubleValue(const String& path, double value);
    void setArrayValue(const String& path, const std::vector<String>& value);
    void setObjectValue(const String& path, const std::unordered_map<String, String>& value);
    
    // Advanced operations
    bool hasPath(const String& path) const;
    void removePath(const String& path);
    std::vector<String> getPaths() const;
    
    // Schema validation
    bool validateSchema(const String& schemaPath) const;
    bool validateSchema(const std::unordered_map<String, String>& schema) const;
    
    // File operations
    bool saveToFile(const String& filePath) const;
    String toString() const;
    
    // Installation recipes
    struct InstallationRecipe {
        String name;
        String version;
        String description;
        std::vector<String> dependencies;
        std::vector<String> files;
        std::unordered_map<String, String> environment;
        std::vector<String> commands;
        std::unordered_map<String, String> permissions;
        bool autoInstall;
        bool requiresRestart;
    };
    
    InstallationRecipe parseInstallationRecipe() const;
    bool validateInstallationRecipe(const InstallationRecipe& recipe) const;
    
    // Validation
    bool validate() const;
    std::vector<String> getErrors() const;
    
private:
    struct JSONNode {
        enum class Type {
            STRING, INTEGER, DOUBLE, BOOLEAN, ARRAY, OBJECT, NULL_VALUE
        };
        
        Type type;
        String stringValue;
        int intValue;
        double doubleValue;
        bool boolValue;
        std::vector<std::shared_ptr<JSONNode>> arrayValue;
        std::unordered_map<String, std::shared_ptr<JSONNode>> objectValue;
        
        JSONNode() : type(Type::NULL_VALUE), intValue(0), doubleValue(0.0), boolValue(false) {}
    };
    
    std::shared_ptr<JSONNode> rootNode;
    std::vector<String> errors;
    
    std::shared_ptr<JSONNode> parseJSONValue(std::istringstream& stream);
    std::shared_ptr<JSONNode> getNodeByPath(const String& path) const;
    void setNodeByPath(const String& path, std::shared_ptr<JSONNode> node);
    String nodeToString(const std::shared_ptr<JSONNode>& node, int indent = 0) const;
    void addError(const String& error);
    String escapeJSONString(const String& str) const;
    String unescapeJSONString(const String& str) const;
};

// Dual Configuration Manager
class DualConfigManager {
public:
    DualConfigManager();
    ~DualConfigManager();
    
    // Initialization
    bool initialize(ConfigPhase phase);
    void shutdown();
    bool isInitialized() const;
    
    // Configuration loading
    bool loadNCFConfig(const String& filePath);
    bool loadTIEConfig(const String& filePath);
    bool loadConfig(const String& filePath);
    
    // Configuration access
    String getValue(const String& path, const String& defaultValue = "") const;
    int getIntValue(const String& path, int defaultValue = 0) const;
    bool getBoolValue(const String& path, bool defaultValue = false) const;
    double getDoubleValue(const String& path, double defaultValue = 0.0) const;
    std::vector<String> getArrayValue(const String& path) const;
    std::unordered_map<String, String> getObjectValue(const String& path) const;
    
    // Configuration modification
    void setValue(const String& path, const String& value);
    void setIntValue(const String& path, int value);
    void setBoolValue(const String& path, bool value);
    void setDoubleValue(const String& path, double value);
    void setArrayValue(const String& path, const std::vector<String>& value);
    void setObjectValue(const String& path, const std::unordered_map<String, String>& value);
    
    // Configuration validation
    bool validateConfig() const;
    std::vector<String> getValidationErrors() const;
    
    // Configuration saving
    bool saveNCFConfig(const String& filePath) const;
    bool saveTIEConfig(const String& filePath) const;
    bool saveConfig(const String& filePath) const;
    
    // Phase management
    ConfigPhase getCurrentPhase() const;
    void setPhase(ConfigPhase phase);
    bool isPhaseSupported(ConfigPhase phase) const;
    
    // File type detection
    ConfigType detectFileType(const String& filePath) const;
    ConfigType detectContentType(const String& content) const;
    
    // Installation automation
    struct InstallationConfig {
        String appName;
        String version;
        String targetPath;
        std::vector<String> dependencies;
        std::unordered_map<String, String> environment;
        std::vector<String> postInstallCommands;
        bool autoStart;
        bool createShortcut;
        std::unordered_map<String, String> registryKeys;
    };
    
    InstallationConfig parseInstallationConfig(const String& configPath) const;
    bool executeInstallation(const InstallationConfig& config) const;
    
    // Security features
    void enableEncryption(bool enabled);
    bool isEncryptionEnabled() const;
    void setEncryptionKey(const String& key);
    String encryptValue(const String& value) const;
    String decryptValue(const String& encryptedValue) const;
    
    // Performance optimization
    void enableCaching(bool enabled);
    bool isCachingEnabled() const;
    void clearCache();
    void setCacheSize(size_t size);
    size_t getCacheSize() const;
    
    // Event system
    void setConfigChangeCallback(std::function<void(const String&, const String&)> callback);
    void setValidationCallback(std::function<bool(const String&, const String&)> callback);
    void setInstallationCallback(std::function<void(const InstallationConfig&)> callback);
    
    // Error handling
    void setErrorCallback(std::function<void(const String&)> callback);
    std::vector<String> getErrors() const;
    void clearErrors();
    
private:
    ConfigPhase currentPhase;
    std::unique_ptr<NCFParser> ncfParser;
    std::unique_ptr<TIEParser> tieParser;
    bool initialized;
    bool encryptionEnabled;
    bool cachingEnabled;
    String encryptionKey;
    size_t cacheSize;
    
    // Callbacks
    std::function<void(const String&, const String&)> configChangeCallback;
    std::function<bool(const String&, const String&)> validationCallback;
    std::function<void(const InstallationConfig&)> installationCallback;
    std::function<void(const String&)> errorCallback;
    
    // Cache
    std::unordered_map<String, String> valueCache;
    std::vector<String> errors;
    
    // Internal methods
    void notifyConfigChange(const String& path, const String& value);
    void addError(const String& error);
    bool validatePath(const String& path) const;
    String normalizePath(const String& path) const;
    bool isSecurePath(const String& path) const;
    void clearValueCache();
};

// Configuration Schemas
namespace ConfigSchemas {
    
    // Browser Engine Schema
    extern const std::unordered_map<String, String> BROWSER_ENGINE_SCHEMA;
    
    // Developer Tools Schema
    extern const std::unordered_map<String, String> DEVELOPER_TOOLS_SCHEMA;
    
    // Security Schema
    extern const std::unordered_map<String, String> SECURITY_SCHEMA;
    
    // Performance Schema
    extern const std::unordered_map<String, String> PERFORMANCE_SCHEMA;
    
    // Installation Schema
    extern const std::unordered_map<String, String> INSTALLATION_SCHEMA;
    
    // App Store Schema
    extern const std::unordered_map<String, String> APP_STORE_SCHEMA;
    
    // Video Platform Schema
    extern const std::unordered_map<String, String> VIDEO_PLATFORM_SCHEMA;
    
    // Cloud Platform Schema
    extern const std::unordered_map<String, String> CLOUD_PLATFORM_SCHEMA;
}

// Configuration Utilities
namespace ConfigUtils {
    
    // File operations
    bool fileExists(const String& filePath);
    bool createDirectory(const String& path);
    bool copyFile(const String& source, const String& destination);
    bool deleteFile(const String& filePath);
    String getFileExtension(const String& filePath);
    String getFileName(const String& filePath);
    String getDirectory(const String& filePath);
    
    // Path operations
    String normalizePath(const String& path);
    String joinPath(const String& path1, const String& path2);
    String getAbsolutePath(const String& relativePath);
    String getRelativePath(const String& absolutePath, const String& basePath);
    
    // Validation
    bool isValidPath(const String& path);
    bool isSecurePath(const String& path);
    bool isValidFileName(const String& fileName);
    bool isValidURL(const String& url);
    
    // Encryption
    String hashString(const String& input);
    String generateRandomKey(size_t length);
    bool verifyHash(const String& input, const String& hash);
    
    // Environment
    String getEnvironmentVariable(const String& name);
    void setEnvironmentVariable(const String& name, const String& value);
    std::unordered_map<String, String> getAllEnvironmentVariables();
    
    // System information
    String getOSName();
    String getOSVersion();
    String getArchitecture();
    String getHostname();
    String getUsername();
    String getHomeDirectory();
    String getTempDirectory();
    
    // Performance
    size_t getAvailableMemory();
    size_t getTotalMemory();
    double getCPUUsage();
    size_t getDiskSpace(const String& path);
    
    // Security
    bool isRunningAsAdmin();
    bool hasPermission(const String& path, const String& permission);
    String getSecurityToken();
    bool validateSecurityToken(const String& token);
}

} // namespace zepra 