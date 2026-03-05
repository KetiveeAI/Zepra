/**
 * @file gc_handle_scope.cpp
 * @brief Handle scopes for protecting references during GC
 *
 * When C++ code (builtins, host functions) holds pointers to
 * JS objects, these must be registered as roots so the GC
 * doesn't collect them. HandleScopes automate this via RAII.
 *
 * HandleScope: RAII scope that registers/unregisters handles
 * Handle<T>: Smart pointer that registers with the current scope
 * EscapableHandleScope: Allows one handle to escape to parent scope
 * PersistentHandle: Long-lived handle (not scope-based)
 *
 * Usage:
 *   void myBuiltin(GCHeap* heap) {
 *       HandleScope scope(heap);
 *       Handle<Object> obj = scope.create(allocateObject());
 *       // obj is protected from GC for this scope
 *       doSomething(obj);
 *   }  // scope exits, handle is removed from roots
 */

#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <memory>

namespace Zepra::Heap {

// =============================================================================
// Handle (GC-protected pointer)
// =============================================================================

/**
 * @brief A GC-safe reference to a heap object
 *
 * The handle stores a pointer to a "slot" that holds the real
 * pointer. During GC compaction, the GC updates the slot's
 * value, and the handle follows via indirection.
 */
template<typename T>
class Handle {
public:
    Handle() : slot_(nullptr) {}
    explicit Handle(T** slot) : slot_(slot) {}

    T* get() const { return slot_ ? *slot_ : nullptr; }
    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }
    explicit operator bool() const { return get() != nullptr; }

    bool isNull() const { return get() == nullptr; }

    /**
     * @brief Get the slot address (for GC to update during compaction)
     */
    T** slot() const { return slot_; }

    bool operator==(const Handle& other) const {
        return get() == other.get();
    }

    bool operator!=(const Handle& other) const {
        return get() != other.get();
    }

private:
    T** slot_;
};

// =============================================================================
// Handle Block (internal allocator for handle slots)
// =============================================================================

class HandleBlock {
public:
    static constexpr size_t SLOTS_PER_BLOCK = 256;

    HandleBlock() : used_(0), next_(nullptr) {}

    void** allocateSlot() {
        if (used_ >= SLOTS_PER_BLOCK) return nullptr;
        void** slot = &slots_[used_++];
        *slot = nullptr;
        return slot;
    }

    size_t used() const { return used_; }

    void reset() {
        // Zero out used slots
        for (size_t i = 0; i < used_; i++) {
            slots_[i] = nullptr;
        }
        used_ = 0;
    }

    HandleBlock* next() const { return next_; }
    void setNext(HandleBlock* n) { next_ = n; }

    void** slotAt(size_t index) {
        return index < used_ ? &slots_[index] : nullptr;
    }

    /**
     * @brief Enumerate all active slots (for GC root scanning)
     */
    void forEachSlot(std::function<void(void** slot)> visitor) {
        for (size_t i = 0; i < used_; i++) {
            if (slots_[i] != nullptr) {
                visitor(&slots_[i]);
            }
        }
    }

private:
    void* slots_[SLOTS_PER_BLOCK];
    size_t used_;
    HandleBlock* next_;
};

// =============================================================================
// HandleScope
// =============================================================================

// Forward declaration
class HandleScopeManager;

/**
 * @brief RAII scope for GC handles
 *
 * All handles created within a scope are automatically
 * released when the scope exits. Scopes form a stack.
 */
class HandleScope {
public:
    explicit HandleScope(HandleScopeManager* manager);
    ~HandleScope();

    HandleScope(const HandleScope&) = delete;
    HandleScope& operator=(const HandleScope&) = delete;

    /**
     * @brief Create a handle in this scope
     */
    template<typename T>
    Handle<T> create(T* ptr) {
        void** slot = allocateSlot();
        if (!slot) return Handle<T>();
        *slot = ptr;
        return Handle<T>(reinterpret_cast<T**>(slot));
    }

    size_t handleCount() const {
        return currentBlock_ ? blockBaseUsed_ + currentBlock_->used() : 0;
    }

private:
    friend class HandleScopeManager;

    void** allocateSlot();

    HandleScopeManager* manager_;
    HandleScope* previous_;
    HandleBlock* currentBlock_;
    size_t blockBaseUsed_;  // Used count when scope opened
};

// =============================================================================
// EscapableHandleScope
// =============================================================================

/**
 * @brief HandleScope that allows one handle to escape to parent
 *
 * Used when a function needs to return a handle to its caller:
 *   Handle<Object> createObject(HandleScopeManager* mgr) {
 *       EscapableHandleScope scope(mgr);
 *       Handle<Object> obj = scope.create(new Object());
 *       // ... initialize obj ...
 *       return scope.escape(obj);  // Move to parent scope
 *   }
 */
class EscapableHandleScope : public HandleScope {
public:
    explicit EscapableHandleScope(HandleScopeManager* manager)
        : HandleScope(manager)
        , escaped_(false)
        , manager_(manager) {}

