/**
 * @file WasmBinary.h
 * @brief WebAssembly binary format encoder and decoder
 * 
 * Provides binary parsing and generation for WASM modules with:
 * - LEB128 variable-length integer encoding
 * - Fixed-size value encoding
 * - Section parsing
 * - Opcode handling (including prefixed opcodes)
 * 
 * Based on Firefox SpiderMonkey WasmBinary.h
 */

#pragma once

#include "WasmConstants.h"
#include "WasmValType.h"
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include <optional>
#include <stdexcept>

namespace Zepra::Wasm {

// =============================================================================
// Constants for LEB128
// =============================================================================

static constexpr size_t MaxVarU32DecodedBytes = 5;
static constexpr size_t MaxVarU64DecodedBytes = 10;
static constexpr size_t MaxVarS32DecodedBytes = 5;
static constexpr size_t MaxVarS64DecodedBytes = 10;

// =============================================================================
// Opcode Class (handles primary + prefixed opcodes)
// =============================================================================

class Opcode {
public:
    Opcode() : bits_(0) {}
    
    explicit Opcode(Op op) : bits_(static_cast<uint32_t>(op)) {}
    explicit Opcode(MiscOp op) 
        : bits_((static_cast<uint32_t>(op) << 8) | static_cast<uint32_t>(Op::MiscPrefix)) {}
    explicit Opcode(ThreadOp op)
        : bits_((static_cast<uint32_t>(op) << 8) | static_cast<uint32_t>(Op::ThreadPrefix)) {}
    explicit Opcode(SimdOp op)
        : bits_((static_cast<uint32_t>(op) << 8) | static_cast<uint32_t>(Op::SimdPrefix)) {}
    explicit Opcode(GcOp op)
        : bits_((static_cast<uint32_t>(op) << 8) | static_cast<uint32_t>(Op::GcPrefix)) {}
    
    bool isOp() const { return bits_ < static_cast<uint32_t>(Op::GcPrefix); }
    bool isMisc() const { return (bits_ & 0xFF) == static_cast<uint32_t>(Op::MiscPrefix); }
    bool isThread() const { return (bits_ & 0xFF) == static_cast<uint32_t>(Op::ThreadPrefix); }
    bool isSimd() const { return (bits_ & 0xFF) == static_cast<uint32_t>(Op::SimdPrefix); }
    bool isGc() const { return (bits_ & 0xFF) == static_cast<uint32_t>(Op::GcPrefix); }
    
    Op asOp() const { return static_cast<Op>(bits_); }
    MiscOp asMisc() const { return static_cast<MiscOp>(bits_ >> 8); }
    ThreadOp asThread() const { return static_cast<ThreadOp>(bits_ >> 8); }
    SimdOp asSimd() const { return static_cast<SimdOp>(bits_ >> 8); }
    GcOp asGc() const { return static_cast<GcOp>(bits_ >> 8); }
    
    uint32_t bits() const { return bits_; }
    
    bool operator==(const Opcode& other) const { return bits_ == other.bits_; }
    bool operator!=(const Opcode& other) const { return bits_ != other.bits_; }
    
private:
    uint32_t bits_;
};

// =============================================================================
// Bytes type (output buffer for Encoder)
// =============================================================================

using Bytes = std::vector<uint8_t>;

// =============================================================================
// Encoder Class
// =============================================================================

class Encoder {
public:
    explicit Encoder(Bytes& bytes) : bytes_(bytes) {}
    
    size_t currentOffset() const { return bytes_.size(); }
    bool empty() const { return bytes_.empty(); }
    
    // ==========================================================================
    // Fixed-size encoding
    // ==========================================================================
    
    bool writeFixedU8(uint8_t v) {
        bytes_.push_back(v);
        return true;
    }
    
    bool writeFixedU16(uint16_t v) {
        bytes_.push_back(static_cast<uint8_t>(v));
        bytes_.push_back(static_cast<uint8_t>(v >> 8));
        return true;
    }
    
    bool writeFixedU32(uint32_t v) {
        bytes_.push_back(static_cast<uint8_t>(v));
        bytes_.push_back(static_cast<uint8_t>(v >> 8));
        bytes_.push_back(static_cast<uint8_t>(v >> 16));
        bytes_.push_back(static_cast<uint8_t>(v >> 24));
        return true;
    }
    
    bool writeFixedU64(uint64_t v) {
        for (int i = 0; i < 8; i++) {
            bytes_.push_back(static_cast<uint8_t>(v >> (i * 8)));
        }
        return true;
    }
    
