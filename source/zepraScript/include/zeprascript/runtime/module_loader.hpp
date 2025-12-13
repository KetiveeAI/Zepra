#pragma once

/**
 * @file module_loader.hpp
 * @brief ES Module loader with file system access
 */

#include "../config.hpp"
#include "../runtime/value.hpp"
#include "../runtime/object.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

namespace Zepra::Runtime {

class VM;

/**
 * @brief Module record storing exports and metadata
 */
struct Module {
    std::string path;                          // Resolved absolute path
    Object* exports = nullptr;                 // Module exports object
    bool evaluated = false;                    // Whether module has been executed
    std::vector<std::string> dependencies;     // Import dependencies
};

/**
 * @brief Module loader with caching and resolution
 */
class ModuleLoader {
public:
    explicit ModuleLoader(VM* vm);
    
    /**
     * @brief Load a module and return its exports
     * @param specifier Module path (relative or bare)
     * @param referrer Path of the importing module
     * @return Exports object
     */
    Value loadModule(const std::string& specifier, const std::string& referrer = "");
    
    /**
     * @brief Set the base directory for module resolution
     */
    void setBaseDirectory(const std::string& dir) { baseDirectory_ = dir; }
    
    /**
     * @brief Register a built-in module
     */
    void registerBuiltinModule(const std::string& name, Object* exports);
    
    /**
     * @brief Clear module cache (for testing)
     */
    void clearCache() { cache_.clear(); }
    
private:
    /**
     * @brief Resolve module specifier to absolute path
     */
    std::string resolvePath(const std::string& specifier, const std::string& referrer);
    
    /**
     * @brief Load and parse module source file
     */
    std::string loadSource(const std::string& path);
    
    /**
     * @brief Compile and execute module, returning exports
     */
    Object* evaluateModule(const std::string& path, const std::string& source);
    
    VM* vm_;
    std::string baseDirectory_;
    std::unordered_map<std::string, Module> cache_;
    std::unordered_map<std::string, Object*> builtins_;
};

} // namespace Zepra::Runtime
