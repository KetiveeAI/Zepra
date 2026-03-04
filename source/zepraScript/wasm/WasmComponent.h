/**
 * @file WasmComponent.h
 * @brief WebAssembly Component Model Support
 * 
 * Implements the WASM Component Model proposal:
 * - Component types and instances
 * - WIT (WebAssembly Interface Types)
 * - Resource handles
 * - Component linking
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include <optional>

namespace Zepra::Wasm::Component {

// =============================================================================
// WIT Value Types (Interface Types)
// =============================================================================

enum class WitValType : uint8_t {
    Bool,
    S8, S16, S32, S64,
    U8, U16, U32, U64,
    F32, F64,
    Char,
    String,
    List,
    Record,
    Tuple,
    Variant,
    Enum,
    Option,
    Result,
    Flags,
    Own,      // Owned resource handle
    Borrow    // Borrowed resource handle
};

/**
 * @brief WIT type definition
 */
struct WitType {
    WitValType kind;
    std::vector<WitType> params;  // For List, Tuple, etc.
    std::string name;             // For named types
    
    static WitType primitive(WitValType t) { return {t, {}, ""}; }
    static WitType list(WitType elem) { return {WitValType::List, {elem}, ""}; }
    static WitType option(WitType inner) { return {WitValType::Option, {inner}, ""}; }
};

// =============================================================================
// Resource Types
// =============================================================================

/**
 * @brief Resource handle (own or borrow)
 */
struct ResourceHandle {
    uint32_t rep;       // Resource representation index
    uint32_t typeIdx;   // Resource type index
    bool isOwn;         // true = own, false = borrow
    
    static ResourceHandle own(uint32_t rep, uint32_t type) {
        return {rep, type, true};
    }
    static ResourceHandle borrow(uint32_t rep, uint32_t type) {
        return {rep, type, false};
    }
};

/**
 * @brief Resource type definition
 */
struct ResourceType {
    std::string name;
    uint32_t index;
    
    // Destructor function index (if has destructor)
    std::optional<uint32_t> dropFunc;
};

// =============================================================================
// Function Types
// =============================================================================

/**
 * @brief WIT function signature
 */
struct WitFuncType {
    std::vector<std::pair<std::string, WitType>> params;
    std::vector<std::pair<std::string, WitType>> results;
    
    bool isMethod = false;      // Method on a resource
    uint32_t resourceIdx = 0;   // Resource this method belongs to
};

// =============================================================================
// Interface Types
// =============================================================================

/**
 * @brief WIT interface definition
 */
struct WitInterface {
    std::string name;
    std::string packageName;
    
    std::vector<ResourceType> resources;
    std::vector<std::pair<std::string, WitType>> types;
    std::vector<std::pair<std::string, WitFuncType>> functions;
    
    std::string fullyQualifiedName() const {
        return packageName.empty() ? name : packageName + "/" + name;
    }
};

// =============================================================================
// Component Types
// =============================================================================

/**
 * @brief Import descriptor
 */
struct ComponentImport {
    std::string name;
    enum class Kind { Func, Type, Instance, Component } kind;
    uint32_t typeIdx;
};

/**
 * @brief Export descriptor
 */
struct ComponentExport {
    std::string name;
    enum class Kind { Func, Type, Instance, Component } kind;
    uint32_t index;
};

/**
 * @brief Component type definition
 */
struct ComponentType {
    std::vector<ComponentImport> imports;
    std::vector<ComponentExport> exports;
    std::vector<WitInterface> interfaces;
};

// =============================================================================
// Component Instance
// =============================================================================

/**
 * @brief Instantiated component
 */
class ComponentInstance {
public:
    ComponentInstance(const ComponentType* type) : type_(type) {}
    
    // Get exported function
    void* getExport(const std::string& name) const {
        auto it = exports_.find(name);
        return it != exports_.end() ? it->second : nullptr;
    }
    
    // Set import
    void setImport(const std::string& name, void* value) {
        imports_[name] = value;
    }
    