    bool writeFixedF32(float f) {
        uint32_t bits;
        std::memcpy(&bits, &f, sizeof(f));
        return writeFixedU32(bits);
    }
    
    bool writeFixedF64(double d) {
        uint64_t bits;
        std::memcpy(&bits, &d, sizeof(d));
        return writeFixedU64(bits);
    }
    
    // ==========================================================================
    // Variable-length LEB128 encoding
    // ==========================================================================
    
    bool writeVarU32(uint32_t v) {
        do {
            uint8_t byte = v & 0x7F;
            v >>= 7;
            if (v != 0) byte |= 0x80;
            bytes_.push_back(byte);
        } while (v != 0);
        return true;
    }
    
    bool writeVarU64(uint64_t v) {
        do {
            uint8_t byte = v & 0x7F;
            v >>= 7;
            if (v != 0) byte |= 0x80;
            bytes_.push_back(byte);
        } while (v != 0);
        return true;
    }
    
    bool writeVarS32(int32_t v) {
        bool more = true;
        while (more) {
            uint8_t byte = v & 0x7F;
            v >>= 7;
            bool signBit = (byte & 0x40) != 0;
            more = !((v == 0 && !signBit) || (v == -1 && signBit));
            if (more) byte |= 0x80;
            bytes_.push_back(byte);
        }
        return true;
    }
    
    bool writeVarS64(int64_t v) {
        bool more = true;
        while (more) {
            uint8_t byte = v & 0x7F;
            v >>= 7;
            bool signBit = (byte & 0x40) != 0;
            more = !((v == 0 && !signBit) || (v == -1 && signBit));
            if (more) byte |= 0x80;
            bytes_.push_back(byte);
        }
        return true;
    }
    
    // ==========================================================================
    // Type encoding
    // ==========================================================================
    
    bool writeValType(ValType type) {
        return writeFixedU8(static_cast<uint8_t>(type.toTypeCode()));
    }
    
    bool writeBlockType(BlockType bt) {
        switch (bt.kind()) {
            case BlockType::Kind::Void:
                return writeFixedU8(static_cast<uint8_t>(TypeCode::BlockVoid));
            case BlockType::Kind::Single:
                return writeValType(bt.singleType());
            case BlockType::Kind::TypeIndex:
                return writeVarS32(static_cast<int32_t>(bt.typeIndex()));
        }
        return false;
    }
    
    // ==========================================================================
    // Opcode encoding
    // ==========================================================================
    
    bool writeOp(Op op) {
        return writeFixedU8(static_cast<uint8_t>(op));
    }
    
    bool writeOp(Opcode opcode) {
        uint32_t bits = opcode.bits();
        if (!writeFixedU8(bits & 0xFF)) return false;
        if (opcode.isOp()) return true;
        return writeVarU32(bits >> 8);
    }
    
    // ==========================================================================
    // Bytes and strings
    // ==========================================================================
    
    bool writeBytes(const uint8_t* data, size_t len) {
        if (!writeVarU32(static_cast<uint32_t>(len))) return false;
        bytes_.insert(bytes_.end(), data, data + len);
        return true;
    }
    
    bool writeBytes(const std::vector<uint8_t>& data) {
        return writeBytes(data.data(), data.size());
    }
    
    bool writeString(const std::string& str) {
        return writeBytes(reinterpret_cast<const uint8_t*>(str.data()), str.size());
    }
    
    // ==========================================================================
    // Section handling
    // ==========================================================================
    
    bool startSection(SectionId id, size_t* offset) {
        if (!writeVarU32(static_cast<uint32_t>(id))) return false;
        *offset = currentOffset();
        // Write placeholder for section size (5 bytes max for u32)
        return writeVarU32(0);
    }
    
    void finishSection(size_t offset) {
        size_t sectionSize = currentOffset() - offset - varU32ByteLength(offset);
        patchVarU32(offset, static_cast<uint32_t>(sectionSize));
    }
    
    // ==========================================================================
    // Module header
    // ==========================================================================
    
    bool writeModuleHeader() {
        return writeFixedU32(MagicNumber) && writeFixedU32(EncodingVersion);
    }
    
private:
    size_t varU32ByteLength(size_t offset) const {
        size_t start = offset;
        while (bytes_[offset] & 0x80) offset++;
        return offset - start + 1;
    }
    
    void patchVarU32(size_t offset, uint32_t value) {
        size_t len = varU32ByteLength(offset);
        for (size_t i = 0; i < len - 1; i++) {
            bytes_[offset + i] = (value & 0x7F) | 0x80;
            value >>= 7;
        }
        bytes_[offset + len - 1] = value & 0x7F;
    }
    