    /**
     * @brief Escape one handle to the parent scope
     *
     * Can only be called once per EscapableHandleScope.
     */
    template<typename T>
    Handle<T> escape(Handle<T> handle);

private:
    bool escaped_;
    HandleScopeManager* manager_;
};

// =============================================================================
// PersistentHandle
// =============================================================================

/**
 * @brief Long-lived GC handle (not tied to a scope)
 *
 * Must be explicitly released. Used for:
 * - Global references held by native code
 * - Weak callbacks
 * - Event listeners
 */
template<typename T>
class PersistentHandle {
public:
    PersistentHandle() : slot_(nullptr), weakCallback_(nullptr) {}

    explicit PersistentHandle(T* ptr) : weakCallback_(nullptr) {
        slot_ = new T*(ptr);
    }

    ~PersistentHandle() {
        release();
    }

    PersistentHandle(PersistentHandle&& other) noexcept
        : slot_(other.slot_)
        , weakCallback_(std::move(other.weakCallback_)) {
        other.slot_ = nullptr;
    }

    PersistentHandle& operator=(PersistentHandle&& other) noexcept {
        if (this != &other) {
            release();
            slot_ = other.slot_;
            weakCallback_ = std::move(other.weakCallback_);
            other.slot_ = nullptr;
        }
        return *this;
    }

    PersistentHandle(const PersistentHandle&) = delete;
    PersistentHandle& operator=(const PersistentHandle&) = delete;

    T* get() const { return slot_ ? *slot_ : nullptr; }
    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }
    explicit operator bool() const { return get() != nullptr; }

    void reset(T* ptr) {
        if (slot_) {
            *slot_ = ptr;
        } else {
            slot_ = new T*(ptr);
        }
    }

    void release() {
        delete slot_;
        slot_ = nullptr;
    }

    /**
     * @brief Set a weak callback (called when referent is collected)
     */
    void setWeak(std::function<void(T*)> callback) {
        weakCallback_ = std::move(callback);
    }

    bool isWeak() const { return weakCallback_ != nullptr; }

    /**
     * @brief Get slot address for GC root tracking
     */
    void** rootSlot() { return reinterpret_cast<void**>(slot_); }

private:
    T** slot_;
    std::function<void(T*)> weakCallback_;
};

// =============================================================================
// HandleScope Manager
// =============================================================================

/**
 * @brief Manages the handle scope stack and block allocation
 */
class HandleScopeManager {
public:
    HandleScopeManager() : currentScope_(nullptr) {
        // Allocate initial block
        blocks_.push_back(std::make_unique<HandleBlock>());
        currentBlock_ = blocks_.back().get();
    }

    ~HandleScopeManager() = default;

    HandleScope* currentScope() { return currentScope_; }
    HandleBlock* currentBlock() { return currentBlock_; }

    void pushScope(HandleScope* scope) {
        scope->previous_ = currentScope_;
        currentScope_ = scope;
    }

    void popScope(HandleScope* scope) {
        assert(currentScope_ == scope);
        currentScope_ = scope->previous_;

        // Reset the block back to where this scope started
        if (currentBlock_) {
            currentBlock_->reset();
        }
    }

    void** allocateSlot() {
        void** slot = currentBlock_->allocateSlot();
        if (slot) return slot;

        // Need new block
        auto newBlock = std::make_unique<HandleBlock>();
        newBlock->setNext(currentBlock_);
        currentBlock_ = newBlock.get();
        blocks_.push_back(std::move(newBlock));

        return currentBlock_->allocateSlot();
    }

    /**
     * @brief Enumerate all roots from handles (for GC)
     */
    void enumerateHandleRoots(std::function<void(void** slot)> visitor) {
        for (auto& block : blocks_) {
            block->forEachSlot(visitor);
        }
    }

    size_t blockCount() const { return blocks_.size(); }

private:
    HandleScope* currentScope_;
    HandleBlock* currentBlock_;
    std::vector<std::unique_ptr<HandleBlock>> blocks_;

    friend class HandleScope;
    friend class EscapableHandleScope;
};

// =============================================================================
// HandleScope implementation
// =============================================================================

inline HandleScope::HandleScope(HandleScopeManager* manager)
    : manager_(manager)
    , previous_(nullptr)
    , currentBlock_(manager->currentBlock())
    , blockBaseUsed_(manager->currentBlock()->used()) {
    manager->pushScope(this);
}

inline HandleScope::~HandleScope() {
    manager_->popScope(this);
}

inline void** HandleScope::allocateSlot() {
    return manager_->allocateSlot();
}

template<typename T>
Handle<T> EscapableHandleScope::escape(Handle<T> handle) {
    assert(!escaped_ && "Can only escape one handle per scope");
    escaped_ = true;

    // Allocate a slot in the parent scope (if any)
    T* ptr = handle.get();
    void** parentSlot = manager_->allocateSlot();
    if (parentSlot) {
        *parentSlot = ptr;
    }
    return Handle<T>(reinterpret_cast<T**>(parentSlot));
}

} // namespace Zepra::Heap
