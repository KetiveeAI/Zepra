#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <variant>
#include <unordered_map>

namespace Zepra::Wasm {

enum class WasmExternKind : uint8_t {
    Func,
    Table,
    Memory,
    Global,
    Tag
};

struct WasmFuncType {
    std::vector<uint8_t> params;
    std::vector<uint8_t> results;

    bool operator==(const WasmFuncType& other) const {
        return params == other.params && results == other.results;
    }
};

struct WasmTableType {
    uint8_t elementType;
    uint32_t minSize;
    uint32_t maxSize;
    bool hasMax;
};

struct WasmMemoryType {
    uint32_t minPages;
    uint32_t maxPages;
    bool hasMax;
    bool shared;
    bool memory64;
};

struct WasmGlobalType {
    uint8_t valueType;
    bool mutable_;
};

using WasmExternType = std::variant<WasmFuncType, WasmTableType, WasmMemoryType, WasmGlobalType>;

class WasmTypeReflection {
public:
    static WasmExternKind kindOf(const WasmExternType& type) {
        return static_cast<WasmExternKind>(type.index());
    }

    static std::string kindToString(WasmExternKind kind) {
        switch (kind) {
            case WasmExternKind::Func: return "function";
            case WasmExternKind::Table: return "table";
            case WasmExternKind::Memory: return "memory";
            case WasmExternKind::Global: return "global";
            case WasmExternKind::Tag: return "tag";
            default: return "unknown";
        }
    }

    static std::string valTypeToString(uint8_t valType) {
        switch (valType) {
            case 0x7F: return "i32";
            case 0x7E: return "i64";
            case 0x7D: return "f32";
            case 0x7C: return "f64";
            case 0x7B: return "v128";
            case 0x70: return "funcref";
            case 0x6F: return "externref";
            default: return "unknown";
        }
    }
};

class WasmFunction {
public:
    using NativeFunc = std::function<std::vector<uint64_t>(const std::vector<uint64_t>&)>;

private:
    WasmFuncType type_;
    NativeFunc impl_;
    std::string name_;

public:
    WasmFunction(WasmFuncType type, NativeFunc impl, std::string name = "")
        : type_(std::move(type)), impl_(std::move(impl)), name_(std::move(name)) {}

    std::vector<uint64_t> call(const std::vector<uint64_t>& args) const {
        return impl_(args);
    }

    const WasmFuncType& type() const { return type_; }
    const std::string& name() const { return name_; }
};

class WasmStringEncoding {
public:
    static std::vector<uint8_t> encodeUTF8(const std::string& str) {
        return std::vector<uint8_t>(str.begin(), str.end());
    }

    static std::string decodeUTF8(const std::vector<uint8_t>& bytes) {
        return std::string(bytes.begin(), bytes.end());
    }

    static std::vector<uint16_t> encodeUTF16(const std::string& str) {
        std::vector<uint16_t> result;
        for (char c : str) {
            result.push_back(static_cast<uint16_t>(static_cast<uint8_t>(c)));
        }
        return result;
    }

    static std::vector<uint8_t> encodeWTF8(const std::string& str) {
        return encodeUTF8(str);
    }

    static std::vector<uint16_t> encodeWTF16(const std::string& str) {
        return encodeUTF16(str);
    }
};

class JSWasmInterop {
public:
    using JSValue = std::variant<std::nullptr_t, bool, double, std::string, 
                                  std::vector<uint8_t>, std::shared_ptr<void>>;

    static uint64_t jsToWasm(const JSValue& value, uint8_t targetType) {
        if (auto* b = std::get_if<bool>(&value)) {
            return *b ? 1 : 0;
        }
        if (auto* d = std::get_if<double>(&value)) {
            if (targetType == 0x7F) return static_cast<uint64_t>(static_cast<int32_t>(*d));
            if (targetType == 0x7E) return static_cast<uint64_t>(static_cast<int64_t>(*d));
            if (targetType == 0x7D) {
                float f = static_cast<float>(*d);
                uint32_t bits;
                std::memcpy(&bits, &f, sizeof(bits));
                return bits;
            }
            if (targetType == 0x7C) {
                uint64_t bits;
                std::memcpy(&bits, d, sizeof(bits));
                return bits;
            }
        }
        return 0;
    }

    static JSValue wasmToJS(uint64_t value, uint8_t sourceType) {
        switch (sourceType) {
            case 0x7F: return static_cast<double>(static_cast<int32_t>(value));
            case 0x7E: return static_cast<double>(static_cast<int64_t>(value));
            case 0x7D: {
                uint32_t bits = static_cast<uint32_t>(value);
                float f;
                std::memcpy(&f, &bits, sizeof(f));
                return static_cast<double>(f);
            }
            case 0x7C: {
                double d;
                std::memcpy(&d, &value, sizeof(d));
                return d;
            }
            default: return nullptr;
        }
    }
};

class WasmImportObject {
private:
    std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<void>>> imports_;

public:
    template<typename T>
    void set(const std::string& module, const std::string& name, std::shared_ptr<T> value) {
        imports_[module][name] = std::static_pointer_cast<void>(value);
    }

    template<typename T>
    std::shared_ptr<T> get(const std::string& module, const std::string& name) const {
        auto modIt = imports_.find(module);
        if (modIt == imports_.end()) return nullptr;
        auto nameIt = modIt->second.find(name);
        if (nameIt == modIt->second.end()) return nullptr;
        return std::static_pointer_cast<T>(nameIt->second);
    }

    bool has(const std::string& module, const std::string& name) const {
        auto modIt = imports_.find(module);
        if (modIt == imports_.end()) return false;
        return modIt->second.find(name) != modIt->second.end();
    }
};

class WasmExportObject {
private:
    std::unordered_map<std::string, std::pair<WasmExternKind, std::shared_ptr<void>>> exports_;

public:
    template<typename T>
    void set(const std::string& name, WasmExternKind kind, std::shared_ptr<T> value) {
        exports_[name] = {kind, std::static_pointer_cast<void>(value)};
    }

    template<typename T>
    std::shared_ptr<T> get(const std::string& name) const {
        auto it = exports_.find(name);
        if (it == exports_.end()) return nullptr;
        return std::static_pointer_cast<T>(it->second.second);
    }

    WasmExternKind kindOf(const std::string& name) const {
        auto it = exports_.find(name);
        return it != exports_.end() ? it->second.first : WasmExternKind::Func;
    }

    std::vector<std::string> names() const {
        std::vector<std::string> result;
        for (const auto& [name, _] : exports_) result.push_back(name);
        return result;
    }
};

}
