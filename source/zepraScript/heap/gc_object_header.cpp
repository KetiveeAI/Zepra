// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_object_header.cpp — Object header word layout and accessors

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace Zepra::Heap {

// Every heap object starts with an 8-byte header word.
// Layout (64-bit):
//   [63:32] shapeId       (32 bits — inline cache key)
//   [31:16] sizeClass     (16 bits — size in 8-byte granules, max 512KB)
//   [15:8]  typeTag        (8 bits — ObjectType enum)
//   [7:4]   age            (4 bits — 0–15 scavenge age)
//   [3]     forwarded      (1 bit — has forwarding pointer)
//   [2]     pinned         (1 bit — cannot be moved)
//   [1]     marked         (1 bit — GC mark bit)
//   [0]     remembered     (1 bit — in remembered set)

class ObjectHeader {
public:
    static constexpr uint64_t REMEMBERED_BIT = 1ULL << 0;
    static constexpr uint64_t MARKED_BIT     = 1ULL << 1;
    static constexpr uint64_t PINNED_BIT     = 1ULL << 2;
    static constexpr uint64_t FORWARDED_BIT  = 1ULL << 3;
    static constexpr uint64_t AGE_SHIFT      = 4;
    static constexpr uint64_t AGE_MASK       = 0xFULL << AGE_SHIFT;
    static constexpr uint64_t TYPE_SHIFT     = 8;
    static constexpr uint64_t TYPE_MASK      = 0xFFULL << TYPE_SHIFT;
    static constexpr uint64_t SIZE_SHIFT     = 16;
    static constexpr uint64_t SIZE_MASK      = 0xFFFFULL << SIZE_SHIFT;
    static constexpr uint64_t SHAPE_SHIFT    = 32;
    static constexpr uint64_t SHAPE_MASK     = 0xFFFFFFFFULL << SHAPE_SHIFT;

    explicit ObjectHeader(uint64_t raw = 0) : raw_(raw) {}

    // Encode a new header.
    static ObjectHeader create(uint32_t shapeId, uint16_t sizeGranules,
                                uint8_t typeTag, uint8_t age = 0) {
        uint64_t raw = 0;
        raw |= (static_cast<uint64_t>(shapeId) << SHAPE_SHIFT);
        raw |= (static_cast<uint64_t>(sizeGranules) << SIZE_SHIFT);
        raw |= (static_cast<uint64_t>(typeTag) << TYPE_SHIFT);
        raw |= (static_cast<uint64_t>(age & 0xF) << AGE_SHIFT);
        return ObjectHeader(raw);
    }

    uint32_t shapeId() const { return static_cast<uint32_t>((raw_ & SHAPE_MASK) >> SHAPE_SHIFT); }
    uint16_t sizeGranules() const { return static_cast<uint16_t>((raw_ & SIZE_MASK) >> SIZE_SHIFT); }
    size_t sizeBytes() const { return static_cast<size_t>(sizeGranules()) * 8; }
    uint8_t typeTag() const { return static_cast<uint8_t>((raw_ & TYPE_MASK) >> TYPE_SHIFT); }
    uint8_t age() const { return static_cast<uint8_t>((raw_ & AGE_MASK) >> AGE_SHIFT); }

    bool isMarked() const { return raw_ & MARKED_BIT; }
    bool isPinned() const { return raw_ & PINNED_BIT; }
    bool isForwarded() const { return raw_ & FORWARDED_BIT; }
    bool isRemembered() const { return raw_ & REMEMBERED_BIT; }

    void setMarked() { raw_ |= MARKED_BIT; }
    void clearMarked() { raw_ &= ~MARKED_BIT; }
    void setPinned() { raw_ |= PINNED_BIT; }
    void clearPinned() { raw_ &= ~PINNED_BIT; }
    void setForwarded() { raw_ |= FORWARDED_BIT; }
    void setRemembered() { raw_ |= REMEMBERED_BIT; }
    void clearRemembered() { raw_ &= ~REMEMBERED_BIT; }

    void incrementAge() {
        uint8_t a = age();
        if (a < 15) {
            raw_ = (raw_ & ~AGE_MASK) | (static_cast<uint64_t>(a + 1) << AGE_SHIFT);
        }
    }

    void setShapeId(uint32_t id) {
        raw_ = (raw_ & ~SHAPE_MASK) | (static_cast<uint64_t>(id) << SHAPE_SHIFT);
    }

    uint64_t raw() const { return raw_; }

    // Atomic operations for concurrent GC.
    static bool atomicMark(std::atomic<uint64_t>* header) {
        uint64_t old = header->load(std::memory_order_relaxed);
        while (!(old & MARKED_BIT)) {
            if (header->compare_exchange_weak(old, old | MARKED_BIT,
                    std::memory_order_acq_rel)) {
                return true;
            }
        }
        return false;
    }

    static void atomicClearMark(std::atomic<uint64_t>* header) {
        header->fetch_and(~MARKED_BIT, std::memory_order_relaxed);
    }

private:
    uint64_t raw_;
};

} // namespace Zepra::Heap
