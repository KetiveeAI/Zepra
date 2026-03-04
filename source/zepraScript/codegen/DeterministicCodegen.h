/**
 * @file DeterministicCodegen.h
 * @brief Deterministic Code Generation
 * 
 * Guarantees:
 * - Same source → same binaries
 * - Stable relocation ordering
 * - Reproducible WAOT output
 * - Content-addressable hashing
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

namespace Zepra::Codegen {

// =============================================================================
// Deterministic Hash
// =============================================================================

/**
 * @brief Content-addressable hash for reproducibility
 */
class ContentHash {
public:
    static constexpr size_t HASH_SIZE = 32;  // SHA-256
    
    using HashBytes = std::array<uint8_t, HASH_SIZE>;
    
    // Hash arbitrary data
    static HashBytes hash(const uint8_t* data, size_t len) {
        HashBytes result{};
        
        // Simple deterministic hash (would use SHA-256 in production)
        uint64_t h1 = 0, h2 = 0, h3 = 0, h4 = 0;
        
        for (size_t i = 0; i < len; i++) {
            h1 = h1 * 31 + data[i];
            h2 = h2 * 37 + data[i];
            h3 = h3 * 41 + data[i];
            h4 = h4 * 43 + data[i];
        }
        
        // Pack into result
        for (int i = 0; i < 8; i++) {
            result[i] = (h1 >> (i * 8)) & 0xFF;
            result[8 + i] = (h2 >> (i * 8)) & 0xFF;
            result[16 + i] = (h3 >> (i * 8)) & 0xFF;
            result[24 + i] = (h4 >> (i * 8)) & 0xFF;
        }
        
        return result;
    }
    
    // Hash a vector
    template<typename T>
    static HashBytes hash(const std::vector<T>& vec) {
        return hash(reinterpret_cast<const uint8_t*>(vec.data()), 
                   vec.size() * sizeof(T));
    }
    
    // Hash to hex string
    static std::string toHex(const HashBytes& hash) {
        static const char hex[] = "0123456789abcdef";
        std::string result;
        result.reserve(HASH_SIZE * 2);
        
        for (uint8_t b : hash) {
            result.push_back(hex[b >> 4]);
            result.push_back(hex[b & 0xF]);
        }
        return result;
    }
    
    // Compare hashes
    static bool equal(const HashBytes& a, const HashBytes& b) {
        return a == b;
    }
};

// =============================================================================
// Stable Relocation Ordering
// =============================================================================

/**
 * @brief Relocation entry with stable ordering
 */
struct Relocation {
    uint32_t offset;      // Offset in code
    uint32_t symbolIndex; // Symbol being referenced
    uint8_t type;         // Relocation type
    int64_t addend;       // Addend
    
    // Deterministic comparison
    bool operator<(const Relocation& other) const {
        if (offset != other.offset) return offset < other.offset;
        if (symbolIndex != other.symbolIndex) return symbolIndex < other.symbolIndex;
        if (type != other.type) return type < other.type;
        return addend < other.addend;
    }
    
    bool operator==(const Relocation& other) const {
        return offset == other.offset &&
               symbolIndex == other.symbolIndex &&
               type == other.type &&
               addend == other.addend;
    }
};

/**
 * @brief Ensures stable relocation ordering
 */
class RelocationStabilizer {
public:
    // Add relocation
    void add(const Relocation& rel) {
        relocations_.push_back(rel);
        sorted_ = false;
    }
    
    // Get sorted relocations
    const std::vector<Relocation>& getSorted() {
        if (!sorted_) {
            std::sort(relocations_.begin(), relocations_.end());
            sorted_ = true;
        }
        return relocations_;
    }
    
    // Compute hash of relocations
    ContentHash::HashBytes hash() {
        (void)getSorted();  // Ensure sorted
        return ContentHash::hash(relocations_);
    }
    
    void clear() {
        relocations_.clear();
        sorted_ = true;
    }
    
private:
    std::vector<Relocation> relocations_;
    bool sorted_ = true;
};

// =============================================================================
// Stable Symbol Ordering
// =============================================================================

/**
 * @brief Symbol with stable ordering
 */
struct Symbol {
    std::string name;
    uint32_t index;
    uint32_t offset;
    uint32_t size;
    uint8_t type;
    uint8_t binding;
    
    bool operator<(const Symbol& other) const {
        return name < other.name;  // Alphabetical
    }
};

/**
 * @brief Ensures stable symbol ordering
 */
class SymbolStabilizer {
public:
    void add(const Symbol& sym) {
        symbols_.push_back(sym);
        sorted_ = false;
    }
    
    const std::vector<Symbol>& getSorted() {
        if (!sorted_) {
            std::sort(symbols_.begin(), symbols_.end());
            // Reassign indices after sorting
            for (size_t i = 0; i < symbols_.size(); i++) {
                symbols_[i].index = static_cast<uint32_t>(i);
            }
            sorted_ = true;
        }
        return symbols_;
    }
    
    void clear() {
        symbols_.clear();
        sorted_ = true;
    }
    
private:
    std::vector<Symbol> symbols_;
    bool sorted_ = true;
};

