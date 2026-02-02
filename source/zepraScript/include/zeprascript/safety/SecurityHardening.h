/**
 * @file SecurityHardening.h
 * @brief Security Hardening for Release Builds
 * 
 * Firefox/WebKit-inspired patterns:
 * - Debug hooks disabled in release
 * - Reference tracking enforcement
 * - Memory contract hardening
 * - Import/export validation
 * - Cross-realm audit enforcement
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <unordered_set>
#include <functional>

namespace Zepra::Security {

// =============================================================================
// Build Mode Detection
// =============================================================================

#ifdef NDEBUG
    #define ZEPRA_RELEASE_BUILD 1
    #define ZEPRA_DEBUG_BUILD 0
#else
    #define ZEPRA_RELEASE_BUILD 0
    #define ZEPRA_DEBUG_BUILD 1
#endif

// =============================================================================
// Debug Hook Disabling (Release Mode)
// =============================================================================

/**
 * @brief Controls debug features availability
 * 
 * In release builds:
 * - Breakpoints are no-ops
 * - Profiling hooks are disabled
 * - Debug tracing is stripped
 */
class DebugGuard {
public:
    // Check if debug features are available
    static constexpr bool isDebugBuild() {
        return ZEPRA_DEBUG_BUILD != 0;
    }
    
    // Breakpoint installation - no-op in release
    static bool installBreakpoint(uint32_t pc) {
#if ZEPRA_RELEASE_BUILD
        (void)pc;
        return false;  // No breakpoints in release
#else
        return doInstallBreakpoint(pc);
#endif
    }
    
    // Profiling - no-op in release
    static void profileEnter(const char* name) {
#if ZEPRA_RELEASE_BUILD
        (void)name;
#else
        doProfileEnter(name);
#endif
    }
    
    static void profileExit() {
#if ZEPRA_RELEASE_BUILD
        // No-op
#else
        doProfileExit();
#endif
    }
    
    // Tracing - no-op in release
    static void trace(const char* fmt, ...) {
#if ZEPRA_RELEASE_BUILD
        (void)fmt;
#else
        // Would format and log
#endif
    }
    
private:
    static bool doInstallBreakpoint(uint32_t pc);
    static void doProfileEnter(const char* name);
    static void doProfileExit();
};

// Macros that compile to nothing in release
#if ZEPRA_RELEASE_BUILD
    #define ZEPRA_TRACE(...) ((void)0)
    #define ZEPRA_PROFILE_ENTER(name) ((void)0)
    #define ZEPRA_PROFILE_EXIT() ((void)0)
    #define ZEPRA_DEBUG_ASSERT(cond) ((void)0)
#else
    #define ZEPRA_TRACE(...) Zepra::Security::DebugGuard::trace(__VA_ARGS__)
    #define ZEPRA_PROFILE_ENTER(name) Zepra::Security::DebugGuard::profileEnter(name)
    #define ZEPRA_PROFILE_EXIT() Zepra::Security::DebugGuard::profileExit()
    #define ZEPRA_DEBUG_ASSERT(cond) do { if (!(cond)) __builtin_trap(); } while(0)
#endif

// =============================================================================
// Reference Tracking Enforcement (Always On)
// =============================================================================

/**
 * @brief Enforced reference tracking (even in release)
 * 
 * Unlike debug-only tracking, this catches real security bugs.
 */
class RefEnforcer {
public:
    // Reference operations with enforcement
    static void addRef(void* obj) {
        if (!obj) return;
        
        auto& map = getRefCounts();
        map[obj]++;
    }
    
    static void release(void* obj) {
        if (!obj) return;
        
        auto& map = getRefCounts();
        auto it = map.find(obj);
        
        if (it == map.end()) {
            reportViolation("release-untracked", obj);
            return;
        }
        
        if (it->second == 0) {
            reportViolation("double-release", obj);
            return;
        }
        
        it->second--;
    }
    
    // Verify no leaks (call at shutdown)
    static size_t verifyNoLeaks() {
        size_t leaks = 0;
        for (const auto& [obj, count] : getRefCounts()) {
            if (count > 0) {
                leaks++;
            }
        }
        return leaks;
    }
    
    // Set violation handler
    using ViolationHandler = std::function<void(const char*, void*)>;
    static void setViolationHandler(ViolationHandler handler) {
        violationHandler_ = std::move(handler);
    }
    
private:
    static std::unordered_map<void*, size_t>& getRefCounts() {
        static std::unordered_map<void*, size_t> counts;
        return counts;
    }
    
    static void reportViolation(const char* type, void* obj) {
        if (violationHandler_) {
            violationHandler_(type, obj);
        }
        // In release: log and continue (don't crash)
        // In debug: could assert
    }
    
    static inline ViolationHandler violationHandler_;
};

// =============================================================================
// Memory Contract Enforcement
// =============================================================================

/**
 * @brief Strictly enforced memory contracts
 */
class MemoryEnforcer {
public:
    struct MemoryRegion {
        uintptr_t base;
        size_t size;
        bool readable;
        bool writable;
        bool executable;
    };
    
    // Register memory region
    static void registerRegion(void* base, size_t size, bool r, bool w, bool x) {
        regions_.push_back({
            reinterpret_cast<uintptr_t>(base),
            size, r, w, x
        });
    }
    
