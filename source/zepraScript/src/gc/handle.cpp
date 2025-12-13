/**
 * @file handle.cpp
 * @brief GC-safe handle system implementation
 * 
 * Provides HandleBase for safe references to GC-managed objects.
 * Handles automatically register with the GC as roots, preventing
 * the referenced object from being collected.
 * 
 * HandleScope provides RAII-based automatic cleanup of handles
 * when exiting a scope.
 */

#include "zeprascript/config.hpp"
#include "zeprascript/gc/heap.hpp"
#include "zeprascript/runtime/object.hpp"
#include <vector>
#include <cassert>
#include <cstdint>

namespace Zepra::GC {

// =============================================================================
// HandleStorage - Global storage for handle slots
// =============================================================================

class HandleStorage {
public:
    static constexpr size_t BLOCK_SIZE = 256;
    static constexpr size_t MAX_BLOCKS = 64;
    
    struct Block {
        void* slots[BLOCK_SIZE];
        size_t used;
        
        Block() : used(0) {
            for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                slots[i] = nullptr;
            }
        }
        
        void** allocate() {
            if (used >= BLOCK_SIZE) return nullptr;
            return &slots[used++];
        }
        
        void reset(size_t toIndex) {
            while (used > toIndex) {
                slots[--used] = nullptr;
            }
        }
    };
    
    HandleStorage() {
        // Create initial block
        blocks_.push_back(new Block());
        currentBlock_ = 0;
    }
    
    ~HandleStorage() {
        for (Block* block : blocks_) {
            delete block;
        }
    }
    
    /**
     * @brief Allocate a new handle slot
     * @return Pointer to the slot (stores the object pointer)
     */
    void** allocateSlot() {
        Block* block = blocks_[currentBlock_];
        void** slot = block->allocate();
        
        if (!slot) {
            // Current block full, try next or create new
            if (currentBlock_ + 1 < blocks_.size()) {
                currentBlock_++;
                block = blocks_[currentBlock_];
                slot = block->allocate();
            } else if (blocks_.size() < MAX_BLOCKS) {
                blocks_.push_back(new Block());
                currentBlock_ = blocks_.size() - 1;
                slot = blocks_[currentBlock_]->allocate();
            }
        }
        
        return slot;
    }
    
    /**
     * @brief Get current allocation position for scope tracking
     */
    struct Position {
        size_t blockIndex;
        size_t slotIndex;
    };
    
    Position currentPosition() const {
        return {currentBlock_, blocks_[currentBlock_]->used};
    }
    
    /**
     * @brief Reset to a previous position (for HandleScope cleanup)
     */
    void resetTo(const Position& pos) {
        // Clear blocks after the target block
        for (size_t i = pos.blockIndex + 1; i <= currentBlock_; ++i) {
            blocks_[i]->reset(0);
        }
        
        // Reset target block
        blocks_[pos.blockIndex]->reset(pos.slotIndex);
        currentBlock_ = pos.blockIndex;
    }
    
    /**
     * @brief Iterate all valid handle slots (for GC root enumeration)
     */
    template<typename Callback>
    void forEach(Callback&& callback) const {
        for (size_t b = 0; b <= currentBlock_; ++b) {
            const Block* block = blocks_[b];
            size_t end = (b == currentBlock_) ? block->used : BLOCK_SIZE;
            for (size_t i = 0; i < end; ++i) {
                if (block->slots[i]) {
                    callback(block->slots[i]);
                }
            }
        }
    }
    
    /**
     * @brief Count active handles
     */
    size_t handleCount() const {
        size_t count = 0;
        for (size_t b = 0; b <= currentBlock_; ++b) {
            const Block* block = blocks_[b];
            size_t end = (b == currentBlock_) ? block->used : BLOCK_SIZE;
            count += end;
        }
        return count;
    }
    
private:
    std::vector<Block*> blocks_;
    size_t currentBlock_ = 0;
};

// Global handle storage instance
static HandleStorage* globalHandleStorage = nullptr;

HandleStorage* getHandleStorage() {
    if (!globalHandleStorage) {
        globalHandleStorage = new HandleStorage();
    }
    return globalHandleStorage;
}

void shutdownHandleStorage() {
    delete globalHandleStorage;
    globalHandleStorage = nullptr;
}

// =============================================================================
// HandleBase - Non-template base class for GC handles
// =============================================================================

class HandleBase {
public:
    HandleBase() : slot_(nullptr) {}
    
    explicit HandleBase(void* ptr) {
        slot_ = getHandleStorage()->allocateSlot();
        if (slot_) {
            *slot_ = ptr;
        }
    }
    
    HandleBase(const HandleBase& other) {
        slot_ = getHandleStorage()->allocateSlot();
        if (slot_ && other.slot_) {
            *slot_ = *other.slot_;
        }
    }
    
    HandleBase& operator=(const HandleBase& other) {
        if (this != &other && slot_ && other.slot_) {
            *slot_ = *other.slot_;
        }
        return *this;
    }
    
    bool isNull() const {
        return !slot_ || !*slot_;
    }
    
    void clear() {
        if (slot_) {
            *slot_ = nullptr;
        }
    }
    
    void* raw() const {
        return slot_ ? *slot_ : nullptr;
    }
    
    void set(void* ptr) {
        if (slot_) {
            *slot_ = ptr;
        }
    }
    
protected:
    void** slot_;
};

// =============================================================================
// ObjectHandle - Type-specific handle for Runtime::Object
// =============================================================================

class ObjectHandle : public HandleBase {
public:
    ObjectHandle() : HandleBase() {}
    
