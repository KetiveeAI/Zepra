// ZepraScript WASM CPU Feature Detection
// Copyright (c) 2024 KetiveeAI - All Rights Reserved

#pragma once

#include <cstdint>

namespace Zepra::Wasm {

// CPU feature flags detected via CPUID
class CpuFeatures {
public:
    // Feature detection flags
    bool hasSSE2 = false;
    bool hasSSE3 = false;
    bool hasSSSE3 = false;
    bool hasSSE41 = false;
    bool hasSSE42 = false;
    bool hasAVX = false;
    bool hasAVX2 = false;
    bool hasBMI1 = false;
    bool hasBMI2 = false;
    bool hasLZCNT = false;
    bool hasPOPCNT = false;
    bool hasFMA = false;
    
    // Initialize feature detection (call once at startup)
    void detect() {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
        uint32_t eax, ebx, ecx, edx;
        
        // Get max CPUID function
        cpuid(0, 0, eax, ebx, ecx, edx);
        uint32_t maxFunc = eax;
        
        if (maxFunc >= 1) {
            cpuid(1, 0, eax, ebx, ecx, edx);
            
            // ECX features
            hasSSE3   = (ecx >> 0) & 1;
            hasSSSE3  = (ecx >> 9) & 1;
            hasSSE41  = (ecx >> 19) & 1;  // SSE4.1: roundss, roundsd, etc.
            hasSSE42  = (ecx >> 20) & 1;
            hasPOPCNT = (ecx >> 23) & 1;
            hasAVX    = (ecx >> 28) & 1;
            hasFMA    = (ecx >> 12) & 1;
            
            // EDX features
            hasSSE2   = (edx >> 26) & 1;
        }
        
        if (maxFunc >= 7) {
            cpuid(7, 0, eax, ebx, ecx, edx);
            
            // EBX features
            hasBMI1  = (ebx >> 3) & 1;
            hasAVX2  = (ebx >> 5) & 1;
            hasBMI2  = (ebx >> 8) & 1;
            hasLZCNT = hasBMI1;  // LZCNT is part of BMI1
        }
#else
        // Non-x86: assume no SSE extensions
        hasSSE2 = false;
        hasSSE41 = false;
#endif
    }
    
    // Singleton access
    static CpuFeatures& instance() {
        static CpuFeatures features;
        return features;
    }
    
    // Convenience accessor
    static bool sse41Supported() {
        return instance().hasSSE41;
    }
    
    static bool popcntSupported() {
        return instance().hasPOPCNT;
    }
    
    static bool lzcntSupported() {
        return instance().hasLZCNT;
    }

private:
    CpuFeatures() {
        detect();
    }
    
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    static void cpuid(uint32_t func, uint32_t subfunc,
                      uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx) {
#if defined(__GNUC__) || defined(__clang__)
        __asm__ __volatile__(
            "cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(func), "c"(subfunc)
        );
#elif defined(_MSC_VER)
        int cpuInfo[4];
        __cpuidex(cpuInfo, func, subfunc);
        eax = cpuInfo[0];
        ebx = cpuInfo[1];
        ecx = cpuInfo[2];
        edx = cpuInfo[3];
#endif
    }
#endif
};

} // namespace Zepra::Wasm
