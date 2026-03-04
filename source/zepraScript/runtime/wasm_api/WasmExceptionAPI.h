#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <exception>
#include <functional>

namespace Zepra::Wasm {

class WasmException : public std::exception {
private:
    uint32_t tagIndex_;
    std::vector<uint8_t> payload_;
    std::string message_;

public:
    WasmException(uint32_t tagIndex, std::vector<uint8_t> payload = {})
        : tagIndex_(tagIndex), payload_(std::move(payload)) {
        message_ = "WasmException: tag=" + std::to_string(tagIndex_);
    }

    uint32_t tagIndex() const { return tagIndex_; }
    const std::vector<uint8_t>& payload() const { return payload_; }
    const char* what() const noexcept override { return message_.c_str(); }

    template<typename T>
    T getPayloadAs(size_t offset = 0) const {
        if (offset + sizeof(T) > payload_.size()) {
            return T{};
        }
        T result;
        std::memcpy(&result, payload_.data() + offset, sizeof(T));
        return result;
    }
};

struct WasmExceptionTag {
    uint32_t index;
    std::vector<uint8_t> paramTypes;
    std::string name;

    WasmExceptionTag(uint32_t idx, std::vector<uint8_t> types, std::string n = "")
        : index(idx), paramTypes(std::move(types)), name(std::move(n)) {}
};

class WasmExceptionTable {
private:
    std::vector<WasmExceptionTag> tags_;

public:
    uint32_t addTag(std::vector<uint8_t> paramTypes, const std::string& name = "") {
        uint32_t idx = static_cast<uint32_t>(tags_.size());
        tags_.emplace_back(idx, std::move(paramTypes), name);
        return idx;
    }

    const WasmExceptionTag* getTag(uint32_t index) const {
        return index < tags_.size() ? &tags_[index] : nullptr;
    }

    size_t tagCount() const { return tags_.size(); }
};

enum class WasmCatchKind : uint8_t {
    Catch,
    CatchRef,
    CatchAll,
    CatchAllRef
};

struct WasmCatchClause {
    WasmCatchKind kind;
    uint32_t tagIndex;
    uint32_t targetLabel;
};

class WasmTryBlock {
private:
    std::vector<WasmCatchClause> catches_;
    bool hasDelegate_ = false;
    uint32_t delegateTarget_ = 0;

public:
    void addCatch(uint32_t tagIndex, uint32_t targetLabel) {
        catches_.push_back({WasmCatchKind::Catch, tagIndex, targetLabel});
    }

    void addCatchRef(uint32_t tagIndex, uint32_t targetLabel) {
        catches_.push_back({WasmCatchKind::CatchRef, tagIndex, targetLabel});
    }

    void addCatchAll(uint32_t targetLabel) {
        catches_.push_back({WasmCatchKind::CatchAll, 0, targetLabel});
    }

    void addCatchAllRef(uint32_t targetLabel) {
        catches_.push_back({WasmCatchKind::CatchAllRef, 0, targetLabel});
    }

    void setDelegate(uint32_t target) {
        hasDelegate_ = true;
        delegateTarget_ = target;
    }

    const std::vector<WasmCatchClause>& catches() const { return catches_; }
    bool hasDelegate() const { return hasDelegate_; }
    uint32_t delegateTarget() const { return delegateTarget_; }
};

class WasmExceptionHandler {
public:
    using ThrowCallback = std::function<void(const WasmException&)>;
    using CatchCallback = std::function<bool(const WasmException&)>;

private:
    std::vector<WasmTryBlock> tryStack_;
    ThrowCallback onThrow_;
    CatchCallback onCatch_;

public:
    void pushTry(WasmTryBlock block) {
        tryStack_.push_back(std::move(block));
    }

    void popTry() {
        if (!tryStack_.empty()) {
            tryStack_.pop_back();
        }
    }

    void throwException(uint32_t tagIndex, std::vector<uint8_t> payload = {}) {
        WasmException ex(tagIndex, std::move(payload));
        if (onThrow_) onThrow_(ex);
        throw ex;
    }

    template<typename T>
    void throwException(uint32_t tagIndex, T value) {
        std::vector<uint8_t> payload(sizeof(T));
        std::memcpy(payload.data(), &value, sizeof(T));
        throwException(tagIndex, std::move(payload));
    }

    void rethrow(uint32_t depth) {
        (void)depth;
        throw;
    }

    void setThrowCallback(ThrowCallback cb) { onThrow_ = std::move(cb); }
    void setCatchCallback(CatchCallback cb) { onCatch_ = std::move(cb); }

    bool hasTryBlock() const { return !tryStack_.empty(); }
    const WasmTryBlock& currentTry() const { return tryStack_.back(); }
};

class JSWasmExceptionBridge {
public:
    static std::exception_ptr wasmToJS(const WasmException& wasmEx) {
        return std::make_exception_ptr(wasmEx);
    }

    static WasmException jsToWasm(std::exception_ptr ex, uint32_t defaultTag = 0) {
        try {
            std::rethrow_exception(ex);
        } catch (const WasmException& we) {
            return we;
        } catch (const std::exception& e) {
            std::string msg = e.what();
            std::vector<uint8_t> payload(msg.begin(), msg.end());
            return WasmException(defaultTag, std::move(payload));
        } catch (...) {
            return WasmException(defaultTag);
        }
    }

    template<typename Fn>
    static auto wrapWithExceptionHandling(Fn&& fn, WasmExceptionHandler& handler) {
        return [fn = std::forward<Fn>(fn), &handler]() {
            try {
                return fn();
            } catch (const WasmException& e) {
                handler.throwException(e.tagIndex(), e.payload());
                throw;
            }
        };
    }
};

}