    explicit ObjectHandle(Runtime::Object* ptr) 
        : HandleBase(static_cast<void*>(ptr)) {}
    
    Runtime::Object* get() const {
        return static_cast<Runtime::Object*>(raw());
    }
    
    Runtime::Object* operator->() const {
        assert(!isNull() && "Dereferencing null handle");
        return get();
    }
    
    Runtime::Object& operator*() const {
        assert(!isNull() && "Dereferencing null handle");
        return *get();
    }
    
    explicit operator bool() const {
        return !isNull();
    }
    
    bool operator==(const ObjectHandle& other) const {
        return raw() == other.raw();
    }
    
    bool operator!=(const ObjectHandle& other) const {
        return raw() != other.raw();
    }
};

// =============================================================================
// HandleScope - RAII scope for automatic handle cleanup
// =============================================================================

class HandleScope {
public:
    HandleScope() {
        storage_ = getHandleStorage();
        savedPosition_ = storage_->currentPosition();
        
        // Link to previous scope
        previousScope_ = currentScope_;
        currentScope_ = this;
    }
    
    ~HandleScope() {
        // Reset storage to saved position
        storage_->resetTo(savedPosition_);
        
        // Unlink from scope chain
        currentScope_ = previousScope_;
    }
    
    // Non-copyable
    HandleScope(const HandleScope&) = delete;
    HandleScope& operator=(const HandleScope&) = delete;
    
    /**
     * @brief Create an object handle in this scope
     */
    ObjectHandle createObjectHandle(Runtime::Object* ptr) {
        return ObjectHandle(ptr);
    }
    
    /**
     * @brief Get number of handles in this scope
     */
    size_t handleCount() const {
        auto current = storage_->currentPosition();
        
        // Count difference from saved position
        if (current.blockIndex == savedPosition_.blockIndex) {
            return current.slotIndex - savedPosition_.slotIndex;
        }
        
        // Multiple blocks
        size_t count = HandleStorage::BLOCK_SIZE - savedPosition_.slotIndex;
        for (size_t b = savedPosition_.blockIndex + 1; b < current.blockIndex; ++b) {
            count += HandleStorage::BLOCK_SIZE;
        }
        count += current.slotIndex;
        return count;
    }
    
    /**
     * @brief Get the current innermost scope
     */
    static HandleScope* current() {
        return currentScope_;
    }
    
protected:
    HandleStorage* storage_;
    HandleStorage::Position savedPosition_;
    HandleScope* previousScope_;
    
    static thread_local HandleScope* currentScope_;
};

thread_local HandleScope* HandleScope::currentScope_ = nullptr;

// =============================================================================
// EscapableHandleScope - HandleScope that can return handles to caller
// =============================================================================

class EscapableHandleScope : public HandleScope {
public:
    EscapableHandleScope() : HandleScope(), escaped_(false), escapedValue_(nullptr) {}
    
    /**
     * @brief Escape a value to the parent scope (can only be called once)
     * @return The escaped handle in parent scope
     */
    ObjectHandle escapeObject(ObjectHandle handle) {
        assert(!escaped_ && "Can only escape one handle per EscapableHandleScope");
        escaped_ = true;
        
        // Store the value
        Runtime::Object* value = handle.get();
        escapedValue_ = value;
        
        // Close this scope early (resets to saved position)
        storage_->resetTo(savedPosition_);
        
        // Create handle in parent scope's context
        return ObjectHandle(value);
    }
    
private:
    bool escaped_;
    void* escapedValue_;
};

// =============================================================================
// PersistentObjectHandle - Handle that outlives scopes
// =============================================================================

class PersistentObjectHandle {
public:
    PersistentObjectHandle() : ptr_(nullptr) {}
    
    explicit PersistentObjectHandle(Runtime::Object* ptr) : ptr_(ptr) {
        // Production would register with Heap as root
    }
    
    ~PersistentObjectHandle() {
        reset();
    }
    
    PersistentObjectHandle(const PersistentObjectHandle& other) : ptr_(nullptr) {
        reset(other.ptr_);
    }
    
    PersistentObjectHandle& operator=(const PersistentObjectHandle& other) {
        if (this != &other) {
            reset(other.ptr_);
        }
        return *this;
    }
    
    PersistentObjectHandle(PersistentObjectHandle&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }
    
    PersistentObjectHandle& operator=(PersistentObjectHandle&& other) noexcept {
        if (this != &other) {
            reset();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }
    
    void reset(Runtime::Object* ptr = nullptr) {
        ptr_ = ptr;
    }
    
    Runtime::Object* get() const { return ptr_; }
    
    Runtime::Object* operator->() const {
        assert(ptr_ && "Dereferencing null persistent handle");
        return ptr_;
    }
    
    Runtime::Object& operator*() const {
        assert(ptr_ && "Dereferencing null persistent handle");
        return *ptr_;
    }
    
    explicit operator bool() const { return ptr_ != nullptr; }
    
private:
    Runtime::Object* ptr_;
};

// =============================================================================
// GC Integration Functions
// =============================================================================

/**
 * @brief Register all handles as GC roots
 */
void registerHandlesAsRoots(Heap* heap) {
    if (!globalHandleStorage || !heap) return;
    
    globalHandleStorage->forEach([heap](void* ptr) {
        if (ptr) {
            heap->addRoot(ptr);
        }
    });
}

/**
 * @brief Get count of active handles (for debugging/stats)
 */
size_t getActiveHandleCount() {
    if (!globalHandleStorage) return 0;
    return globalHandleStorage->handleCount();
}

} // namespace Zepra::GC
