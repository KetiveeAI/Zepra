/**
 * @file BytecodeSerialization.h
 * @brief Bytecode serialization and snapshot support
 * 
 * Implements:
 * - Binary bytecode format
 * - Snapshot creation/restoration
 * - Code caching
 * - Versioned serialization
 * 
 * Based on V8 snapshot and JSC bytecode caching
 */

#pragma once

#include "OpcodeReference.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <cstdio>

namespace Zepra::Bytecode {

// =============================================================================
// Bytecode File Format
// =============================================================================

// Magic number: "ZEPB" (Zepra Bytecode)
constexpr uint32_t BYTECODE_MAGIC = 0x5A455042;

// Current format version
constexpr uint16_t BYTECODE_VERSION_MAJOR = 1;
constexpr uint16_t BYTECODE_VERSION_MINOR = 0;

struct BytecodeHeader {
    uint32_t magic;
    uint16_t versionMajor;
    uint16_t versionMinor;
    uint32_t flags;
    uint32_t sourceHash;        // Hash of source for cache validation
    uint32_t functionCount;
    uint32_t stringTableOffset;
    uint32_t constantPoolOffset;
    uint32_t functionTableOffset;
    uint32_t totalSize;
};

enum class BytecodeFlags : uint32_t {
    None            = 0,
    Strict          = 1 << 0,
    Module          = 1 << 1,
    HasAsync        = 1 << 2,
    HasGenerator    = 1 << 3,
    Compressed      = 1 << 4,
    DebugInfo       = 1 << 5
};

// =============================================================================
// Function Record
// =============================================================================

struct FunctionRecord {
    uint32_t nameIndex;         // Index in string table
    uint32_t bytecodeOffset;    // Offset to bytecode
    uint32_t bytecodeLength;    // Bytecode size
    uint16_t paramCount;
    uint16_t localCount;
    uint16_t upvalueCount;
    uint16_t flags;             // Async, generator, arrow, etc.
    uint32_t sourceStart;       // Source location
    uint32_t sourceEnd;
};

// =============================================================================
// Constant Pool Entry
// =============================================================================

enum class ConstantType : uint8_t {
    Null,
    Undefined,
    Boolean,
    Int32,
    Int64,
    Double,
    String,
    BigInt,
    Function,
    Regex,
    Object,
    Array
};

struct ConstantEntry {
    ConstantType type;
    uint32_t dataOffset;        // Offset to data
    uint32_t dataSize;          // Size of data
};

// =============================================================================
// Serializer
// =============================================================================

class BytecodeSerializer {
public:
    BytecodeSerializer() = default;
    
    void beginModule(const std::string& sourceHash, uint32_t flags);
    void addString(const std::string& str);
    void addConstant(ConstantType type, const void* data, size_t size);
    void addFunction(const FunctionRecord& func, const std::vector<uint8_t>& bytecode);
    void addDebugInfo(uint32_t funcIndex, const std::vector<uint32_t>& lineTable);
    
    std::vector<uint8_t> finalize();
    
    bool writeToFile(const std::string& path);
    
private:
    BytecodeHeader header_{};
    std::vector<std::string> strings_;
    std::vector<std::pair<ConstantType, std::vector<uint8_t>>> constants_;
    std::vector<std::pair<FunctionRecord, std::vector<uint8_t>>> functions_;
    std::vector<std::vector<uint32_t>> debugInfo_;
    
    void writeHeader(std::vector<uint8_t>& out);
    void writeStringTable(std::vector<uint8_t>& out);
    void writeConstantPool(std::vector<uint8_t>& out);
    void writeFunctionTable(std::vector<uint8_t>& out);
    
    void write8(std::vector<uint8_t>& out, uint8_t v) { out.push_back(v); }
    void write16(std::vector<uint8_t>& out, uint16_t v);
    void write32(std::vector<uint8_t>& out, uint32_t v);
    void writeBytes(std::vector<uint8_t>& out, const void* data, size_t size);
};

// =============================================================================
// Deserializer
// =============================================================================

class BytecodeDeserializer {
public:
    struct LoadResult {
        bool success = false;
        std::string error;
        BytecodeHeader header;
        std::vector<std::string> strings;
        std::vector<std::pair<ConstantType, std::vector<uint8_t>>> constants;
        std::vector<std::pair<FunctionRecord, std::vector<uint8_t>>> functions;
    };
    
    static LoadResult load(const std::vector<uint8_t>& data);
    static LoadResult loadFromFile(const std::string& path);
    
    // Check if cached bytecode is valid for source
    static bool validateCache(const std::vector<uint8_t>& cache, 
                              const std::string& sourceHash);
    
private:
    static bool validateHeader(const BytecodeHeader& header);
    static uint16_t read16(const uint8_t* p);
    static uint32_t read32(const uint8_t* p);
};

// =============================================================================
// Code Cache
// =============================================================================

class CodeCache {
public:
    explicit CodeCache(const std::string& cacheDir);
    
    // Store compiled bytecode for source
    bool store(const std::string& sourceId, 
               const std::string& sourceHash,
               const std::vector<uint8_t>& bytecode);
    
    // Retrieve cached bytecode
    std::vector<uint8_t> retrieve(const std::string& sourceId,
                                   const std::string& sourceHash);
    
    // Invalidate cache for source
    void invalidate(const std::string& sourceId);
    
    // Clear entire cache
    void clear();
    
    // Cache statistics
    struct Stats {
        size_t hits = 0;
        size_t misses = 0;
        size_t bytes = 0;
        size_t entries = 0;
    };
    
    const Stats& stats() const { return stats_; }
    
private:
    std::string cacheDir_;
    std::map<std::string, std::string> index_;  // sourceId -> cacheFile
    Stats stats_;
    
    std::string getCachePath(const std::string& sourceId);
    void loadIndex();
    void saveIndex();
};

// =============================================================================
// Snapshot Support
// =============================================================================

/**
 * @brief Creates startup snapshots for fast initialization
 */
class SnapshotCreator {
public:
    SnapshotCreator() = default;
    
    // Add built-in functions
    void addBuiltin(const std::string& name, const std::vector<uint8_t>& bytecode);
    
    // Add global constants
    void addGlobalConstant(const std::string& name, ConstantType type, 
                           const void* data, size_t size);
    
    // Create snapshot blob
    std::vector<uint8_t> createSnapshot();
    
private:
    std::vector<std::pair<std::string, std::vector<uint8_t>>> builtins_;
    std::vector<std::tuple<std::string, ConstantType, std::vector<uint8_t>>> globals_;
};

/**
 * @brief Restores from snapshot
 */
class SnapshotRestorer {
public:
    struct RestoreResult {
        bool success;
        std::map<std::string, std::vector<uint8_t>> builtins;
        std::map<std::string, std::pair<ConstantType, std::vector<uint8_t>>> globals;
    };
    
    static RestoreResult restore(const std::vector<uint8_t>& snapshot);
};

} // namespace Zepra::Bytecode
