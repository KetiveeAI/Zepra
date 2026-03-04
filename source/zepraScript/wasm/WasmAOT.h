/**
 * @file WasmAOT.h
 * @brief Ahead-of-Time Compilation and Code Serialization
 * 
 * Implements:
 * - CompiledCode serialization format
 * - Relocation table for position-independent code
 * - Platform metadata
 * - Code signature for cache validation
 */

#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <cstring>

namespace Zepra::Wasm {

// =============================================================================
// Platform Detection
// =============================================================================

enum class Platform : uint8_t {
    Unknown = 0,
    X64_Linux = 1,
    X64_Windows = 2,
    X64_macOS = 3,
    ARM64_Linux = 4,
    ARM64_macOS = 5,
    ARM64_Windows = 6
};

inline Platform detectPlatform() {
#if defined(__x86_64__) || defined(_M_X64)
    #if defined(__linux__)
        return Platform::X64_Linux;
    #elif defined(_WIN32)
        return Platform::X64_Windows;
    #elif defined(__APPLE__)
        return Platform::X64_macOS;
    #endif
#elif defined(__aarch64__) || defined(_M_ARM64)
    #if defined(__linux__)
        return Platform::ARM64_Linux;
    #elif defined(__APPLE__)
        return Platform::ARM64_macOS;
    #elif defined(_WIN32)
        return Platform::ARM64_Windows;
    #endif
#endif
    return Platform::Unknown;
}

// =============================================================================
// Relocation Types
// =============================================================================

enum class RelocationType : uint8_t {
    Absolute64 = 0,      // 64-bit absolute address
    Relative32 = 1,      // 32-bit PC-relative
    FunctionIndex = 2,   // Function table index
    TableIndex = 3,      // Table slot index
    MemoryBase = 4,      // Linear memory base pointer
    GlobalBase = 5,      // Global data base pointer
    ImportSlot = 6,      // Import slot pointer
    StackLimit = 7       // Stack limit check address
};

/**
 * @brief Relocation entry
 */
struct Relocation {
    uint32_t offset;         // Offset in code section
    RelocationType type;
    uint32_t symbolIndex;    // Index into symbol table
    int64_t addend;          // Addend for relocation calculation
};

// =============================================================================
// Compiled Code Section
// =============================================================================

/**
 * @brief Compiled function metadata
 */
struct CompiledFunction {
    uint32_t funcIndex;
    uint32_t codeOffset;     // Offset into code buffer
    uint32_t codeSize;
    uint32_t stackSize;
    uint32_t localsSize;
    
    // Debug info (optional)
    std::string name;
    std::vector<uint32_t> lineNumberTable;  // PC -> line number
};

/**
 * @brief Serialized code section
 */
struct CodeSection {
    std::vector<uint8_t> code;
    std::vector<Relocation> relocations;
    std::vector<CompiledFunction> functions;
};

// =============================================================================
// AOT Module Format
// =============================================================================

/**
 * @brief AOT module header
 */
struct AOTHeader {
    static constexpr uint32_t MAGIC = 0x544F4157;  // "WAOT"
    static constexpr uint32_t VERSION = 1;
    
    uint32_t magic = MAGIC;
    uint32_t version = VERSION;
    Platform platform;
    uint8_t reserved[3] = {0, 0, 0};
    
    // Module identity
    uint8_t moduleHash[32];       // SHA-256 of original WASM module
    uint64_t timestamp;
    
    // Section offsets
    uint32_t codeSectionOffset;
    uint32_t codeSectionSize;
    uint32_t dataSectionOffset;
    uint32_t dataSectionSize;
    uint32_t relocSectionOffset;
    uint32_t relocSectionSize;
    uint32_t metadataSectionOffset;
    uint32_t metadataSectionSize;
};

/**
 * @brief Serialized AOT module
 */
class AOTModule {
public:
    AOTHeader header;
    CodeSection code;
    std::vector<uint8_t> dataSegments;
    std::string metadata;  // JSON metadata
    
    // Serialize to bytes
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        
        // Calculate section sizes
        size_t codeSize = code.code.size();
        size_t relocSize = code.relocations.size() * sizeof(Relocation);
        size_t dataSize = dataSegments.size();
        size_t metaSize = metadata.size();
        
        // Reserve space
        size_t totalSize = sizeof(AOTHeader) + codeSize + relocSize + dataSize + metaSize;
        buffer.reserve(totalSize);
        
        // Write header
        AOTHeader h = header;
        h.codeSectionOffset = sizeof(AOTHeader);
        h.codeSectionSize = static_cast<uint32_t>(codeSize);
        h.relocSectionOffset = h.codeSectionOffset + h.codeSectionSize;
        h.relocSectionSize = static_cast<uint32_t>(relocSize);
        h.dataSectionOffset = h.relocSectionOffset + h.relocSectionSize;
        h.dataSectionSize = static_cast<uint32_t>(dataSize);
        h.metadataSectionOffset = h.dataSectionOffset + h.dataSectionSize;
        h.metadataSectionSize = static_cast<uint32_t>(metaSize);
        
        buffer.resize(sizeof(AOTHeader));
        std::memcpy(buffer.data(), &h, sizeof(AOTHeader));
        
