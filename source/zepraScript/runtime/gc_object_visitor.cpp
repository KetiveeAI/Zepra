/**
 * @file gc_object_visitor.cpp
 * @brief Runtime-side GC object visitor — bridges Object::visitRefs to GC
 *
 * This lives in the runtime because it understands Object layout,
 * prototype chains, property storage, and closures. The GC calls
 * this to discover outgoing references from any JS object.
 *
 * visitRefs is the single most important function for GC correctness:
 * if it misses a reference, the GC will collect a live object → crash.
 */

#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <unordered_map>

namespace Zepra::Runtime {

// =============================================================================
// Object Types (mirrors runtime/objects/object.hpp ObjectType)
// =============================================================================

enum class VisitableType : uint8_t {
    Ordinary = 0,
    Array,
    Function,
    BoundFunction,
    String,
    Boolean,
    Number,
    Date,
    RegExp,
    Error,
    Map,
    Set,
    WeakMap,
    WeakSet,
    Promise,
    Generator,
    AsyncFunction,
    ArrayBuffer,
    TypedArray,
    DataView,
    SharedArrayBuffer,
    Atomics,
    WeakRef,
    FinalizationRegistry,
    Proxy,
    Symbol,
    BigInt,
};

// =============================================================================
// Reference Slot
// =============================================================================

struct RefSlot {
    uintptr_t* location;  // Address of the slot holding the reference
    uintptr_t value;       // Current value in the slot
    bool isWeak;

    RefSlot() : location(nullptr), value(0), isWeak(false) {}
    RefSlot(uintptr_t* loc, uintptr_t val, bool weak = false)
        : location(loc), value(val), isWeak(weak) {}
};

// =============================================================================
// GC Object Visitor
// =============================================================================

/**
 * @brief Discovers all heap references from a JS object
 *
 * Called by the GC marker for each grey object.
 * Must enumerate every heap pointer:
 *   - Prototype chain
 *   - Named properties (inline + out-of-line)
 *   - Indexed elements
 *   - Closure scope chain
 *   - Internal slots (e.g., Promise [[Result]])
 */
class GCObjectVisitor {
public:
    using RefCallback = std::function<void(uintptr_t ref)>;
    using SlotCallback = std::function<void(uintptr_t* slot)>;
    using WeakCallback = std::function<void(uintptr_t ref)>;

    struct Callbacks {
        // Get object type
        std::function<VisitableType(uintptr_t addr)> objectType;

        // Get prototype pointer
        std::function<uintptr_t(uintptr_t addr)> prototype;

        // Get number of named property slots
        std::function<size_t(uintptr_t addr)> propertySlotCount;

        // Get property slot address
        std::function<uintptr_t*(uintptr_t addr, size_t index)> propertySlot;

        // Is property slot a heap reference?
        std::function<bool(uintptr_t addr, size_t index)> isRefSlot;

        // Get indexed element count
        std::function<size_t(uintptr_t addr)> elementCount;

        // Get indexed element slot
        std::function<uintptr_t*(uintptr_t addr, size_t index)> elementSlot;

        // Is indexed element a heap reference?
        std::function<bool(uintptr_t addr, size_t index)> isElementRef;

        // Function-specific: scope chain
        std::function<uintptr_t(uintptr_t addr)> scopeChain;

        // Map/Set-specific: iterate entries
        std::function<void(uintptr_t addr, RefCallback)> mapEntries;

        // Promise-specific: result/reactions
        std::function<void(uintptr_t addr, RefCallback)> promiseSlots;

        // Proxy-specific: target/handler
        std::function<void(uintptr_t addr, RefCallback)> proxySlots;

        // WeakRef: target (weak edge)
        std::function<uintptr_t(uintptr_t addr)> weakRefTarget;
    };

    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }

    /**
     * @brief Visit all strong references from an object
     */
    void visitObject(uintptr_t addr, RefCallback strongRef) {
        if (!cb_.objectType) return;
        VisitableType type = cb_.objectType(addr);

        // 1. Prototype
        if (cb_.prototype) {
            uintptr_t proto = cb_.prototype(addr);
            if (proto != 0) strongRef(proto);
        }

        // 2. Named properties
        visitProperties(addr, strongRef);

        // 3. Indexed elements (arrays)
        visitElements(addr, strongRef);

        // 4. Type-specific internal slots
        visitTypeSpecific(addr, type, strongRef);
    }

    /**
     * @brief Visit all references including weak ones
     */
    void visitObjectFull(uintptr_t addr,
                          RefCallback strongRef,
                          WeakCallback weakRef) {
        visitObject(addr, strongRef);

        if (!cb_.objectType) return;
        VisitableType type = cb_.objectType(addr);

        // Weak references only
        if (type == VisitableType::WeakRef && cb_.weakRefTarget) {
            uintptr_t target = cb_.weakRefTarget(addr);
            if (target != 0) weakRef(target);
        }
    }

