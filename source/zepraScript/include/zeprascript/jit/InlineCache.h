/**
 * @file InlineCache.h
 * @brief Polymorphic Inline Caching System
 * 
 * SpiderMonkey-inspired IC implementation:
 * - Monomorphic/Polymorphic/Megamorphic states
 * - IC stub chains for property access
 * - CacheIR bytecode for stub compilation
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <memory>
#include <functional>

namespace Zepra::JIT {

// =============================================================================
// IC State Machine
// =============================================================================

enum class ICState : uint8_t {
    Uninitialized,  // Never executed
    Monomorphic,    // Single shape/type
    Polymorphic,    // 2-4 shapes
    Megamorphic     // Too many shapes, go generic
};

// =============================================================================
// Shape ID (Hidden Class)
// =============================================================================

using ShapeId = uint32_t;
constexpr ShapeId INVALID_SHAPE = 0;

/**
 * @brief Compact property descriptor
 */
struct PropertySlot {
    uint16_t offset;      // Byte offset in object
    uint8_t flags;        // Writable, enumerable, etc.
    uint8_t type;         // Expected value type
    
    bool isInline() const { return (flags & 0x80) == 0; }
    bool isAccessor() const { return (flags & 0x40) != 0; }
};

// =============================================================================
// IC Stub (Compiled Cache Entry)
// =============================================================================

/**
 * @brief Single IC stub - one shape → one property access path
 */
class ICStub {
public:
    ICStub(ShapeId shape, PropertySlot slot)
        : expectedShape_(shape), slot_(slot) {}
    
    ShapeId expectedShape() const { return expectedShape_; }
    const PropertySlot& slot() const { return slot_; }
    
    // Native code pointer (set after compilation)
    void* code() const { return code_; }
    void setCode(void* c) { code_ = c; }
    
    // Chain link
    ICStub* next() const { return next_; }
    void setNext(ICStub* n) { next_ = n; }
    
    // Hit counter for tier-up decisions
    uint32_t hitCount() const { return hitCount_; }
    void recordHit() { hitCount_++; }
    
private:
    ShapeId expectedShape_;
    PropertySlot slot_;
    void* code_ = nullptr;
    ICStub* next_ = nullptr;
    uint32_t hitCount_ = 0;
};

// =============================================================================
// IC Chain (Polymorphic Stubs)
// =============================================================================

constexpr size_t MAX_POLYMORPHIC_STUBS = 4;

/**
 * @brief Chain of IC stubs for polymorphic sites
 */
class ICChain {
public:
    ICChain() = default;
    ~ICChain() { clear(); }
    
    // State management
    ICState state() const { return state_; }
    
    // Add stub for shape
    bool addStub(ShapeId shape, PropertySlot slot) {
        if (stubCount_ >= MAX_POLYMORPHIC_STUBS) {
            state_ = ICState::Megamorphic;
            return false;
        }
        
        auto stub = new ICStub(shape, slot);
        stub->setNext(head_);
        head_ = stub;
        stubCount_++;
        
        updateState();
        return true;
    }
    
    // Find stub for shape
    ICStub* findStub(ShapeId shape) const {
        for (ICStub* s = head_; s; s = s->next()) {
            if (s->expectedShape() == shape) {
                return s;
            }
        }
        return nullptr;
    }
    
    // Iterate stubs
    template<typename F>
    void forEach(F&& fn) const {
        for (ICStub* s = head_; s; s = s->next()) {
            fn(*s);
        }
    }
    
    size_t stubCount() const { return stubCount_; }
    
    void clear() {
        ICStub* s = head_;
        while (s) {
            ICStub* next = s->next();
            delete s;
            s = next;
        }
        head_ = nullptr;
        stubCount_ = 0;
        state_ = ICState::Uninitialized;
    }
    
private:
    void updateState() {
        if (stubCount_ == 0) {
            state_ = ICState::Uninitialized;
        } else if (stubCount_ == 1) {
            state_ = ICState::Monomorphic;
        } else if (stubCount_ <= MAX_POLYMORPHIC_STUBS) {
            state_ = ICState::Polymorphic;
        } else {
            state_ = ICState::Megamorphic;
        }
    }
    
    ICStub* head_ = nullptr;
    size_t stubCount_ = 0;
    ICState state_ = ICState::Uninitialized;
};

// =============================================================================
// CacheIR Bytecode (Stub Generation DSL)
// =============================================================================

/**
 * @brief CacheIR opcodes for IC stub generation
 */
enum class CacheIROp : uint8_t {
    // Guards
    GuardShape,         // Check object shape
    GuardType,          // Check value type
    GuardProto,         // Check prototype chain
    GuardNoDetachedBuffer,
    
    // Property access
    LoadFixedSlot,      // Load inline property
    LoadDynamicSlot,    // Load out-of-line property
    StoreFixedSlot,     // Store inline property
    StoreDynamicSlot,   // Store out-of-line property
    
    // Optimized paths
    LoadInt32Slot,      // Load known int32
    LoadDoubleSlot,     // Load known double
    LoadStringSlot,     // Load known string
    
    // Call
    CallNativeGetter,   // Call getter function
    CallNativeSetter,   // Call setter function
    
    // Result
    Return,             // Return result
    Megamorphic,        // Fallback to slow path
};

