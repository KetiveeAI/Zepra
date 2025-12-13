/**
 * @file module_loader.cpp
 * @brief ES Module loader implementation
 */

#include "zeprascript/runtime/module_loader.hpp"
#include "zeprascript/runtime/vm.hpp"
#include "zeprascript/frontend/parser.hpp"
#include "zeprascript/bytecode/bytecode_generator.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace Zepra::Runtime {

namespace fs = std::filesystem;

ModuleLoader::ModuleLoader(VM* vm) : vm_(vm) {
    // Default to current directory
    baseDirectory_ = fs::current_path().string();
}

Value ModuleLoader::loadModule(const std::string& specifier, const std::string& referrer) {
    // Check for builtin modules
    auto builtinIt = builtins_.find(specifier);
    if (builtinIt != builtins_.end()) {
        return Value::object(builtinIt->second);
    }
    
    // Resolve the module path
    std::string resolvedPath = resolvePath(specifier, referrer);
    
    // Check cache
    auto cacheIt = cache_.find(resolvedPath);
    if (cacheIt != cache_.end()) {
        if (cacheIt->second.evaluated) {
            return Value::object(cacheIt->second.exports);
        }
    }
    
    // Load the source file
    std::string source = loadSource(resolvedPath);
    if (source.empty()) {
        throw std::runtime_error("Cannot load module: " + resolvedPath);
    }
    
    // Compile and execute
    Object* exports = evaluateModule(resolvedPath, source);
    
    // Cache the result
    Module mod;
    mod.path = resolvedPath;
    mod.exports = exports;
    mod.evaluated = true;
    cache_[resolvedPath] = mod;
    
    return Value::object(exports);
}

void ModuleLoader::registerBuiltinModule(const std::string& name, Object* exports) {
    builtins_[name] = exports;
}

std::string ModuleLoader::resolvePath(const std::string& specifier, const std::string& referrer) {
    // Handle relative paths
    if (specifier.starts_with("./") || specifier.starts_with("../")) {
        fs::path base;
        if (!referrer.empty()) {
            base = fs::path(referrer).parent_path();
        } else {
            base = fs::path(baseDirectory_);
        }
        
        fs::path resolved = base / specifier;
        
        // Try with .js extension if not present
        if (!resolved.has_extension()) {
            if (fs::exists(resolved.string() + ".js")) {
                resolved = resolved.string() + ".js";
            }
        }
        
        return fs::canonical(resolved).string();
    }
    
    // Handle absolute paths
    if (specifier.starts_with("/")) {
        return specifier;
    }
    
    // Bare specifier - look in node_modules or base directory
    fs::path modulePath = fs::path(baseDirectory_) / specifier;
    if (fs::exists(modulePath)) {
        return fs::canonical(modulePath).string();
    }
    
    // Try with .js extension
    if (fs::exists(modulePath.string() + ".js")) {
        return fs::canonical(modulePath.string() + ".js").string();
    }
    
    // Return as-is if nothing found
    return specifier;
}

std::string ModuleLoader::loadSource(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

Object* ModuleLoader::evaluateModule(const std::string& path, const std::string& source) {
    // Create exports object
    Object* exports = new Object();
    
    // Parse the module source
    auto program = Frontend::parse(source, path);
    if (!program) {
        throw std::runtime_error("Failed to parse module: " + path);
    }
    
    // Compile to bytecode
    Bytecode::BytecodeGenerator generator;
    auto chunk = generator.compile(program.get());
    if (!chunk || generator.hasErrors()) {
        std::string errors;
        for (const auto& err : generator.errors()) {
            errors += err + "\n";
        }
        throw std::runtime_error("Failed to compile module: " + path + "\n" + errors);
    }
    
    // Save current VM state
    // Execute the module bytecode
    // The module's exports will be populated via OP_EXPORT
    
    // For now, run in the current VM context
    // This is simplified - full implementation needs separate module context
    vm_->setGlobal("exports", Value::object(exports));
    vm_->execute(chunk.get());
    
    return exports;
}

} // namespace Zepra::Runtime
