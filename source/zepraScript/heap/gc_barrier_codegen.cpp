// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_barrier_codegen.cpp — JIT write barrier code emission

#include <vector>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <functional>

namespace Zepra::Heap {

// The JIT compiler emits inline write barriers into generated machine code.
// This module provides the barrier templates that the JIT patches into
// compiled functions. The fast path is ~5 instructions, the slow path
// calls into the runtime.

enum class BarrierKind : uint8_t {
    StoreField,       // obj.field = val
    StoreElement,     // arr[idx] = val
    StoreGlobal,      // global.x = val
    BulkCopy          // Array splice/copy
};

enum class Arch : uint8_t {
    X86_64,
    AArch64
};

// Barrier patch point — where JIT-compiled code calls back to GC.
struct BarrierPatchSite {
    uintptr_t codeAddr;       // Address in compiled code
    size_t offset;             // Offset from function start
    BarrierKind kind;
    uint8_t srcReg;            // Register holding source object
    uint8_t valReg;            // Register holding new value
    uint8_t slotOffsetImm;    // Immediate offset for field store
};

// x86-64 barrier stubs
struct X64BarrierStub {
    // Fast path: check if src is in nursery → skip
    // cmp src, nursery_end
    // jb .skip
    // call slow_path
    // .skip:
    static constexpr uint8_t FAST_PATH_TEMPLATE[] = {
        0x48, 0x3B, 0x3D, 0x00, 0x00, 0x00, 0x00,  // cmp rdi, [rip+nursery_end]
        0x72, 0x05,                                    // jb skip (5 bytes ahead)
        0xE8, 0x00, 0x00, 0x00, 0x00                  // call slow_path
    };
    static constexpr size_t FAST_PATH_SIZE = sizeof(FAST_PATH_TEMPLATE);
    static constexpr size_t NURSERY_END_OFFSET = 3;
    static constexpr size_t CALL_TARGET_OFFSET = 10;
};

// AArch64 barrier stubs
struct AArch64BarrierStub {
    // Fast path:
    // ldr x16, nursery_end
    // cmp src, x16
    // b.lo skip
    // bl slow_path
    // skip:
    static constexpr uint32_t FAST_PATH_TEMPLATE[] = {
        0x58000010,    // ldr x16, #nursery_end_literal
        0xEB10001F,    // cmp src, x16
        0x54000043,    // b.lo skip (3 instructions ahead)
        0x94000000     // bl slow_path
    };
    static constexpr size_t FAST_PATH_SIZE = sizeof(FAST_PATH_TEMPLATE);
};

class BarrierCodeGen {
public:
    explicit BarrierCodeGen(Arch arch) : arch_(arch) {}

    // Emit fast-path barrier into code buffer.
    size_t emitBarrier(uint8_t* buffer, size_t bufferSize,
                        BarrierKind kind,
                        uintptr_t nurseryEndAddr,
                        uintptr_t slowPathAddr,
                        uintptr_t currentPC) {
        switch (arch_) {
            case Arch::X86_64:
                return emitX64Barrier(buffer, bufferSize, kind,
                    nurseryEndAddr, slowPathAddr, currentPC);
            case Arch::AArch64:
                return emitAArch64Barrier(buffer, bufferSize, kind,
                    nurseryEndAddr, slowPathAddr, currentPC);
        }
        return 0;
    }

    // Record patch site for later GC updates.
    void recordPatchSite(const BarrierPatchSite& site) {
        patchSites_.push_back(site);
    }

    // When nursery moves (after GC), patch all barrier comparisons.
    void patchNurseryBounds(uintptr_t newEnd) {
        for (auto& site : patchSites_) {
            patchSiteNurseryEnd(site, newEnd);
        }
    }

    size_t patchSiteCount() const { return patchSites_.size(); }

private:
    size_t emitX64Barrier(uint8_t* buffer, size_t bufferSize,
                            BarrierKind kind,
                            uintptr_t nurseryEndAddr,
                            uintptr_t slowPathAddr,
                            uintptr_t currentPC) {
        if (bufferSize < X64BarrierStub::FAST_PATH_SIZE) return 0;

        std::memcpy(buffer, X64BarrierStub::FAST_PATH_TEMPLATE,
            X64BarrierStub::FAST_PATH_SIZE);

        // Patch RIP-relative nursery_end address
        uintptr_t ripAfterCmp = currentPC + 7;
        int32_t nurseryEndRel = static_cast<int32_t>(
            nurseryEndAddr - ripAfterCmp);
        std::memcpy(buffer + X64BarrierStub::NURSERY_END_OFFSET,
            &nurseryEndRel, 4);

        // Patch call target
        uintptr_t ripAfterCall = currentPC + X64BarrierStub::FAST_PATH_SIZE;
        int32_t callRel = static_cast<int32_t>(
            slowPathAddr - ripAfterCall);
        std::memcpy(buffer + X64BarrierStub::CALL_TARGET_OFFSET,
            &callRel, 4);

        return X64BarrierStub::FAST_PATH_SIZE;
    }

    size_t emitAArch64Barrier(uint8_t* buffer, size_t bufferSize,
                                BarrierKind kind,
                                uintptr_t nurseryEndAddr,
                                uintptr_t slowPathAddr,
                                uintptr_t currentPC) {
        if (bufferSize < AArch64BarrierStub::FAST_PATH_SIZE) return 0;

        std::memcpy(buffer, AArch64BarrierStub::FAST_PATH_TEMPLATE,
            AArch64BarrierStub::FAST_PATH_SIZE);

        // Patch bl offset for slow path
        uintptr_t blPC = currentPC + 12;
        int32_t offset = static_cast<int32_t>(
            (slowPathAddr - blPC) >> 2);
        uint32_t blInsn = 0x94000000 | (offset & 0x03FFFFFF);
        std::memcpy(buffer + 12, &blInsn, 4);

        return AArch64BarrierStub::FAST_PATH_SIZE;
    }

    void patchSiteNurseryEnd(const BarrierPatchSite& site,
                               uintptr_t newEnd) {
        if (arch_ == Arch::X86_64) {
            uintptr_t ripAfterCmp = site.codeAddr +
                X64BarrierStub::NURSERY_END_OFFSET + 4;
            int32_t rel = static_cast<int32_t>(newEnd - ripAfterCmp);
            auto* patchLoc = reinterpret_cast<int32_t*>(
                site.codeAddr + X64BarrierStub::NURSERY_END_OFFSET);
            *patchLoc = rel;
        }
    }

    Arch arch_;
    std::vector<BarrierPatchSite> patchSites_;
};

} // namespace Zepra::Heap