/**
 * @brief CacheIR bytecode writer
 */
class CacheIRWriter {
public:
    void guardShape(uint8_t objReg, ShapeId shape) {
        emit(CacheIROp::GuardShape);
        emit(objReg);
        emit32(shape);
    }
    
    void loadFixedSlot(uint8_t objReg, uint16_t offset, uint8_t resultReg) {
        emit(CacheIROp::LoadFixedSlot);
        emit(objReg);
        emit16(offset);
        emit(resultReg);
    }
    
    void loadDynamicSlot(uint8_t objReg, uint16_t slotIndex, uint8_t resultReg) {
        emit(CacheIROp::LoadDynamicSlot);
        emit(objReg);
        emit16(slotIndex);
        emit(resultReg);
    }
    
    void storeFixedSlot(uint8_t objReg, uint16_t offset, uint8_t valueReg) {
        emit(CacheIROp::StoreFixedSlot);
        emit(objReg);
        emit16(offset);
        emit(valueReg);
    }
    
    void returnReg(uint8_t reg) {
        emit(CacheIROp::Return);
        emit(reg);
    }
    
    void megamorphic() {
        emit(CacheIROp::Megamorphic);
    }
    
    const std::vector<uint8_t>& bytecode() const { return buffer_; }
    
    void clear() { buffer_.clear(); }
    
private:
    void emit(CacheIROp op) { buffer_.push_back(static_cast<uint8_t>(op)); }
    void emit(uint8_t b) { buffer_.push_back(b); }
    void emit16(uint16_t v) {
        buffer_.push_back(v & 0xFF);
        buffer_.push_back((v >> 8) & 0xFF);
    }
    void emit32(uint32_t v) {
        buffer_.push_back(v & 0xFF);
        buffer_.push_back((v >> 8) & 0xFF);
        buffer_.push_back((v >> 16) & 0xFF);
        buffer_.push_back((v >> 24) & 0xFF);
    }
    
    std::vector<uint8_t> buffer_;
};

// =============================================================================
// IC Site (Per-Bytecode Cache)
// =============================================================================

enum class ICKind : uint8_t {
    GetProp,          // obj.prop
    SetProp,          // obj.prop = value
    GetElem,          // obj[key]
    SetElem,          // obj[key] = value
    Call,             // fn()
    Construct,        // new Fn()
    BinaryOp,         // a + b
    UnaryOp,          // -a
    Compare,          // a < b
    TypeOf,           // typeof x
    In,               // key in obj
    InstanceOf        // obj instanceof Ctor
};

/**
 * @brief Single IC site attached to bytecode
 */
class ICSite {
public:
    ICSite(ICKind kind, uint32_t bytecodeOffset)
        : kind_(kind), bytecodeOffset_(bytecodeOffset) {}
    
    ICKind kind() const { return kind_; }
    uint32_t bytecodeOffset() const { return bytecodeOffset_; }
    
    ICChain& chain() { return chain_; }
    const ICChain& chain() const { return chain_; }
    
    ICState state() const { return chain_.state(); }
    
    // Statistics
    uint32_t missCount() const { return missCount_; }
    void recordMiss() { missCount_++; }
    
    bool shouldRecompile() const {
        return missCount_ > 100 && chain_.stubCount() < MAX_POLYMORPHIC_STUBS;
    }
    
private:
    ICKind kind_;
    uint32_t bytecodeOffset_;
    ICChain chain_;
    uint32_t missCount_ = 0;
};

// =============================================================================
// IC Manager
// =============================================================================

/**
 * @brief Manages all IC sites for a function
 */
class ICManager {
public:
    static ICManager& instance() {
        static ICManager mgr;
        return mgr;
    }
    
    // Get or create IC site
    ICSite* getSite(uintptr_t functionId, uint32_t bytecodeOffset, ICKind kind) {
        auto key = makeKey(functionId, bytecodeOffset);
        
        auto it = sites_.find(key);
        if (it != sites_.end()) {
            return it->second.get();
        }
        
        auto site = std::make_unique<ICSite>(kind, bytecodeOffset);
        auto* ptr = site.get();
        sites_[key] = std::move(site);
        return ptr;
    }
    
    // Statistics
    struct Stats {
        size_t totalSites = 0;
        size_t monomorphic = 0;
        size_t polymorphic = 0;
        size_t megamorphic = 0;
        size_t uninitialized = 0;
    };
    
    Stats collectStats() const {
        Stats s;
        s.totalSites = sites_.size();
        for (const auto& [_, site] : sites_) {
            switch (site->state()) {
                case ICState::Uninitialized: s.uninitialized++; break;
                case ICState::Monomorphic: s.monomorphic++; break;
                case ICState::Polymorphic: s.polymorphic++; break;
                case ICState::Megamorphic: s.megamorphic++; break;
            }
        }
        return s;
    }
    
    void clear() { sites_.clear(); }
    
private:
    ICManager() = default;
    
    uint64_t makeKey(uintptr_t fn, uint32_t offset) {
        return (static_cast<uint64_t>(fn) << 32) | offset;
    }
    
    std::unordered_map<uint64_t, std::unique_ptr<ICSite>> sites_;
};

} // namespace Zepra::JIT