// =============================================================================
// Deterministic WAOT Output
// =============================================================================

/**
 * @brief WAOT module header (no timestamps)
 */
struct DeterministicAOTHeader {
    uint32_t magic = 0x5A455052;  // "ZEPR"
    uint32_t version = 1;
    uint32_t abiVersion = 1;
    uint32_t flags = 0;
    uint32_t codeSize = 0;
    uint32_t dataSize = 0;
    uint32_t relocCount = 0;
    uint32_t symbolCount = 0;
    // NO timestamp - deterministic builds
    ContentHash::HashBytes contentHash{};  // Hash of content
};

/**
 * @brief Builds deterministic AOT modules
 */
class DeterministicAOTBuilder {
public:
    // Set code
    void setCode(std::vector<uint8_t> code) {
        code_ = std::move(code);
    }
    
    // Set data
    void setData(std::vector<uint8_t> data) {
        data_ = std::move(data);
    }
    
    // Add relocation
    void addRelocation(const Relocation& rel) {
        relocations_.add(rel);
    }
    
    // Add symbol
    void addSymbol(const Symbol& sym) {
        symbols_.add(sym);
    }
    
    // Build deterministic output
    std::vector<uint8_t> build() {
        std::vector<uint8_t> output;
        
        // Ensure stable ordering
        const auto& sortedRelocs = relocations_.getSorted();
        const auto& sortedSyms = symbols_.getSorted();
        
        // Build header
        DeterministicAOTHeader header;
        header.codeSize = static_cast<uint32_t>(code_.size());
        header.dataSize = static_cast<uint32_t>(data_.size());
        header.relocCount = static_cast<uint32_t>(sortedRelocs.size());
        header.symbolCount = static_cast<uint32_t>(sortedSyms.size());
        
        // Compute content hash
        std::vector<uint8_t> allContent;
        allContent.insert(allContent.end(), code_.begin(), code_.end());
        allContent.insert(allContent.end(), data_.begin(), data_.end());
        header.contentHash = ContentHash::hash(allContent);
        
        // Serialize header
        const uint8_t* headerBytes = reinterpret_cast<const uint8_t*>(&header);
        output.insert(output.end(), headerBytes, headerBytes + sizeof(header));
        
        // Serialize code
        output.insert(output.end(), code_.begin(), code_.end());
        
        // Serialize data
        output.insert(output.end(), data_.begin(), data_.end());
        
        // Serialize relocations (already sorted)
        for (const auto& rel : sortedRelocs) {
            const uint8_t* relBytes = reinterpret_cast<const uint8_t*>(&rel);
            output.insert(output.end(), relBytes, relBytes + sizeof(rel));
        }
        
        // Serialize symbols (already sorted)
        for (const auto& sym : sortedSyms) {
            // Name length + name + fixed fields
            uint32_t nameLen = static_cast<uint32_t>(sym.name.size());
            const uint8_t* nlBytes = reinterpret_cast<const uint8_t*>(&nameLen);
            output.insert(output.end(), nlBytes, nlBytes + sizeof(nameLen));
            output.insert(output.end(), sym.name.begin(), sym.name.end());
            
            // Fixed fields
            const uint8_t* fixedBytes = reinterpret_cast<const uint8_t*>(&sym.offset);
            output.insert(output.end(), fixedBytes, fixedBytes + 12);  // offset, size, type, binding
        }
        
        return output;
    }
    
    // Get content hash
    ContentHash::HashBytes getContentHash() const {
        std::vector<uint8_t> content;
        content.insert(content.end(), code_.begin(), code_.end());
        content.insert(content.end(), data_.begin(), data_.end());
        return ContentHash::hash(content);
    }
    
private:
    std::vector<uint8_t> code_;
    std::vector<uint8_t> data_;
    RelocationStabilizer relocations_;
    SymbolStabilizer symbols_;
};

// =============================================================================
// Cache Invalidation
// =============================================================================

/**
 * @brief Cache entry with ABI-aware invalidation
 */
struct CacheKey {
    ContentHash::HashBytes sourceHash;  // Hash of source
    uint32_t abiVersion;                // ABI version
    uint32_t targetArch;                // Target architecture
    uint32_t optLevel;                  // Optimization level
    
    bool operator==(const CacheKey& other) const {
        return sourceHash == other.sourceHash &&
               abiVersion == other.abiVersion &&
               targetArch == other.targetArch &&
               optLevel == other.optLevel;
    }
};

/**
 * @brief Validates cache entries
 */
class CacheValidator {
public:
    // Check if cache entry is still valid
    static bool isValid(const CacheKey& cached, const CacheKey& current) {
        // ABI mismatch = invalid
        if (cached.abiVersion != current.abiVersion) {
            return false;
        }
        
        // Arch mismatch = invalid
        if (cached.targetArch != current.targetArch) {
            return false;
        }
        
        // Source changed = invalid
        if (cached.sourceHash != current.sourceHash) {
            return false;
        }
        
        // Opt level can differ (recompile with different opts)
        if (cached.optLevel != current.optLevel) {
            return false;
        }
        
        return true;
    }
};

} // namespace Zepra::Codegen
