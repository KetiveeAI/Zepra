// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_snapshot_test.cpp — Heap snapshot serializer tests

#include <vector>
#include <functional>
#include <string>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cassert>

namespace Zepra::Test {

// Minimal snapshot builder for testing serialization correctness.
struct TestSnapshotNode {
    uint8_t type;
    std::string name;
    uint32_t id;
    uint32_t selfSize;
    uint32_t edgeCount;
};

struct TestSnapshotEdge {
    uint8_t type;
    std::string name;
    uint32_t fromIdx;
    uint32_t toIdx;
};

class TestSnapshotBuilder {
public:
    uint32_t addNode(uint8_t type, const std::string& name,
                      uint32_t id, uint32_t size) {
        uint32_t idx = static_cast<uint32_t>(nodes_.size());
        nodes_.push_back({type, name, id, size, 0});
        return idx;
    }

    void addEdge(uint32_t fromIdx, uint8_t type,
                  const std::string& name, uint32_t toIdx) {
        edges_.push_back({type, name, fromIdx, toIdx});
        if (fromIdx < nodes_.size()) nodes_[fromIdx].edgeCount++;
    }

    size_t nodeCount() const { return nodes_.size(); }
    size_t edgeCount() const { return edges_.size(); }

    bool writeToTmpFile(const char* path) {
        FILE* f = fopen(path, "w");
        if (!f) return false;
        fprintf(f, "{\"snapshot\":{\"node_count\":%zu,\"edge_count\":%zu},",
            nodes_.size(), edges_.size());
        fprintf(f, "\"nodes\":[");
        for (size_t i = 0; i < nodes_.size(); i++) {
            if (i > 0) fprintf(f, ",");
            fprintf(f, "%u,%u,%u,%u,%u,0,0",
                nodes_[i].type, static_cast<unsigned>(i),
                nodes_[i].id, nodes_[i].selfSize, nodes_[i].edgeCount);
        }
        fprintf(f, "],\"edges\":[");
        for (size_t i = 0; i < edges_.size(); i++) {
            if (i > 0) fprintf(f, ",");
            fprintf(f, "%u,%u,%u",
                edges_[i].type, static_cast<unsigned>(i),
                edges_[i].toIdx * 7);
        }
        fprintf(f, "]}");
        fclose(f);
        return true;
    }

private:
    std::vector<TestSnapshotNode> nodes_;
    std::vector<TestSnapshotEdge> edges_;
};

using SnapshotTestFn = std::function<bool(const char*& fail)>;

class SnapshotTestRunner {
public:
    void add(const char* name, SnapshotTestFn fn) {
        tests_.push_back({name, std::move(fn)});
    }

    size_t runAll() {
        size_t p = 0, f = 0;
        fprintf(stderr, "\n=== Heap Snapshot Tests ===\n\n");
        for (auto& [name, fn] : tests_) {
            const char* fail = nullptr;
            bool ok = false;
            try { ok = fn(fail); } catch (...) { fail = "exception"; }
            if (ok) { fprintf(stderr, "  [PASS] %s\n", name); p++; }
            else { fprintf(stderr, "  [FAIL] %s: %s\n", name, fail ? fail : "?"); f++; }
        }
        fprintf(stderr, "\n  %zu passed, %zu failed\n\n", p, f);
        return f;
    }

private:
    std::vector<std::pair<const char*, SnapshotTestFn>> tests_;
};

static void registerSnapshotTests(SnapshotTestRunner& runner) {

    runner.add("EmptySnapshot", [](const char*& fail) -> bool {
        TestSnapshotBuilder builder;
        if (builder.nodeCount() != 0 || builder.edgeCount() != 0) {
            fail = "empty builder should have 0 nodes/edges"; return false;
        }
        return true;
    });

    runner.add("SingleNode", [](const char*& fail) -> bool {
        TestSnapshotBuilder builder;
        auto idx = builder.addNode(3, "Object", 1, 64);
        if (idx != 0) { fail = "first node should be index 0"; return false; }
        if (builder.nodeCount() != 1) { fail = "expected 1 node"; return false; }
        return true;
    });

    runner.add("NodeWithEdges", [](const char*& fail) -> bool {
        TestSnapshotBuilder builder;
        auto parent = builder.addNode(3, "Parent", 1, 32);
        auto child = builder.addNode(3, "Child", 2, 16);
        builder.addEdge(parent, 2, "child", child);

        if (builder.edgeCount() != 1) { fail = "expected 1 edge"; return false; }
        return true;
    });

    runner.add("SerializeToFile", [](const char*& fail) -> bool {
        TestSnapshotBuilder builder;
        builder.addNode(3, "Root", 1, 128);
        builder.addNode(1, "Array", 2, 256);
        builder.addEdge(0, 2, "items", 1);

        bool ok = builder.writeToTmpFile("/tmp/gc_snapshot_test.json");
        if (!ok) { fail = "write failed"; return false; }

        FILE* f = fopen("/tmp/gc_snapshot_test.json", "r");
        if (!f) { fail = "read failed"; return false; }
        char buf[64];
        size_t n = fread(buf, 1, 63, f);
        buf[n] = 0;
        fclose(f);

        if (strstr(buf, "\"snapshot\"") == nullptr) {
            fail = "missing snapshot key"; return false;
        }
        return true;
    });

    runner.add("MultipleEdges", [](const char*& fail) -> bool {
        TestSnapshotBuilder builder;
        auto root = builder.addNode(3, "Root", 1, 32);
        for (int i = 0; i < 5; i++) {
            auto child = builder.addNode(3, "child", i + 2, 16);
            builder.addEdge(root, 2, "c" + std::to_string(i), child);
        }
        if (builder.edgeCount() != 5) { fail = "expected 5 edges"; return false; }
        return true;
    });

    runner.add("LargeGraph", [](const char*& fail) -> bool {
        TestSnapshotBuilder builder;
        for (int i = 0; i < 1000; i++) {
            builder.addNode(3, "obj", i, 32);
        }
        for (int i = 0; i < 999; i++) {
            builder.addEdge(i, 2, "next", i + 1);
        }
        if (builder.nodeCount() != 1000) { fail = "expected 1000 nodes"; return false; }
        if (builder.edgeCount() != 999) { fail = "expected 999 edges"; return false; }
        return true;
    });
}

static size_t runSnapshotTests() {
    SnapshotTestRunner runner;
    registerSnapshotTests(runner);
    return runner.runAll();
}

} // namespace Zepra::Test
