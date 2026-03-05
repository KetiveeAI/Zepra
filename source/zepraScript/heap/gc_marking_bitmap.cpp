// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_marking_bitmap.cpp — Bitmap-based mark state for GC

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <functional>

namespace Zepra::Heap {

// One bit per allocation granule (8 bytes). For a 256KB page, the
// bitmap is 256KB / 8 / 8 = 4KB — fits in a single OS page.

class MarkingBitmap {
public:
    static constexpr size_t GRANULE_SHIFT = 3;   // 8-byte granules
    static constexpr size_t GRANULE_SIZE = 1 << GRANULE_SHIFT;
    static constexpr size_t BITS_PER_WORD = 64;

    MarkingBitmap() : words_(nullptr), wordCount_(0), heapBase_(0) {}

    ~MarkingBitmap() { destroy(); }

    bool initialize(uintptr_t heapBase, size_t heapSize) {
        heapBase_ = heapBase;
        size_t granules = heapSize / GRANULE_SIZE;
        wordCount_ = (granules + BITS_PER_WORD - 1) / BITS_PER_WORD;
        words_ = new(std::nothrow) std::atomic<uint64_t>[wordCount_];
        if (!words_) return false;
        clearAll();
        return true;
    }

    void destroy() {
        delete[] words_;
        words_ = nullptr;
        wordCount_ = 0;
    }

    // Set mark bit. Returns true if it was previously clear (new mark).
    bool mark(uintptr_t addr) {
        auto [wordIdx, bitIdx] = indices(addr);
        if (wordIdx >= wordCount_) return false;
        uint64_t mask = uint64_t(1) << bitIdx;
        uint64_t old = words_[wordIdx].fetch_or(mask, std::memory_order_relaxed);
        return (old & mask) == 0;
    }

    bool isMarked(uintptr_t addr) const {
        auto [wordIdx, bitIdx] = indices(addr);
        if (wordIdx >= wordCount_) return false;
        return (words_[wordIdx].load(std::memory_order_relaxed) >> bitIdx) & 1;
    }

    void clearMark(uintptr_t addr) {
        auto [wordIdx, bitIdx] = indices(addr);
        if (wordIdx >= wordCount_) return;
        uint64_t mask = ~(uint64_t(1) << bitIdx);
        words_[wordIdx].fetch_and(mask, std::memory_order_relaxed);
    }

    void clearAll() {
        for (size_t i = 0; i < wordCount_; i++) {
            words_[i].store(0, std::memory_order_relaxed);
        }
    }

    // Count live objects in address range.
    size_t countMarked(uintptr_t start, uintptr_t end) const {
        size_t count = 0;
        for (uintptr_t addr = start; addr < end; addr += GRANULE_SIZE) {
            if (isMarked(addr)) count++;
        }
        return count;
    }

    // Iterate marked addresses in range.
    void forEachMarked(uintptr_t start, uintptr_t end,
                        std::function<void(uintptr_t)> fn) const {
        auto [startWord, startBit] = indices(start);
        auto [endWord, endBit] = indices(end);

        for (size_t w = startWord; w <= std::min(endWord, wordCount_ - 1); w++) {
            uint64_t word = words_[w].load(std::memory_order_relaxed);
            if (word == 0) continue;

            size_t bitStart = (w == startWord) ? startBit : 0;
            size_t bitEnd = (w == endWord) ? endBit : BITS_PER_WORD;

            for (size_t b = bitStart; b < bitEnd; b++) {
                if ((word >> b) & 1) {
                    uintptr_t addr = heapBase_ +
                        (w * BITS_PER_WORD + b) * GRANULE_SIZE;
                    fn(addr);
                }
            }
        }
    }

    size_t memoryUsage() const { return wordCount_ * sizeof(std::atomic<uint64_t>); }

private:
    std::pair<size_t, size_t> indices(uintptr_t addr) const {
        size_t granule = (addr - heapBase_) >> GRANULE_SHIFT;
        return {granule / BITS_PER_WORD, granule % BITS_PER_WORD};
    }

    std::atomic<uint64_t>* words_;
    size_t wordCount_;
    uintptr_t heapBase_;
};

} // namespace Zepra::Heap
