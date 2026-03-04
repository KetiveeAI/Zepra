/**
 * @file DynamicImport.cpp
 * @brief Dynamic import implementation
 */

#include "modules/DynamicImport.h"

namespace Zepra::Modules {

// =============================================================================
// DynamicImportHandler
// =============================================================================

DynamicImportHandler::DynamicImportHandler(ModuleLoader* loader, ModuleExecutor* executor)
    : loader_(loader), executor_(executor) {}

Runtime::Value DynamicImportHandler::handleImport(const std::string& specifier,
                                                   const std::string& referrer,
                                                   const ImportOptions& options) {
    // Validate attributes
    std::string attrError;
    if (!validateAttributes(options, attrError)) {
        // Return rejected promise
        auto promise = createImportPromise();
        rejectImportPromise(promise, "Invalid import attributes: " + attrError);
        return promise;
    }
    
    // Create promise for async loading
    auto promise = createImportPromise();
    
    // Start async loading
    std::thread([this, specifier, referrer, options, promise]() mutable {
        try {
            // Use host callback if provided
            ModuleRecord* module = nullptr;
            
            if (hostCallback_) {
                auto future = hostCallback_(specifier, referrer, options);
                module = future.get();
            } else {
                // Default: use loader
                std::string resolved = loader_->resolve(specifier, referrer);
                auto future = loader_->loadModule(resolved, referrer);
                module = future.get();
            }
            
            if (!module) {
                rejectImportPromise(promise, "Failed to load module: " + specifier);
                return;
            }
            
            if (module->hasError()) {
                rejectImportPromise(promise, module->error());
                return;
            }
            
            // Check import assertions
            if (!ImportAssertions::moduleMatchesAssertion(module, options)) {
                rejectImportPromise(promise, "Module type mismatch");
                return;
            }
            
            // Execute if needed
            auto& state = executor_->getState(module);
            if (state.phase != EvaluationState::Phase::Evaluated) {
                try {
                    executor_->execute(module);
                } catch (const std::exception& e) {
                    rejectImportPromise(promise, e.what());
                    return;
                }
            }
            
            // Check for evaluation error
            if (executor_->hasAnyError(module)) {
                auto error = executor_->getError(module);
                rejectImportPromise(promise, error.value_or("Evaluation error"));
                return;
            }
            
            // Resolve with namespace
            resolveImportPromise(promise, module);
            
        } catch (const std::exception& e) {
            rejectImportPromise(promise, e.what());
        }
    }).detach();
    
    return promise;
}

void DynamicImportHandler::handleImportAsync(const std::string& specifier,
                                              const std::string& referrer,
                                              std::function<void(Runtime::Value)> onSuccess,
                                              std::function<void(Runtime::Value)> onError) {
    std::thread([this, specifier, referrer, onSuccess, onError]() {
        try {
            std::string resolved = loader_->resolve(specifier, referrer);
            auto future = loader_->loadModule(resolved, referrer);
            auto* module = future.get();
            
            if (!module || module->hasError()) {
                onError(Runtime::Value::undefined());
                return;
            }
            
            executor_->execute(module);
            
            if (executor_->hasAnyError(module)) {
                onError(Runtime::Value::undefined());
                return;
            }
            
            auto ns = createNamespaceObject(module);
            onSuccess(ns);
            
        } catch (...) {
            onError(Runtime::Value::undefined());
        }
    }).detach();
}

bool DynamicImportHandler::validateAttributes(const ImportOptions& options, 
                                               std::string& error) {
    auto supported = supportedAttributes();
    
    for (const auto& [key, value] : options.with_) {
        if (std::find(supported.begin(), supported.end(), key) == supported.end()) {
            error = "Unsupported import attribute: " + key;
            return false;
        }
    }
    
    for (const auto& [key, value] : options.assert_) {
        if (std::find(supported.begin(), supported.end(), key) == supported.end()) {
            error = "Unsupported import assertion: " + key;
            return false;
        }
    }
    
    return true;
}

std::vector<std::string> DynamicImportHandler::supportedAttributes() {
    return {"type"};
}

Runtime::Value DynamicImportHandler::createNamespaceObject(ModuleRecord* module) {
    // Would create actual Module Namespace object
    return Runtime::Value::undefined();
}

Runtime::Value DynamicImportHandler::createImportPromise() {
    // Would integrate with Promise implementation
    return Runtime::Value::undefined();
}

void DynamicImportHandler::resolveImportPromise(Runtime::Value promise, ModuleRecord* module) {
    // Promise.resolve with namespace
}

void DynamicImportHandler::rejectImportPromise(Runtime::Value promise, const std::string& error) {
    // Promise.reject with TypeError
}

// =============================================================================
// ImportMetaObject
// =============================================================================

ImportMetaObject::ImportMetaObject(const std::string& moduleUrl)
    : url_(moduleUrl) {}

std::string ImportMetaObject::resolve(const std::string& specifier) const {
    if (specifier.empty()) return url_;
    
    // URL resolution logic
    if (specifier[0] == '/' || specifier.find("://") != std::string::npos) {
        return specifier;
    }
    
    if (specifier.substr(0, 2) == "./" || specifier.substr(0, 3) == "../") {
        size_t lastSlash = url_.rfind('/');
        if (lastSlash != std::string::npos) {
            return url_.substr(0, lastSlash + 1) + specifier;
        }
    }
    
    return specifier;
}

void ImportMetaObject::setProperty(const std::string& key, Runtime::Value value) {
    properties_[key] = value;
}

Runtime::Value ImportMetaObject::getProperty(const std::string& key) const {
    auto it = properties_.find(key);
    return it != properties_.end() ? it->second : Runtime::Value::undefined();
}

bool ImportMetaObject::hasProperty(const std::string& key) const {
    return properties_.count(key) > 0;
}

Runtime::Value ImportMetaObject::toValue() const {
    // Would create actual JS object
    return Runtime::Value::undefined();
}

// =============================================================================
// Import Assertions
// =============================================================================

namespace ImportAssertions {

bool validateTypeAssertion(const std::string& type,
                           const std::string& mimeType,
                           std::string& error) {
    std::string expected = expectedMimeType(type);
    
    if (expected.empty()) {
        error = "Unknown type assertion: " + type;
        return false;
    }
    
    // Strip parameters from MIME type
    std::string actualType = mimeType;
    size_t semicolon = actualType.find(';');
    if (semicolon != std::string::npos) {
        actualType = actualType.substr(0, semicolon);
    }
    
    if (actualType != expected) {
        error = "MIME type mismatch: expected " + expected + ", got " + actualType;
        return false;
    }
    
    return true;
}

std::string expectedMimeType(const std::string& typeAssertion) {
    if (typeAssertion == "json") return "application/json";
    if (typeAssertion == "css") return "text/css";
    if (typeAssertion == "javascript") return "text/javascript";
    return "";
}

bool moduleMatchesAssertion(ModuleRecord* module, const ImportOptions& options) {
    if (!options.hasAttribute("type")) {
        return true; // No assertion, always matches
    }
    
    // Would check actual module type against assertion
    return true;
}

} // namespace ImportAssertions

} // namespace Zepra::Modules