    /**
     * @brief Visit all slot addresses (for pointer updating during compaction)
     */
    void visitSlots(uintptr_t addr, SlotCallback slotVisitor) {
        if (!cb_.propertySlotCount || !cb_.propertySlot) return;

        size_t count = cb_.propertySlotCount(addr);
        for (size_t i = 0; i < count; i++) {
            if (cb_.isRefSlot && cb_.isRefSlot(addr, i)) {
                uintptr_t* slot = cb_.propertySlot(addr, i);
                if (slot && *slot != 0) {
                    slotVisitor(slot);
                }
            }
        }

        if (cb_.elementCount && cb_.elementSlot) {
            size_t elemCount = cb_.elementCount(addr);
            for (size_t i = 0; i < elemCount; i++) {
                if (cb_.isElementRef && cb_.isElementRef(addr, i)) {
                    uintptr_t* slot = cb_.elementSlot(addr, i);
                    if (slot && *slot != 0) {
                        slotVisitor(slot);
                    }
                }
            }
        }
    }

    /**
     * @brief Count outgoing references (for heap snapshot edge count)
     */
    size_t countReferences(uintptr_t addr) {
        size_t count = 0;
        visitObject(addr, [&](uintptr_t) { count++; });
        return count;
    }

private:
    void visitProperties(uintptr_t addr, RefCallback cb) {
        if (!cb_.propertySlotCount || !cb_.propertySlot) return;

        size_t count = cb_.propertySlotCount(addr);
        for (size_t i = 0; i < count; i++) {
            if (cb_.isRefSlot && cb_.isRefSlot(addr, i)) {
                uintptr_t* slot = cb_.propertySlot(addr, i);
                if (slot && *slot != 0) {
                    cb(*slot);
                }
            }
        }
    }

    void visitElements(uintptr_t addr, RefCallback cb) {
        if (!cb_.elementCount || !cb_.elementSlot) return;

        size_t count = cb_.elementCount(addr);
        for (size_t i = 0; i < count; i++) {
            if (cb_.isElementRef && cb_.isElementRef(addr, i)) {
                uintptr_t* slot = cb_.elementSlot(addr, i);
                if (slot && *slot != 0) {
                    cb(*slot);
                }
            }
        }
    }

    void visitTypeSpecific(uintptr_t addr, VisitableType type,
                            RefCallback cb) {
        switch (type) {
            case VisitableType::Function:
            case VisitableType::AsyncFunction:
            case VisitableType::Generator:
                if (cb_.scopeChain) {
                    uintptr_t scope = cb_.scopeChain(addr);
                    if (scope != 0) cb(scope);
                }
                break;

            case VisitableType::Map:
            case VisitableType::Set:
                if (cb_.mapEntries) {
                    cb_.mapEntries(addr, cb);
                }
                break;

            case VisitableType::Promise:
                if (cb_.promiseSlots) {
                    cb_.promiseSlots(addr, cb);
                }
                break;

            case VisitableType::Proxy:
                if (cb_.proxySlots) {
                    cb_.proxySlots(addr, cb);
                }
                break;

            case VisitableType::BoundFunction:
                if (cb_.scopeChain) {
                    uintptr_t target = cb_.scopeChain(addr);
                    if (target != 0) cb(target);
                }
                break;

            default:
                break;
        }
    }

    Callbacks cb_;
};

// =============================================================================
// Root Scanner (runtime side)
// =============================================================================

/**
 * @brief Scans runtime roots for GC
 *
 * Root categories:
 * 1. VM stack frames (each frame has locals, temporaries)
 * 2. Global object and its properties
 * 3. Native handles (HandleScopeManager)
 * 4. Microtask queue
 * 5. WeakRef / FinalizationRegistry entries
 */
class RuntimeRootScanner {
public:
    using RootVisitor = std::function<void(uintptr_t addr)>;
    using SlotVisitor = std::function<void(uintptr_t* slot)>;

    struct Callbacks {
        // Iterate VM stack (locals + temporaries)
        std::function<void(RootVisitor)> scanStack;

        // Iterate global object properties
        std::function<void(RootVisitor)> scanGlobals;

        // Iterate native handle scopes
        std::function<void(SlotVisitor)> scanHandles;

        // Iterate microtask queue
        std::function<void(RootVisitor)> scanMicrotasks;

        // Iterate module registry
        std::function<void(RootVisitor)> scanModules;
    };

    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }

    /**
     * @brief Scan all roots, report to GC
     */
    void scanRoots(RootVisitor visitor) {
        size_t count = 0;

        if (cb_.scanStack) {
            cb_.scanStack([&](uintptr_t addr) {
                visitor(addr);
                count++;
            });
        }

        if (cb_.scanGlobals) {
            cb_.scanGlobals([&](uintptr_t addr) {
                visitor(addr);
                count++;
            });
        }

        if (cb_.scanHandles) {
            cb_.scanHandles([&](uintptr_t* slot) {
                if (slot && *slot != 0) {
                    visitor(*slot);
                    count++;
                }
            });
        }

        if (cb_.scanMicrotasks) {
            cb_.scanMicrotasks([&](uintptr_t addr) {
                visitor(addr);
                count++;
            });
        }

        if (cb_.scanModules) {
            cb_.scanModules([&](uintptr_t addr) {
                visitor(addr);
                count++;
            });
        }

        lastRootCount_ = count;
    }

    size_t lastRootCount() const { return lastRootCount_; }

private:
    Callbacks cb_;
    size_t lastRootCount_ = 0;
};

} // namespace Zepra::Runtime