        // Write sections
        buffer.insert(buffer.end(), code.code.begin(), code.code.end());
        
        for (const auto& reloc : code.relocations) {
            const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&reloc);
            buffer.insert(buffer.end(), ptr, ptr + sizeof(Relocation));
        }
        
        buffer.insert(buffer.end(), dataSegments.begin(), dataSegments.end());
        buffer.insert(buffer.end(), metadata.begin(), metadata.end());
        
        return buffer;
    }
    
    // Deserialize from bytes
    static std::unique_ptr<AOTModule> deserialize(const std::vector<uint8_t>& buffer) {
        if (buffer.size() < sizeof(AOTHeader)) {
            return nullptr;
        }
        
        auto module = std::make_unique<AOTModule>();
        std::memcpy(&module->header, buffer.data(), sizeof(AOTHeader));
        
        // Validate magic and version
        if (module->header.magic != AOTHeader::MAGIC ||
            module->header.version != AOTHeader::VERSION) {
            return nullptr;
        }
        
        // Validate platform
        if (module->header.platform != detectPlatform()) {
            return nullptr;
        }
        
        // Read code section
        if (module->header.codeSectionOffset + module->header.codeSectionSize <= buffer.size()) {
            module->code.code.assign(
                buffer.begin() + module->header.codeSectionOffset,
                buffer.begin() + module->header.codeSectionOffset + module->header.codeSectionSize
            );
        }
        
        // Read relocations
        size_t numRelocs = module->header.relocSectionSize / sizeof(Relocation);
        module->code.relocations.resize(numRelocs);
        if (numRelocs > 0) {
            std::memcpy(module->code.relocations.data(),
                       buffer.data() + module->header.relocSectionOffset,
                       module->header.relocSectionSize);
        }
        
        // Read data segments
        if (module->header.dataSectionSize > 0) {
            module->dataSegments.assign(
                buffer.begin() + module->header.dataSectionOffset,
                buffer.begin() + module->header.dataSectionOffset + module->header.dataSectionSize
            );
        }
        
        // Read metadata
        if (module->header.metadataSectionSize > 0) {
            module->metadata.assign(
                buffer.begin() + module->header.metadataSectionOffset,
                buffer.begin() + module->header.metadataSectionOffset + module->header.metadataSectionSize
            );
        }
        
        return module;
    }
};

// =============================================================================
// Code Linker (applies relocations)
// =============================================================================

/**
 * @brief Applies relocations to load AOT code
 */
class AOTLinker {
public:
    struct SymbolTable {
        std::vector<uintptr_t> functions;   // Function entry points
        std::vector<uintptr_t> tables;      // Table base pointers
        uintptr_t memoryBase = 0;
        uintptr_t globalBase = 0;
        std::vector<uintptr_t> imports;     // Import slots
        uintptr_t stackLimit = 0;
    };
    
    // Link AOT module with runtime addresses
    static bool link(AOTModule& module, uint8_t* codeBase, const SymbolTable& symbols) {
        for (const auto& reloc : module.code.relocations) {
            uintptr_t value = 0;
            
            switch (reloc.type) {
                case RelocationType::Absolute64:
                    value = symbols.functions[reloc.symbolIndex];
                    break;
                case RelocationType::FunctionIndex:
                    value = symbols.functions[reloc.symbolIndex];
                    break;
                case RelocationType::TableIndex:
                    value = symbols.tables[reloc.symbolIndex];
                    break;
                case RelocationType::MemoryBase:
                    value = symbols.memoryBase;
                    break;
                case RelocationType::GlobalBase:
                    value = symbols.globalBase;
                    break;
                case RelocationType::ImportSlot:
                    value = symbols.imports[reloc.symbolIndex];
                    break;
                case RelocationType::StackLimit:
                    value = symbols.stackLimit;
                    break;
                case RelocationType::Relative32: {
                    // PC-relative relocation
                    uintptr_t target = symbols.functions[reloc.symbolIndex];
                    uintptr_t pc = reinterpret_cast<uintptr_t>(codeBase + reloc.offset + 4);
                    int32_t rel = static_cast<int32_t>(target - pc + reloc.addend);
                    std::memcpy(codeBase + reloc.offset, &rel, 4);
                    continue;
                }
            }
            
            value += reloc.addend;
            std::memcpy(codeBase + reloc.offset, &value, sizeof(uintptr_t));
        }
        
        return true;
    }
};

// =============================================================================
// Module Hasher (for cache lookup)
// =============================================================================

/**
 * @brief Compute hash of WASM module for cache key
 */
class ModuleHasher {
public:
    // Simple hash (in production, use SHA-256)
    static void hash(const uint8_t* data, size_t size, uint8_t out[32]) {
        // FNV-1a hash, padded to 32 bytes
        uint64_t h = 14695981039346656037ULL;
        for (size_t i = 0; i < size; i++) {
            h ^= data[i];
            h *= 1099511628211ULL;
        }
        
        std::memset(out, 0, 32);
        std::memcpy(out, &h, 8);
        std::memcpy(out + 8, &size, 8);
    }
    
    static bool compareHash(const uint8_t a[32], const uint8_t b[32]) {
        return std::memcmp(a, b, 32) == 0;
    }
};

} // namespace Zepra::Wasm
