/**
 * @file TransferableAPI.h
 * @brief Transferable Objects Implementation
 */

#pragma once

#include <memory>
#include <vector>
#include <any>
#include <typeindex>
#include <stdexcept>

namespace Zepra::Runtime {

// =============================================================================
// Transferable Interface
// =============================================================================

class Transferable {
public:
    virtual ~Transferable() = default;
    virtual bool isDetached() const = 0;
    virtual void detach() = 0;
    virtual std::unique_ptr<Transferable> transfer() = 0;
    virtual std::type_index type() const = 0;
};

// =============================================================================
// Transfer List
// =============================================================================

class TransferList {
public:
    void add(Transferable* obj) {
        if (obj && !obj->isDetached()) {
            items_.push_back(obj);
        }
    }
    
    bool contains(Transferable* obj) const {
        for (auto* item : items_) {
            if (item == obj) return true;
        }
        return false;
    }
    
    std::vector<std::unique_ptr<Transferable>> transferAll() {
        std::vector<std::unique_ptr<Transferable>> result;
        result.reserve(items_.size());
        
        for (auto* item : items_) {
            result.push_back(item->transfer());
        }
        
        items_.clear();
        return result;
    }
    
    size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }

private:
    std::vector<Transferable*> items_;
};

// =============================================================================
// Transferable ArrayBuffer Wrapper
// =============================================================================

template<typename BufferType>
class TransferableBuffer : public Transferable {
public:
    explicit TransferableBuffer(std::shared_ptr<BufferType> buffer)
        : buffer_(std::move(buffer)), detached_(false) {}
    
    bool isDetached() const override { return detached_; }
    
    void detach() override {
        detached_ = true;
        buffer_.reset();
    }
    
    std::unique_ptr<Transferable> transfer() override {
        if (detached_) {
            throw std::runtime_error("Cannot transfer detached buffer");
        }
        
        auto transferred = std::make_unique<TransferableBuffer>(std::move(buffer_));
        detached_ = true;
        return transferred;
    }
    
    std::type_index type() const override {
        return std::type_index(typeid(BufferType));
    }
    
    std::shared_ptr<BufferType> buffer() const {
        if (detached_) return nullptr;
        return buffer_;
    }

private:
    std::shared_ptr<BufferType> buffer_;
    bool detached_;
};

// =============================================================================
// Structured Clone with Transfer
// =============================================================================

struct CloneOptions {
    TransferList transfer;
};

class StructuredCloneTransfer {
public:
    template<typename T>
    static T clone(const T& value, const CloneOptions& options = {}) {
        // Deep clone with transfer support
        return deepClone(value, options);
    }
    
    template<typename T>
    static T deepClone(const T& value, const CloneOptions& options) {
        // For transferable objects, transfer instead of clone
        if constexpr (std::is_base_of_v<Transferable, T>) {
            auto* ptr = const_cast<T*>(&value);
            if (const_cast<CloneOptions&>(options).transfer.contains(ptr)) {
                return *static_cast<T*>(ptr->transfer().release());
            }
        }
        
        // Default: copy
        return value;
    }
};

// =============================================================================
// Stream Transfer
// =============================================================================

class ReadableStreamTransfer : public Transferable {
public:
    bool isDetached() const override { return detached_; }
    
    void detach() override { detached_ = true; }
    
    std::unique_ptr<Transferable> transfer() override {
        if (detached_) {
            throw std::runtime_error("Stream already transferred");
        }
        detached_ = true;
        return std::make_unique<ReadableStreamTransfer>();
    }
    
    std::type_index type() const override {
        return std::type_index(typeid(ReadableStreamTransfer));
    }

private:
    bool detached_ = false;
};

} // namespace Zepra::Runtime
