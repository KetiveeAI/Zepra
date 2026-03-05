// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_card_table.cpp — Card table for generational remembered sets

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <functional>

namespace Zepra::Heap {

// One byte per 512-byte card. A dirty card means the card may contain
// old→young pointers and must be scanned during minor GC.

class CardTable {
public:
    static constexpr size_t CARD_SHIFT = 9;
    static constexpr size_t CARD_SIZE = 1 << CARD_SHIFT;  // 512 bytes

    enum CardState : uint8_t {
        Clean = 0,
        Dirty = 1,
        Scanned = 2,    // Dirty but already scanned this cycle
        Young = 3       // Card is in nursery (skip)
    };

    CardTable() : cards_(nullptr), cardCount_(0), heapBase_(0) {}
    ~CardTable() { destroy(); }

    bool initialize(uintptr_t heapBase, size_t heapSize) {
        heapBase_ = heapBase;
        cardCount_ = heapSize / CARD_SIZE;
        cards_ = new(std::nothrow) std::atomic<uint8_t>[cardCount_];
        if (!cards_) return false;
        clearAll();
        return true;
    }

    void destroy() {
        delete[] cards_;
        cards_ = nullptr;
        cardCount_ = 0;
    }

    void dirtyCard(uintptr_t addr) {
        size_t idx = cardIndex(addr);
        if (idx < cardCount_) {
            cards_[idx].store(Dirty, std::memory_order_relaxed);
        }
    }

    void cleanCard(uintptr_t addr) {
        size_t idx = cardIndex(addr);
        if (idx < cardCount_) {
            cards_[idx].store(Clean, std::memory_order_relaxed);
        }
    }

    bool isDirty(uintptr_t addr) const {
        size_t idx = cardIndex(addr);
        if (idx >= cardCount_) return false;
        return cards_[idx].load(std::memory_order_relaxed) == Dirty;
    }

    void markScanned(uintptr_t addr) {
        size_t idx = cardIndex(addr);
        if (idx < cardCount_) {
            cards_[idx].store(Scanned, std::memory_order_relaxed);
        }
    }

    void clearAll() {
        for (size_t i = 0; i < cardCount_; i++) {
            cards_[i].store(Clean, std::memory_order_relaxed);
        }
    }

    // Reset scanned cards back to clean after GC.
    void resetScanned() {
        for (size_t i = 0; i < cardCount_; i++) {
            uint8_t v = cards_[i].load(std::memory_order_relaxed);
            if (v == Scanned) {
                cards_[i].store(Clean, std::memory_order_relaxed);
            }
        }
    }

    // Iterate dirty cards in address range, calling visitor with card base address.
    size_t forEachDirtyCard(uintptr_t start, uintptr_t end,
                             std::function<void(uintptr_t cardBase)> visitor) {
        size_t startIdx = cardIndex(start);
        size_t endIdx = cardIndex(end);
        if (endIdx > cardCount_) endIdx = cardCount_;
        size_t count = 0;

        for (size_t i = startIdx; i < endIdx; i++) {
            if (cards_[i].load(std::memory_order_relaxed) == Dirty) {
                uintptr_t base = heapBase_ + i * CARD_SIZE;
                visitor(base);
                cards_[i].store(Scanned, std::memory_order_relaxed);
                count++;
            }
        }
        return count;
    }

    size_t dirtyCount() const {
        size_t count = 0;
        for (size_t i = 0; i < cardCount_; i++) {
            if (cards_[i].load(std::memory_order_relaxed) == Dirty) count++;
        }
        return count;
    }

    size_t memoryUsage() const { return cardCount_ * sizeof(std::atomic<uint8_t>); }
    uintptr_t cardBaseAddress(uintptr_t addr) const {
        return heapBase_ + cardIndex(addr) * CARD_SIZE;
    }

private:
    size_t cardIndex(uintptr_t addr) const {
        return (addr - heapBase_) >> CARD_SHIFT;
    }

    std::atomic<uint8_t>* cards_;
    size_t cardCount_;
    uintptr_t heapBase_;
};

} // namespace Zepra::Heap