    Bytes& bytes_;
};

// =============================================================================
// Decoder Class
// =============================================================================

class Decoder {
public:
    Decoder(const uint8_t* begin, const uint8_t* end)
        : begin_(begin), end_(end), cur_(begin) {}
    
    Decoder(const uint8_t* data, size_t size)
        : begin_(data), end_(data + size), cur_(data) {}
    
    // ==========================================================================
    // Position and state
    // ==========================================================================
    
    bool done() const { return cur_ >= end_; }
    size_t bytesRemain() const { return static_cast<size_t>(end_ - cur_); }
    size_t currentOffset() const { return static_cast<size_t>(cur_ - begin_); }
    const uint8_t* currentPosition() const { return cur_; }
    const uint8_t* begin() const { return begin_; }
    const uint8_t* end() const { return end_; }
    
    void rollback(const uint8_t* pos) { cur_ = pos; }
    void advance(size_t n) { cur_ += n; }
    
    // ==========================================================================
    // Peek
    // ==========================================================================
    
    bool peekByte(uint8_t* byte) const {
        if (done()) return false;
        *byte = *cur_;
        return true;
    }
    
    // ==========================================================================
    // Fixed-size reading
    // ==========================================================================
    
    bool readFixedU8(uint8_t* out) {
        if (bytesRemain() < 1) return false;
        *out = *cur_++;
        return true;
    }
    
    // Alias for readFixedU8 (compatibility)
    bool readByte(uint8_t* out) { return readFixedU8(out); }
    
    bool readFixedU16(uint16_t* out) {
        if (bytesRemain() < 2) return false;
        *out = cur_[0] | (uint16_t(cur_[1]) << 8);
        cur_ += 2;
        return true;
    }
    
    bool readFixedU32(uint32_t* out) {
        if (bytesRemain() < 4) return false;
        *out = cur_[0] | (uint32_t(cur_[1]) << 8) | 
               (uint32_t(cur_[2]) << 16) | (uint32_t(cur_[3]) << 24);
        cur_ += 4;
        return true;
    }
    
    bool readFixedU64(uint64_t* out) {
        if (bytesRemain() < 8) return false;
        *out = 0;
        for (int i = 0; i < 8; i++) {
            *out |= uint64_t(cur_[i]) << (i * 8);
        }
        cur_ += 8;
        return true;
    }
    
    bool readFixedF32(float* out) {
        uint32_t bits;
        if (!readFixedU32(&bits)) return false;
        std::memcpy(out, &bits, sizeof(float));
        return true;
    }
    
    bool readFixedF64(double* out) {
        uint64_t bits;
        if (!readFixedU64(&bits)) return false;
        std::memcpy(out, &bits, sizeof(double));
        return true;
    }
    
    // ==========================================================================
    // Variable-length LEB128 reading
    // ==========================================================================
    
    bool readVarU32(uint32_t* out) {
        uint32_t result = 0;
        uint32_t shift = 0;
        uint8_t byte;
        do {
            if (!readFixedU8(&byte)) return false;
            if (shift >= 32) return false;
            result |= uint32_t(byte & 0x7F) << shift;
            shift += 7;
        } while (byte & 0x80);
        *out = result;
        return true;
    }
    
    bool readVarU64(uint64_t* out) {
        uint64_t result = 0;
        uint32_t shift = 0;
        uint8_t byte;
        do {
            if (!readFixedU8(&byte)) return false;
            if (shift >= 64) return false;
            result |= uint64_t(byte & 0x7F) << shift;
            shift += 7;
        } while (byte & 0x80);
        *out = result;
        return true;
    }
    
    bool readVarS32(int32_t* out) {
        int32_t result = 0;
        uint32_t shift = 0;
        uint8_t byte;
        do {
            if (!readFixedU8(&byte)) return false;
            result |= int32_t(byte & 0x7F) << shift;
            shift += 7;
        } while (byte & 0x80);
        
        // Sign extend if needed
        if ((shift < 32) && (byte & 0x40)) {
            result |= (~0U << shift);
        }
        *out = result;
        return true;
    }
    
    bool readVarS64(int64_t* out) {
        int64_t result = 0;
        uint32_t shift = 0;
        uint8_t byte;
        do {
            if (!readFixedU8(&byte)) return false;
            result |= int64_t(byte & 0x7F) << shift;
            shift += 7;
        } while (byte & 0x80);
        
        // Sign extend if needed
        if ((shift < 64) && (byte & 0x40)) {
            result |= (~0ULL << shift);
        }
        *out = result;
        return true;
    }
    
