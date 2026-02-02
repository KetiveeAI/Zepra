/**
 * @file WasmException.h
 * @brief WebAssembly Exception Handling Support
 * 
 * Implements the WebAssembly Exception Handling proposal:
 * - Exception tags (types)
 * - Try/catch/throw semantics
 * - Stack unwinding
 */

#pragma once

#include "wasm.hpp"
#include <vector>
#include <memory>
#include <optional>

namespace Zepra::Wasm {

// =============================================================================
// Exception Value Types (Extended)
// =============================================================================

// ExnRef type code (from WASM exception handling proposal)
constexpr uint8_t VALTYPE_EXNREF = 0x69;
constexpr uint8_t VALTYPE_NULL_EXNREF = 0x74;

// =============================================================================
// Exception Tag
// =============================================================================

/**
 * @brief Exception tag definition
 * 
 * A tag defines the signature (types) of exception payloads.
 * Similar to a function signature, but for exceptions.
 */
struct ExceptionTag {
    uint32_t typeIdx;              // Index into module's type section
    std::vector<ValType> params;   // Types of exception payload values
    
    ExceptionTag() : typeIdx(0) {}
    explicit ExceptionTag(uint32_t idx) : typeIdx(idx) {}
    ExceptionTag(uint32_t idx, std::vector<ValType> p) 
        : typeIdx(idx), params(std::move(p)) {}
};

// =============================================================================
// Exception Instance
// =============================================================================

/**
 * @brief Runtime exception value
 * 
 * Represents a thrown exception with its tag and payload values.
 */
class WasmException {
public:
    WasmException(const ExceptionTag* tag, std::vector<WasmValue> payload)
        : tag_(tag), payload_(std::move(payload)) {}
    
    const ExceptionTag* tag() const { return tag_; }
    const std::vector<WasmValue>& payload() const { return payload_; }
    
    // Check if this exception matches a given tag
    bool matches(const ExceptionTag* other) const {
        return tag_ && other && tag_->typeIdx == other->typeIdx;
    }
    
private:
    const ExceptionTag* tag_;
    std::vector<WasmValue> payload_;
};

// =============================================================================
// Try Block Descriptor
// =============================================================================

/**
 * @brief Describes a try block's catch handlers
 */
struct CatchHandler {
    enum class Kind {
        Catch,      // catch <tag>
        CatchRef,   // catch_ref <tag>
        CatchAll,   // catch_all
        CatchAllRef // catch_all_ref
    };
    
    Kind kind;
    uint32_t tagIdx;       // For Catch/CatchRef only
    uint32_t labelIdx;     // Branch target
    
    static CatchHandler makeCatch(uint32_t tag, uint32_t label) {
        return {Kind::Catch, tag, label};
    }
    static CatchHandler makeCatchRef(uint32_t tag, uint32_t label) {
        return {Kind::CatchRef, tag, label};
    }
    static CatchHandler makeCatchAll(uint32_t label) {
        return {Kind::CatchAll, 0, label};
    }
    static CatchHandler makeCatchAllRef(uint32_t label) {
        return {Kind::CatchAllRef, 0, label};
    }
};

/**
 * @brief Try block information for stack unwinding
 */
struct TryBlock {
    size_t stackHeight;                    // Value stack height at try entry
    size_t controlStackHeight;             // Control stack height at try entry
    std::vector<CatchHandler> handlers;    // Catch handlers
    bool hasCatchAll;                      // Has catch_all handler
    uint32_t delegateDepth;                // For delegate instruction
    
    TryBlock() 
        : stackHeight(0), controlStackHeight(0), hasCatchAll(false), delegateDepth(0) {}
};

// =============================================================================
// Exception Frame (for unwinding)
// =============================================================================

/**
 * @brief Exception frame for stack unwinding
 */
struct ExceptionFrame {
    uint32_t funcIdx;        // Function index
    size_t pcOffset;         // Program counter offset in function body
    size_t frameBase;        // Local frame base
    std::vector<TryBlock> tryBlocks;  // Active try blocks in this frame
};

// =============================================================================
// Exception Handler Registry
// =============================================================================

/**
 * @brief Manages exception tags for a module/instance
 */
class ExceptionTagRegistry {
public:
    // Register a new tag
    uint32_t addTag(ExceptionTag tag) {
        uint32_t idx = static_cast<uint32_t>(tags_.size());
        tags_.push_back(std::move(tag));
        return idx;
    }
    
    // Get tag by index
    const ExceptionTag* getTag(uint32_t idx) const {
        if (idx >= tags_.size()) return nullptr;
        return &tags_[idx];
    }
    
    // Import a tag from another module
    uint32_t importTag(const ExceptionTag* tag) {
        importedTags_.push_back(tag);
        return static_cast<uint32_t>(importedTags_.size() - 1) | 0x80000000;
    }
    
    size_t tagCount() const { return tags_.size(); }
    
private:
    std::vector<ExceptionTag> tags_;
    std::vector<const ExceptionTag*> importedTags_;
};

// =============================================================================
// Unwind Context
// =============================================================================

/**
 * @brief Context for exception unwinding
 */
class UnwindContext {
public:
    UnwindContext() : exception_(nullptr), unwinding_(false) {}
    
    // Start unwinding with an exception
    void startUnwind(std::unique_ptr<WasmException> exn) {
        exception_ = std::move(exn);
        unwinding_ = true;
    }
    
    // Get current exception
    WasmException* currentException() const { return exception_.get(); }
    
    // Check if unwinding
    bool isUnwinding() const { return unwinding_; }
    
    // Stop unwinding (exception caught)
    std::unique_ptr<WasmException> stopUnwind() {
        unwinding_ = false;
        return std::move(exception_);
    }
    
    // Clear without catching (rethrow)
    void clearException() {
        exception_.reset();
        unwinding_ = false;
    }
    
private:
    std::unique_ptr<WasmException> exception_;
    bool unwinding_;
};

// =============================================================================
// Exception Handling Opcodes
// =============================================================================

namespace ExceptionOp {
    constexpr uint8_t TRY = 0x06;
    constexpr uint8_t CATCH = 0x07;
    constexpr uint8_t THROW = 0x08;
    constexpr uint8_t RETHROW = 0x09;
    constexpr uint8_t THROW_REF = 0x0A;
    constexpr uint8_t DELEGATE = 0x18;
    constexpr uint8_t CATCH_ALL = 0x19;
    constexpr uint8_t TRY_TABLE = 0x1F;
}

// =============================================================================
// Section ID for Tags
// =============================================================================

constexpr uint8_t SECTION_TAG = 13;

// =============================================================================
// Exception Handling Helpers
// =============================================================================

/**
 * @brief Find matching catch handler for exception
 */
inline std::optional<const CatchHandler*> findCatchHandler(
    const TryBlock& tryBlock, 
    const WasmException* exn,
    const ExceptionTagRegistry& registry) 
{
    for (const auto& handler : tryBlock.handlers) {
        switch (handler.kind) {
            case CatchHandler::Kind::Catch:
            case CatchHandler::Kind::CatchRef: {
                const ExceptionTag* handlerTag = registry.getTag(handler.tagIdx);
                if (exn->matches(handlerTag)) {
                    return &handler;
                }
                break;
            }
            case CatchHandler::Kind::CatchAll:
            case CatchHandler::Kind::CatchAllRef:
                return &handler;
        }
    }
    return std::nullopt;
}

} // namespace Zepra::Wasm
