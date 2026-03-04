/**
 * @file DynamicImport.h
 * @brief Dynamic import() implementation
 * 
 * Implements:
 * - import() expression handling
 * - Specifier resolution
 * - Promise integration
 * - Module cache interaction
 */

#pragma once

#include "../modules/ModuleLoader.h"
#include "../modules/ModuleExecutor.h"
#include "runtime/async/PromiseAPI.h"
#include <functional>

namespace Zepra::Modules {

// =============================================================================
// Dynamic Import Options
// =============================================================================

struct ImportOptions {
    // Import assertions (deprecated, but supported)
    std::unordered_map<std::string, std::string> assert_;
    
    // Import attributes (current spec)
    std::unordered_map<std::string, std::string> with_;
    
    bool hasAttribute(const std::string& key) const {
        return with_.count(key) || assert_.count(key);
    }
    
    std::string getAttribute(const std::string& key) const {
        auto it = with_.find(key);
        if (it != with_.end()) return it->second;
        it = assert_.find(key);
        if (it != assert_.end()) return it->second;
        return "";
    }
};

// =============================================================================
// Dynamic Import Handler
// =============================================================================

class DynamicImportHandler {
public:
    using HostImportCallback = std::function<
        std::future<ModuleRecord*>(const std::string& specifier,
                                    const std::string& referrer,
                                    const ImportOptions& options)>;
    
    DynamicImportHandler(ModuleLoader* loader, ModuleExecutor* executor);
    
    // =========================================================================
    // Import Handling
    // =========================================================================
    
    /**
     * @brief Handle import() call
     * @return Promise that resolves to module namespace
     */
    Runtime::Value handleImport(const std::string& specifier,
                                 const std::string& referrer,
                                 const ImportOptions& options = {});
    
    /**
     * @brief Handle import() with callback
     */
    void handleImportAsync(const std::string& specifier,
                           const std::string& referrer,
                           std::function<void(Runtime::Value)> onSuccess,
                           std::function<void(Runtime::Value)> onError);
    
    // =========================================================================
    // Host Integration
    // =========================================================================
    
    /**
     * @brief Set custom host import callback
     */
    void setHostImportCallback(HostImportCallback callback) {
        hostCallback_ = std::move(callback);
    }
    
    // =========================================================================
    // Import Attributes
    // =========================================================================
    
    /**
     * @brief Validate import attributes
     */
    bool validateAttributes(const ImportOptions& options, std::string& error);
    
    /**
     * @brief Get supported attribute keys
     */
    static std::vector<std::string> supportedAttributes();
    
private:
    ModuleLoader* loader_;
    ModuleExecutor* executor_;
    HostImportCallback hostCallback_;
    
    Runtime::Value createNamespaceObject(ModuleRecord* module);
    Runtime::Value createImportPromise();
    void resolveImportPromise(Runtime::Value promise, ModuleRecord* module);
    void rejectImportPromise(Runtime::Value promise, const std::string& error);
};

// =============================================================================
// Import Meta Object
// =============================================================================

class ImportMetaObject {
public:
    explicit ImportMetaObject(const std::string& moduleUrl);
    
    // Standard properties
    std::string url() const { return url_; }
    
    // import.meta.resolve(specifier)
    std::string resolve(const std::string& specifier) const;
    
    // Host-defined properties
    void setProperty(const std::string& key, Runtime::Value value);
    Runtime::Value getProperty(const std::string& key) const;
    bool hasProperty(const std::string& key) const;
    
    // Get as JS object
    Runtime::Value toValue() const;
    
private:
    std::string url_;
    std::unordered_map<std::string, Runtime::Value> properties_;
};

// =============================================================================
// Import Assertion Validator
// =============================================================================

namespace ImportAssertions {

/**
 * @brief Validate "type" assertion
 */
bool validateTypeAssertion(const std::string& type, 
                           const std::string& mimeType,
                           std::string& error);

/**
 * @brief Get expected MIME type for assertion
 */
std::string expectedMimeType(const std::string& typeAssertion);

/**
 * @brief Check if module type matches assertion
 */
bool moduleMatchesAssertion(ModuleRecord* module, const ImportOptions& options);

} // namespace ImportAssertions

} // namespace Zepra::Modules
