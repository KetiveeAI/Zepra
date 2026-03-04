#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <functional>
#include <memory>
#include <mutex>

namespace Zepra::Runtime {

class SymbolValue {
private:
    std::string description_;
    size_t id_;

    static size_t nextId() {
        static std::atomic<size_t> counter{0};
        return ++counter;
    }

public:
    explicit SymbolValue(const std::string& description = "")
        : description_(description), id_(nextId()) {}

    const std::string& description() const { return description_; }
    size_t id() const { return id_; }

    bool operator==(const SymbolValue& other) const { return id_ == other.id_; }
    bool operator!=(const SymbolValue& other) const { return id_ != other.id_; }
};

class WellKnownSymbols {
public:
    static const SymbolValue& dispose() {
        static SymbolValue s("Symbol.dispose");
        return s;
    }

    static const SymbolValue& asyncDispose() {
        static SymbolValue s("Symbol.asyncDispose");
        return s;
    }

    static const SymbolValue& metadata() {
        static SymbolValue s("Symbol.metadata");
        return s;
    }

    static const SymbolValue& iterator() {
        static SymbolValue s("Symbol.iterator");
        return s;
    }

    static const SymbolValue& asyncIterator() {
        static SymbolValue s("Symbol.asyncIterator");
        return s;
    }

    static const SymbolValue& toStringTag() {
        static SymbolValue s("Symbol.toStringTag");
        return s;
    }

    static const SymbolValue& hasInstance() {
        static SymbolValue s("Symbol.hasInstance");
        return s;
    }

    static const SymbolValue& isConcatSpreadable() {
        static SymbolValue s("Symbol.isConcatSpreadable");
        return s;
    }

    static const SymbolValue& species() {
        static SymbolValue s("Symbol.species");
        return s;
    }

    static const SymbolValue& toPrimitive() {
        static SymbolValue s("Symbol.toPrimitive");
        return s;
    }

    static const SymbolValue& unscopables() {
        static SymbolValue s("Symbol.unscopables");
        return s;
    }

    static const SymbolValue& match() {
        static SymbolValue s("Symbol.match");
        return s;
    }

    static const SymbolValue& matchAll() {
        static SymbolValue s("Symbol.matchAll");
        return s;
    }

    static const SymbolValue& replace() {
        static SymbolValue s("Symbol.replace");
        return s;
    }

    static const SymbolValue& search() {
        static SymbolValue s("Symbol.search");
        return s;
    }

    static const SymbolValue& split() {
        static SymbolValue s("Symbol.split");
        return s;
    }
};

class SymbolRegistry {
private:
    std::unordered_map<std::string, SymbolValue> registry_;
    std::mutex mutex_;

public:
    static SymbolRegistry& global() {
        static SymbolRegistry instance;
        return instance;
    }

    SymbolValue for_(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = registry_.find(key);
        if (it != registry_.end()) {
            return it->second;
        }
        auto [inserted, _] = registry_.emplace(key, SymbolValue(key));
        return inserted->second;
    }

    std::optional<std::string> keyFor(const SymbolValue& sym) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [key, value] : registry_) {
            if (value == sym) return key;
        }
        return std::nullopt;
    }

    bool has(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        return registry_.find(key) != registry_.end();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        registry_.clear();
    }
};

class SymbolEnhancements {
public:
    static SymbolValue create(const std::string& description = "") {
        return SymbolValue(description);
    }

    static SymbolValue for_(const std::string& key) {
        return SymbolRegistry::global().for_(key);
    }

    static std::optional<std::string> keyFor(const SymbolValue& sym) {
        return SymbolRegistry::global().keyFor(sym);
    }

    static const SymbolValue& dispose() { return WellKnownSymbols::dispose(); }
    static const SymbolValue& asyncDispose() { return WellKnownSymbols::asyncDispose(); }
    static const SymbolValue& metadata() { return WellKnownSymbols::metadata(); }
    static const SymbolValue& iterator() { return WellKnownSymbols::iterator(); }
    static const SymbolValue& asyncIterator() { return WellKnownSymbols::asyncIterator(); }
};

template<typename T>
class Disposable {
public:
    virtual ~Disposable() = default;
    virtual void dispose() = 0;
};

template<typename T>
class AsyncDisposable {
public:
    virtual ~AsyncDisposable() = default;
    virtual std::future<void> disposeAsync() = 0;
};

class DisposableStack {
private:
    std::vector<std::function<void()>> disposers_;
    bool disposed_ = false;

public:
    ~DisposableStack() {
        if (!disposed_) dispose();
    }

    template<typename T>
    T& use(T& resource) {
        static_assert(std::is_base_of_v<Disposable<T>, T>, "Resource must be Disposable");
        disposers_.push_back([&resource]() { resource.dispose(); });
        return resource;
    }

    void defer(std::function<void()> fn) {
        disposers_.push_back(std::move(fn));
    }

    void adopt(void* value, std::function<void(void*)> onDispose) {
        disposers_.push_back([value, onDispose]() { onDispose(value); });
    }

    void move(DisposableStack& other) {
        for (auto& d : other.disposers_) {
            disposers_.push_back(std::move(d));
        }
        other.disposers_.clear();
        other.disposed_ = true;
    }

    void dispose() {
        if (disposed_) return;
        for (auto it = disposers_.rbegin(); it != disposers_.rend(); ++it) {
            try { (*it)(); } catch (...) {}
        }
        disposers_.clear();
        disposed_ = true;
    }

    bool isDisposed() const { return disposed_; }
};

}
