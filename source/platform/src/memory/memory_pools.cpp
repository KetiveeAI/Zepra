#include "memory/memory_pools.h"
#include <cstdio>

namespace Zepra::Memory {

// Explicit instantiation of common pool types
// This helps with compilation and debugging

// Note: Implementation is header-only for templates
// This file exists for future non-template additions

void printMemoryStats() {
    auto& pools = MemoryPools::instance();
    auto stats = pools.getStats();
    
    printf("=== Memory Pool Statistics ===\n");
    printf("Total Allocated: %zu objects\n", stats.total_allocated);
    printf("Total Capacity:  %zu objects\n", stats.total_capacity);
    printf("Total Free:      %zu objects\n", stats.total_free);
    printf("Memory Usage:    %.2f MB\n", stats.memory_usage / (1024.0 * 1024.0));
    printf("Utilization:     %.1f%%\n", 
           stats.total_capacity > 0 
               ? (100.0 * stats.total_allocated / stats.total_capacity)
               : 0.0);
    printf("==============================\n");
}

} // namespace Zepra::Memory
