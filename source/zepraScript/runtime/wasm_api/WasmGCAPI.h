#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <optional>
#include <variant>
#include <string>
#include <functional>

namespace Zepra::Wasm {

enum class WasmHeapType : uint8_t {
    Func,
    Extern,
    Any,
    Eq,
    I31,
    Struct,
    Array,
    None,
    NoFunc,
    NoExtern
};

enum class WasmRefNullability : uint8_t {
    NonNull,
    Nullable
};

struct WasmRefType {
    WasmHeapType heapType;
    WasmRefNullability nullability;
    std::optional<uint32_t> typeIndex;

    bool isNullable() const { return nullability == WasmRefNullability::Nullable; }
    bool isNull() const { return heapType == WasmHeapType::None; }
};

enum class WasmValType : uint8_t {
    I32, I64, F32, F64,
    V128,
    FuncRef, ExternRef,
    AnyRef, EqRef, I31Ref,
    StructRef, ArrayRef
};

struct WasmFieldType {
    WasmValType type;
    bool mutable_;
    std::optional<uint32_t> packedType;
};

class WasmStructType {
private:
    std::vector<WasmFieldType> fields_;
    bool final_ = false;
    std::optional<uint32_t> supertype_;

public:
    WasmStructType() = default;

    void addField(WasmValType type, bool mutable_ = true) {
        fields_.push_back({type, mutable_, std::nullopt});
    }

    void addPackedField(uint32_t packedType, bool mutable_ = true) {
        fields_.push_back({WasmValType::I32, mutable_, packedType});
    }

    size_t fieldCount() const { return fields_.size(); }
    const WasmFieldType& getField(size_t idx) const { return fields_[idx]; }
    
    void setFinal(bool f) { final_ = f; }
    bool isFinal() const { return final_; }
    
    void setSupertype(uint32_t idx) { supertype_ = idx; }
    std::optional<uint32_t> supertype() const { return supertype_; }
};

class WasmArrayType {
private:
    WasmFieldType elementType_;
    bool final_ = false;

public:
    WasmArrayType(WasmValType elemType, bool mutable_ = true)
        : elementType_{elemType, mutable_, std::nullopt} {}

    const WasmFieldType& elementType() const { return elementType_; }
    void setFinal(bool f) { final_ = f; }
    bool isFinal() const { return final_; }
};

using WasmValue = std::variant<
    int32_t, int64_t, float, double,
    std::array<uint8_t, 16>,
    WasmRefType
>;

class WasmStruct {
private:
    const WasmStructType* type_;
    std::vector<WasmValue> fields_;

public:
    explicit WasmStruct(const WasmStructType* type) : type_(type) {
        fields_.resize(type->fieldCount());
    }

    void setField(size_t idx, WasmValue value) {
        if (idx < fields_.size() && type_->getField(idx).mutable_) {
            fields_[idx] = std::move(value);
        }
    }

    const WasmValue& getField(size_t idx) const { return fields_[idx]; }
    const WasmStructType* type() const { return type_; }
};

class WasmArray {
private:
    const WasmArrayType* type_;
    std::vector<WasmValue> elements_;

public:
    WasmArray(const WasmArrayType* type, size_t length)
        : type_(type), elements_(length) {}

    void set(size_t idx, WasmValue value) {
        if (idx < elements_.size() && type_->elementType().mutable_) {
            elements_[idx] = std::move(value);
        }
    }

    const WasmValue& get(size_t idx) const { return elements_[idx]; }
    size_t length() const { return elements_.size(); }
    const WasmArrayType* type() const { return type_; }

    void fill(WasmValue value, size_t start, size_t count) {
        for (size_t i = start; i < start + count && i < elements_.size(); ++i) {
            elements_[i] = value;
        }
    }

    void copy(const WasmArray& src, size_t dstIdx, size_t srcIdx, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            if (dstIdx + i < elements_.size() && srcIdx + i < src.length()) {
                elements_[dstIdx + i] = src.get(srcIdx + i);
            }
        }
    }
};

class I31Ref {
private:
    int32_t value_;

public:
    explicit I31Ref(int32_t v) : value_(v & 0x7FFFFFFF) {}

    int32_t getS() const {
        return (value_ & 0x40000000) ? (value_ | 0x80000000) : value_;
    }

    uint32_t getU() const { return static_cast<uint32_t>(value_); }

    static I31Ref fromS(int32_t v) {
        return I31Ref(v & 0x7FFFFFFF);
    }

    static I31Ref fromU(uint32_t v) {
        return I31Ref(static_cast<int32_t>(v & 0x7FFFFFFF));
    }
};

class WasmGCHeap {
private:
    std::vector<std::shared_ptr<WasmStruct>> structs_;
    std::vector<std::shared_ptr<WasmArray>> arrays_;

public:
    template<typename... Args>
    std::shared_ptr<WasmStruct> allocStruct(Args&&... args) {
        auto ptr = std::make_shared<WasmStruct>(std::forward<Args>(args)...);
        structs_.push_back(ptr);
        return ptr;
    }

    template<typename... Args>
    std::shared_ptr<WasmArray> allocArray(Args&&... args) {
        auto ptr = std::make_shared<WasmArray>(std::forward<Args>(args)...);
        arrays_.push_back(ptr);
        return ptr;
    }

    std::shared_ptr<WasmArray> newArray(const WasmArrayType* type, 
                                         size_t length, 
                                         WasmValue init) {
        auto arr = allocArray(type, length);
        arr->fill(init, 0, length);
        return arr;
    }

    void gc() {
        structs_.erase(
            std::remove_if(structs_.begin(), structs_.end(),
                [](const auto& p) { return p.use_count() == 1; }),
            structs_.end()
        );
        arrays_.erase(
            std::remove_if(arrays_.begin(), arrays_.end(),
                [](const auto& p) { return p.use_count() == 1; }),
            arrays_.end()
        );
    }

    size_t structCount() const { return structs_.size(); }
    size_t arrayCount() const { return arrays_.size(); }
};

class WasmGCTypeChecker {
public:
    static bool isSubtype(const WasmRefType& sub, const WasmRefType& super) {
        if (super.heapType == WasmHeapType::Any) return true;
        if (sub.heapType == WasmHeapType::None) return true;
        
        if (super.heapType == WasmHeapType::Eq) {
            return sub.heapType == WasmHeapType::I31 ||
                   sub.heapType == WasmHeapType::Struct ||
                   sub.heapType == WasmHeapType::Array;
        }
        
        return sub.heapType == super.heapType;
    }

    static bool canCast(const WasmRefType& from, const WasmRefType& to) {
        return isSubtype(from, to) || isSubtype(to, from);
    }
};

}
