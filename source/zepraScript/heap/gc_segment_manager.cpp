// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_segment_manager.cpp — mmap-based heap segment allocation

#include <atomic>
#include <mutex>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <algorithm>

#ifdef __linux__
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace Zepra::Heap {

// A segment is a contiguous mmap'd region (typically 256KB–1MB).
// The segment manager tracks all OS-level allocations and handles
// commit/decommit for memory pressure response.

struct Segment {
    uintptr_t base;
    size_t size;
    size_t committed;
    bool inUse;
    uint64_t id;

    Segment() : base(0), size(0), committed(0), inUse(false), id(0) {}
};

class SegmentManager {
public:
    static constexpr size_t DEFAULT_SEGMENT = 256 * 1024;
    static constexpr size_t PAGE_SIZE = 4096;

    SegmentManager() = default;

    ~SegmentManager() { releaseAll(); }

    // Allocate a new segment via mmap.
    Segment* allocateSegment(size_t size) {
        size = alignUp(size, PAGE_SIZE);

#ifdef __linux__
        void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED) return nullptr;
#else
        void* ptr = std::malloc(size);
        if (!ptr) return nullptr;
#endif

        std::lock_guard<std::mutex> lock(mutex_);
        Segment seg;
        seg.base = reinterpret_cast<uintptr_t>(ptr);
        seg.size = size;
        seg.committed = size;
        seg.inUse = true;
        seg.id = nextId_++;
        segments_.push_back(seg);

        totalMapped_ += size;
        totalCommitted_ += size;

        return &segments_.back();
    }

    // Return a segment to the OS.
    void freeSegment(uint64_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& seg : segments_) {
            if (seg.id == id && seg.inUse) {
#ifdef __linux__
                munmap(reinterpret_cast<void*>(seg.base), seg.size);
#else
                std::free(reinterpret_cast<void*>(seg.base));
#endif
                totalMapped_ -= seg.size;
                totalCommitted_ -= seg.committed;
                seg.inUse = false;
                break;
            }
        }
    }

    // Decommit pages to reduce RSS without unmapping.
    bool decommit(uint64_t id, size_t offset, size_t length) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& seg : segments_) {
            if (seg.id == id && seg.inUse) {
#ifdef __linux__
                uintptr_t addr = seg.base + offset;
                madvise(reinterpret_cast<void*>(addr), length, MADV_DONTNEED);
#endif
                seg.committed -= std::min(length, seg.committed);
                totalCommitted_ -= std::min(length, totalCommitted_);
                return true;
            }
        }
        return false;
    }

    // Recommit decommitted pages.
    bool recommit(uint64_t id, size_t offset, size_t length) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& seg : segments_) {
            if (seg.id == id && seg.inUse) {
                seg.committed += length;
                totalCommitted_ += length;
                return true;
            }
        }
        return false;
    }

    void releaseAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& seg : segments_) {
            if (seg.inUse) {
#ifdef __linux__
                munmap(reinterpret_cast<void*>(seg.base), seg.size);
#else
                std::free(reinterpret_cast<void*>(seg.base));
#endif
            }
        }
        segments_.clear();
        totalMapped_ = 0;
        totalCommitted_ = 0;
    }

    size_t totalMapped() const { return totalMapped_; }
    size_t totalCommitted() const { return totalCommitted_; }
    size_t segmentCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t c = 0;
        for (auto& s : segments_) if (s.inUse) c++;
        return c;
    }

private:
    static size_t alignUp(size_t value, size_t align) {
        return (value + align - 1) & ~(align - 1);
    }

    mutable std::mutex mutex_;
    std::vector<Segment> segments_;
    uint64_t nextId_ = 1;
    size_t totalMapped_ = 0;
    size_t totalCommitted_ = 0;
};

} // namespace Zepra::Heap