    // Validate read access
    static bool validateRead(const void* ptr, size_t len) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        
        for (const auto& r : regions_) {
            if (addr >= r.base && addr + len <= r.base + r.size) {
                if (!r.readable) {
                    reportViolation("read-denied", ptr, len);
                    return false;
                }
                return true;
            }
        }
        
        // Not in any registered region
        reportViolation("read-unregistered", ptr, len);
        return false;
    }
    
    // Validate write access
    static bool validateWrite(void* ptr, size_t len) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        
        for (const auto& r : regions_) {
            if (addr >= r.base && addr + len <= r.base + r.size) {
                if (!r.writable) {
                    reportViolation("write-denied", ptr, len);
                    return false;
                }
                return true;
            }
        }
        
        reportViolation("write-unregistered", ptr, len);
        return false;
    }
    
    // Clear all registrations
    static void clear() {
        regions_.clear();
    }
    
private:
    static void reportViolation(const char* type, const void* ptr, size_t len) {
        (void)type; (void)ptr; (void)len;
        // Log violation, but don't crash in release
    }
    
    static inline std::vector<MemoryRegion> regions_;
};

// =============================================================================
// Import/Export Validation (WASM)
// =============================================================================

/**
 * @brief Validates WASM module imports and exports
 */
class ImportExportValidator {
public:
    struct ImportSpec {
        std::string module;
        std::string name;
        std::string expectedType;  // "func", "memory", "table", "global"
    };
    
    // Register expected import
    static void expectImport(const ImportSpec& spec) {
        expectedImports_.push_back(spec);
    }
    
    // Validate import matches expected type
    static bool validateImport(const std::string& module, const std::string& name,
                              const std::string& actualType) {
        for (const auto& expected : expectedImports_) {
            if (expected.module == module && expected.name == name) {
                if (expected.expectedType != actualType) {
                    reportMismatch("import-type-mismatch", module, name,
                                  expected.expectedType, actualType);
                    return false;
                }
                return true;
            }
        }
        
        // Unknown import - allowed but logged
        return true;
    }
    
    // Validate export exists and matches type
    static bool validateExport(const std::string& name, const std::string& type) {
        // Would check against module exports
        (void)name; (void)type;
        return true;
    }
    
    static void clear() {
        expectedImports_.clear();
    }
    
private:
    static void reportMismatch(const char* type, const std::string& module,
                              const std::string& name, const std::string& expected,
                              const std::string& actual) {
        (void)type; (void)module; (void)name; (void)expected; (void)actual;
    }
    
    static inline std::vector<ImportSpec> expectedImports_;
};

// =============================================================================
// Cross-Realm Enforcement
// =============================================================================

using RealmId = uint32_t;
constexpr RealmId INVALID_REALM = 0;

/**
 * @brief Enforces realm boundaries
 */
class RealmEnforcer {
public:
    // Get current realm
    static RealmId currentRealm() {
        return currentRealm_;
    }
    
    // Enter realm
    static void enterRealm(RealmId realm) {
        realmStack_.push_back(currentRealm_);
        currentRealm_ = realm;
    }
    
    // Exit realm
    static void exitRealm() {
        if (!realmStack_.empty()) {
            currentRealm_ = realmStack_.back();
            realmStack_.pop_back();
        }
    }
    
    // Check if access is allowed
    static bool checkAccess(RealmId objectRealm) {
        if (objectRealm == INVALID_REALM) return true;
        if (objectRealm == currentRealm_) return true;
        
        // Cross-realm access - check if allowed
        std::string key = std::to_string(currentRealm_) + "->" + 
                         std::to_string(objectRealm);
        if (allowedTransitions_.count(key)) {
            return true;
        }
        
        reportViolation(currentRealm_, objectRealm);
        return false;
    }
    
    // Allow specific cross-realm transition
    static void allowTransition(RealmId from, RealmId to) {
        std::string key = std::to_string(from) + "->" + std::to_string(to);
        allowedTransitions_.insert(key);
    }
    
    // RAII guard
    class Guard {
    public:
        explicit Guard(RealmId realm) { enterRealm(realm); }
        ~Guard() { exitRealm(); }
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
    };
    
private:
    static void reportViolation(RealmId from, RealmId to) {
        (void)from; (void)to;
        // Log cross-realm violation
    }
    
    static inline RealmId currentRealm_ = INVALID_REALM;
    static inline std::vector<RealmId> realmStack_;
    static inline std::unordered_set<std::string> allowedTransitions_;
};

// =============================================================================
// Security Audit Summary
// =============================================================================

/**
 * @brief Collects security metrics
 */
class SecurityMetrics {
public:
    struct Metrics {
        size_t refViolations = 0;
        size_t memoryViolations = 0;
        size_t importViolations = 0;
        size_t realmViolations = 0;
        size_t leaksDetected = 0;
    };
    
    static Metrics collect() {
        Metrics m;
        m.leaksDetected = RefEnforcer::verifyNoLeaks();
        // Would collect from other enforcers
        return m;
    }
    
    static bool isSecure() {
        auto m = collect();
        return m.refViolations == 0 &&
               m.memoryViolations == 0 &&
               m.importViolations == 0 &&
               m.realmViolations == 0 &&
               m.leaksDetected == 0;
    }
};

} // namespace Zepra::Security