    // ==========================================================================
    // Type reading
    // ==========================================================================
    
    bool readValType(ValType* out) {
        uint8_t code;
        if (!readFixedU8(&code)) return false;
        auto type = ValType::fromTypeCode(static_cast<TypeCode>(code));
        if (!type) return false;
        *out = *type;
        return true;
    }
    
    bool readBlockType(BlockType* out) {
        uint8_t byte;
        if (!peekByte(&byte)) return false;
        
        if (byte == static_cast<uint8_t>(TypeCode::BlockVoid)) {
            cur_++;
            *out = BlockType::void_();
            return true;
        }
        
        // Check for type index (negative SLEB128)
        if ((byte & 0x80) || (byte & 0x40)) {
            int32_t idx;
            if (!readVarS32(&idx)) return false;
            if (idx >= 0) {
                *out = BlockType::typeIndex(static_cast<uint32_t>(idx));
            } else {
                // Negative = inline type code
                auto type = ValType::fromTypeCode(static_cast<TypeCode>(byte));
                if (!type) return false;
                cur_++;
                *out = BlockType::single(*type);
            }
            return true;
        }
        
        // Single value type
        ValType type;
        if (!readValType(&type)) return false;
        *out = BlockType::single(type);
        return true;
    }
    
    // ==========================================================================
    // Opcode reading
    // ==========================================================================
    
    bool readOp(Opcode* out) {
        uint8_t byte;
        if (!readFixedU8(&byte)) return false;
        
        if (!isPrefixByte(byte)) {
            *out = Opcode(static_cast<Op>(byte));
            return true;
        }
        
        uint32_t ext;
        if (!readVarU32(&ext)) return false;
        
        switch (static_cast<Op>(byte)) {
            case Op::MiscPrefix:
                *out = Opcode(static_cast<MiscOp>(ext));
                break;
            case Op::ThreadPrefix:
                *out = Opcode(static_cast<ThreadOp>(ext));
                break;
            case Op::SimdPrefix:
                *out = Opcode(static_cast<SimdOp>(ext));
                break;
            case Op::GcPrefix:
                *out = Opcode(static_cast<GcOp>(ext));
                break;
            default:
                return false;
        }
        return true;
    }
    
    // ==========================================================================
    // Bytes and strings
    // ==========================================================================
    
    bool readBytes(uint32_t* len, const uint8_t** data) {
        if (!readVarU32(len)) return false;
        if (bytesRemain() < *len) return false;
        *data = cur_;
        cur_ += *len;
        return true;
    }
    
    bool readString(std::string* out) {
        uint32_t len;
        const uint8_t* data;
        if (!readBytes(&len, &data)) return false;
        out->assign(reinterpret_cast<const char*>(data), len);
        return true;
    }
    
    bool skipBytes(uint32_t len) {
        if (bytesRemain() < len) return false;
        cur_ += len;
        return true;
    }
    
    // ==========================================================================
    // Section reading
    // ==========================================================================
    
    bool readSectionHeader(SectionId* id, uint32_t* size) {
        uint8_t idByte;
        if (!readFixedU8(&idByte)) return false;
        *id = static_cast<SectionId>(idByte);
        return readVarU32(size);
    }
    
    bool readModuleHeader() {
        uint32_t magic, version;
        if (!readFixedU32(&magic)) return false;
        if (magic != MagicNumber) return false;
        if (!readFixedU32(&version)) return false;
        if (version != EncodingVersion) return false;
        return true;
    }
    
    // ==========================================================================
    // V128 (SIMD)
    // ==========================================================================
    
    bool readV128(uint8_t out[16]) {
        if (bytesRemain() < 16) return false;
        std::memcpy(out, cur_, 16);
        cur_ += 16;
        return true;
    }
    
    // ==========================================================================
    // Error handling
    // ==========================================================================
    
    std::string error_;
    
    bool fail(const char* msg) {
        error_ = msg;
        return false;
    }
    
    const std::string& error() const { return error_; }
    
private:
    const uint8_t* begin_;
    const uint8_t* end_;
    const uint8_t* cur_;
};

// =============================================================================
// ByteRange (for section bounds)
// =============================================================================

struct ByteRange {
    size_t start;
    size_t end;
    
    size_t size() const { return end - start; }
    bool contains(size_t offset) const { return offset >= start && offset < end; }
};

} // namespace Zepra::Wasm
