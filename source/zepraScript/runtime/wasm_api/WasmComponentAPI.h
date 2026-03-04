#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <optional>

namespace Zepra::Wasm {

enum class ComponentValType : uint8_t {
    Bool,
    S8, U8,
    S16, U16,
    S32, U32,
    S64, U64,
    Float32, Float64,
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
    Own,
    Borrow
};

struct ComponentType;

struct ComponentField {
    std::string name;
    std::shared_ptr<ComponentType> type;
};

struct ComponentCase {
    std::string name;
    std::optional<std::shared_ptr<ComponentType>> type;
};

struct ComponentType {
    ComponentValType kind;
    std::vector<ComponentField> fields;
    std::vector<ComponentCase> cases;
    std::shared_ptr<ComponentType> elementType;
    std::shared_ptr<ComponentType> okType;
    std::shared_ptr<ComponentType> errType;
};

using ComponentValue = std::variant<
    bool,
    int8_t, uint8_t,
    int16_t, uint16_t,
    int32_t, uint32_t,
    int64_t, uint64_t,
    float, double,
    char32_t,
    std::string,
    std::vector<std::shared_ptr<void>>,
    std::unordered_map<std::string, std::shared_ptr<void>>
>;

struct ComponentFunc {
    std::string name;
    std::vector<ComponentField> params;
    std::vector<ComponentField> results;
};

class ComponentInterface {
private:
    std::string name_;
    std::vector<ComponentFunc> functions_;
    std::unordered_map<std::string, std::shared_ptr<ComponentType>> types_;

public:
    explicit ComponentInterface(std::string name) : name_(std::move(name)) {}

    void addFunction(ComponentFunc func) {
        functions_.push_back(std::move(func));
    }

    void addType(const std::string& name, std::shared_ptr<ComponentType> type) {
        types_[name] = std::move(type);
    }

    const std::string& name() const { return name_; }
    const std::vector<ComponentFunc>& functions() const { return functions_; }
    std::shared_ptr<ComponentType> getType(const std::string& name) const {
        auto it = types_.find(name);
        return it != types_.end() ? it->second : nullptr;
    }
};

class ComponentWorld {
private:
    std::string name_;
    std::vector<std::pair<std::string, std::shared_ptr<ComponentInterface>>> imports_;
    std::vector<std::pair<std::string, std::shared_ptr<ComponentInterface>>> exports_;

public:
    explicit ComponentWorld(std::string name) : name_(std::move(name)) {}

    void addImport(const std::string& name, std::shared_ptr<ComponentInterface> iface) {
        imports_.emplace_back(name, std::move(iface));
    }

    void addExport(const std::string& name, std::shared_ptr<ComponentInterface> iface) {
        exports_.emplace_back(name, std::move(iface));
    }

    const std::string& name() const { return name_; }
};

class CanonicalABI {
public:
    static std::vector<uint8_t> liftString(const uint8_t* memory, uint32_t ptr, uint32_t len) {
        return std::vector<uint8_t>(memory + ptr, memory + ptr + len);
    }

    static void lowerString(uint8_t* memory, uint32_t ptr, const std::string& str) {
        std::memcpy(memory + ptr, str.data(), str.size());
    }

    static uint32_t align(uint32_t offset, uint32_t alignment) {
        return (offset + alignment - 1) & ~(alignment - 1);
    }

    static uint32_t sizeOf(ComponentValType type) {
        switch (type) {
            case ComponentValType::Bool:
            case ComponentValType::S8:
            case ComponentValType::U8: return 1;
            case ComponentValType::S16:
            case ComponentValType::U16: return 2;
            case ComponentValType::S32:
            case ComponentValType::U32:
            case ComponentValType::Float32:
            case ComponentValType::Char: return 4;
            case ComponentValType::S64:
            case ComponentValType::U64:
            case ComponentValType::Float64: return 8;
            case ComponentValType::String:
            case ComponentValType::List: return 8;
            default: return 0;
        }
    }

    static uint32_t alignmentOf(ComponentValType type) {
        return sizeOf(type);
    }
};

class WasmComponent {
private:
    std::string name_;
    std::shared_ptr<ComponentWorld> world_;
    std::unordered_map<std::string, std::function<ComponentValue(const std::vector<ComponentValue>&)>> funcs_;

public:
    explicit WasmComponent(std::string name) : name_(std::move(name)) {}

    void setWorld(std::shared_ptr<ComponentWorld> world) {
        world_ = std::move(world);
    }

    void registerFunction(const std::string& name, 
                          std::function<ComponentValue(const std::vector<ComponentValue>&)> fn) {
        funcs_[name] = std::move(fn);
    }

    ComponentValue call(const std::string& name, const std::vector<ComponentValue>& args) const {
        auto it = funcs_.find(name);
        if (it == funcs_.end()) return false;
        return it->second(args);
    }

    const std::string& name() const { return name_; }
    const std::shared_ptr<ComponentWorld>& world() const { return world_; }
};

class ComponentLinker {
private:
    std::unordered_map<std::string, std::shared_ptr<ComponentInterface>> interfaces_;

public:
    void defineInterface(const std::string& name, std::shared_ptr<ComponentInterface> iface) {
        interfaces_[name] = std::move(iface);
    }

    std::shared_ptr<ComponentInterface> getInterface(const std::string& name) const {
        auto it = interfaces_.find(name);
        return it != interfaces_.end() ? it->second : nullptr;
    }

    std::shared_ptr<WasmComponent> instantiate(const std::string& name) {
        return std::make_shared<WasmComponent>(name);
    }
};

class ResourceHandle {
private:
    uint32_t id_;
    bool owned_;
    std::function<void()> destructor_;

public:
    ResourceHandle(uint32_t id, bool owned, std::function<void()> dtor = nullptr)
        : id_(id), owned_(owned), destructor_(std::move(dtor)) {}

    ~ResourceHandle() {
        if (owned_ && destructor_) destructor_();
    }

    uint32_t id() const { return id_; }
    bool isOwned() const { return owned_; }

    ResourceHandle borrow() const {
        return ResourceHandle(id_, false);
    }
};

}