    // Resource table operations
    uint32_t allocResource(uint32_t typeIdx, void* rep) {
        uint32_t handle = nextResourceHandle_++;
        resourceTable_[handle] = {rep, typeIdx};
        return handle;
    }
    
    void* getResource(uint32_t handle) const {
        auto it = resourceTable_.find(handle);
        return it != resourceTable_.end() ? it->second.first : nullptr;
    }
    
    void dropResource(uint32_t handle) {
        resourceTable_.erase(handle);
    }
    
private:
    const ComponentType* type_;
    std::unordered_map<std::string, void*> imports_;
    std::unordered_map<std::string, void*> exports_;
    std::unordered_map<uint32_t, std::pair<void*, uint32_t>> resourceTable_;
    uint32_t nextResourceHandle_ = 1;
};

// =============================================================================
// Component Linker
// =============================================================================

/**
 * @brief Links components together
 */
class ComponentLinker {
public:
    void registerComponent(const std::string& name, std::shared_ptr<ComponentInstance> inst) {
        components_[name] = std::move(inst);
    }
    
    std::shared_ptr<ComponentInstance> instantiate(
        const ComponentType* type,
        const std::unordered_map<std::string, std::string>& importMap
    ) {
        auto instance = std::make_shared<ComponentInstance>(type);
        
        for (const auto& imp : type->imports) {
            auto it = importMap.find(imp.name);
            if (it != importMap.end()) {
                auto comp = components_.find(it->second);
                if (comp != components_.end()) {
                    void* value = comp->second->getExport(imp.name);
                    instance->setImport(imp.name, value);
                }
            }
        }
        
        return instance;
    }
    
private:
    std::unordered_map<std::string, std::shared_ptr<ComponentInstance>> components_;
};

// =============================================================================
// Canonical ABI
// =============================================================================

/**
 * @brief Canonical ABI for lifting/lowering values
 */
class CanonicalABI {
public:
    // Lift: core WASM → component value
    static std::vector<uint64_t> lift(
        const WitType& type,
        const uint8_t* memory,
        size_t offset
    );
    
    // Lower: component value → core WASM
    static void lower(
        const WitType& type,
        const std::vector<uint64_t>& value,
        uint8_t* memory,
        size_t offset
    );
    
    // String encoding
    static std::string liftString(const uint8_t* memory, uint32_t ptr, uint32_t len) {
        return std::string(reinterpret_cast<const char*>(memory + ptr), len);
    }
    
    static std::pair<uint32_t, uint32_t> lowerString(
        const std::string& str,
        uint8_t* memory,
        uint32_t allocPtr
    ) {
        std::memcpy(memory + allocPtr, str.data(), str.size());
        return {allocPtr, static_cast<uint32_t>(str.size())};
    }
    
    // List encoding
    template<typename T>
    static std::vector<T> liftList(const uint8_t* memory, uint32_t ptr, uint32_t len) {
        std::vector<T> result(len);
        const T* data = reinterpret_cast<const T*>(memory + ptr);
        std::memcpy(result.data(), data, len * sizeof(T));
        return result;
    }
};

// =============================================================================
// Component Opcodes
// =============================================================================

namespace ComponentOp {
    // Core section opcodes
    constexpr uint8_t CoreModule = 0x00;
    constexpr uint8_t CoreInstance = 0x01;
    constexpr uint8_t CoreType = 0x02;
    
    // Component section opcodes
    constexpr uint8_t Component = 0x04;
    constexpr uint8_t Instance = 0x05;
    constexpr uint8_t Alias = 0x06;
    constexpr uint8_t Type = 0x07;
    constexpr uint8_t Canon = 0x08;
    constexpr uint8_t Start = 0x09;
    constexpr uint8_t Import = 0x0A;
    constexpr uint8_t Export = 0x0B;
    
    // Canonical function opcodes
    constexpr uint8_t CanonLift = 0x00;
    constexpr uint8_t CanonLower = 0x01;
    constexpr uint8_t ResourceNew = 0x02;
    constexpr uint8_t ResourceDrop = 0x03;
    constexpr uint8_t ResourceRep = 0x04;
}

} // namespace Zepra::Wasm::Component
