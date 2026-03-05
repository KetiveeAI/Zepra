// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_config_api.cpp — Runtime GC configuration API

#include <atomic>
#include <mutex>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <string>
#include <unordered_map>

namespace Zepra::Runtime {

// Embedder-configurable GC parameters. Defaults are tuned for
// interactive browser workloads (short pauses, moderate throughput).

struct GCConfig {
    size_t nurserySize;
    size_t nurserySizeMax;
    size_t oldGenInitial;
    size_t oldGenMax;
    size_t largeObjectThreshold;
    double heapGrowthFactor;
    double gcTriggerRatio;
    uint8_t tenuringThreshold;
    size_t incrementalStepBytes;
    size_t tlabSize;
    bool concurrentMarking;
    bool concurrentSweeping;
    bool compactionEnabled;
    bool pretenureEnabled;

    static GCConfig browser() {
        GCConfig c;
        c.nurserySize = 2 * 1024 * 1024;
        c.nurserySizeMax = 16 * 1024 * 1024;
        c.oldGenInitial = 8 * 1024 * 1024;
        c.oldGenMax = 512 * 1024 * 1024;
        c.largeObjectThreshold = 8192;
        c.heapGrowthFactor = 1.5;
        c.gcTriggerRatio = 0.75;
        c.tenuringThreshold = 6;
        c.incrementalStepBytes = 4096;
        c.tlabSize = 32 * 1024;
        c.concurrentMarking = true;
        c.concurrentSweeping = true;
        c.compactionEnabled = true;
        c.pretenureEnabled = true;
        return c;
    }

    static GCConfig cli() {
        GCConfig c = browser();
        c.nurserySize = 1024 * 1024;
        c.oldGenMax = 128 * 1024 * 1024;
        c.concurrentMarking = false;
        return c;
    }

    static GCConfig embedded() {
        GCConfig c;
        c.nurserySize = 256 * 1024;
        c.nurserySizeMax = 2 * 1024 * 1024;
        c.oldGenInitial = 1024 * 1024;
        c.oldGenMax = 16 * 1024 * 1024;
        c.largeObjectThreshold = 4096;
        c.heapGrowthFactor = 1.2;
        c.gcTriggerRatio = 0.85;
        c.tenuringThreshold = 4;
        c.incrementalStepBytes = 1024;
        c.tlabSize = 8 * 1024;
        c.concurrentMarking = false;
        c.concurrentSweeping = false;
        c.compactionEnabled = false;
        c.pretenureEnabled = false;
        return c;
    }
};

class GCConfigAPI {
public:
    explicit GCConfigAPI(const GCConfig& config = GCConfig::browser())
        : config_(config) {}

    const GCConfig& config() const { return config_; }

    void setNurserySize(size_t bytes) { config_.nurserySize = bytes; }
    void setOldGenMax(size_t bytes) { config_.oldGenMax = bytes; }
    void setTenuringThreshold(uint8_t t) { config_.tenuringThreshold = t; }
    void setConcurrentMarking(bool v) { config_.concurrentMarking = v; }
    void setConcurrentSweeping(bool v) { config_.concurrentSweeping = v; }
    void setCompaction(bool v) { config_.compactionEnabled = v; }
    void setPretenuring(bool v) { config_.pretenureEnabled = v; }

    // Apply config changes at next safe point.
    bool validate() const {
        if (config_.nurserySize < 64 * 1024) return false;
        if (config_.oldGenMax < config_.oldGenInitial) return false;
        if (config_.tenuringThreshold > 15) return false;
        if (config_.heapGrowthFactor < 1.0) return false;
        return true;
    }

private:
    GCConfig config_;
};

} // namespace Zepra::Runtime
