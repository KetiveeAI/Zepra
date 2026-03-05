// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// ZepraScript — gc_heap_snapshot_serializer.cpp — V8-format heap snapshot export

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <algorithm>

namespace Zepra::Heap {

// Serializes heap state into Chrome DevTools Heap Snapshot format.
// Output is JSON matching V8's snapshot schema so existing tools
// (Chrome DevTools, memlab, heapviz) can consume it directly.

enum class SnapshotNodeType : uint8_t {
    Hidden = 0,
    Array,
    String,
    Object,
    Code,
    Closure,
    RegExp,
    Number,
    Native,
    Synthetic,
    ConcatenatedString,
    SlicedString,
    Symbol,
    BigInt
};

enum class SnapshotEdgeType : uint8_t {
    Context = 0,
    Element,
    Property,
    Internal,
    Hidden,
    Shortcut,
    Weak
};

struct SnapshotNode {
    SnapshotNodeType type;
    uint32_t nameIdx;      // Index into strings table
    uint32_t id;           // Unique node ID
    uint32_t selfSize;
    uint32_t edgeCount;
    uint32_t traceNodeId;
    uint8_t detachedness;  // 0=attached, 1=detached from DOM
};

struct SnapshotEdge {
    SnapshotEdgeType type;
    uint32_t nameOrIndex;  // String index or element index
    uint32_t toNode;       // Target node index (not ID)
};

class HeapSnapshotSerializer {
public:
    void addString(const std::string& s) {
        if (stringIndex_.count(s)) return;
        stringIndex_[s] = static_cast<uint32_t>(strings_.size());
        strings_.push_back(s);
    }

    uint32_t stringId(const std::string& s) {
        auto it = stringIndex_.find(s);
        if (it != stringIndex_.end()) return it->second;
        addString(s);
        return stringIndex_[s];
    }

    uint32_t addNode(SnapshotNodeType type, const std::string& name,
                      uint32_t id, uint32_t selfSize) {
        SnapshotNode node;
        node.type = type;
        node.nameIdx = stringId(name);
        node.id = id;
        node.selfSize = selfSize;
        node.edgeCount = 0;
        node.traceNodeId = 0;
        node.detachedness = 0;

        uint32_t nodeIdx = static_cast<uint32_t>(nodes_.size());
        nodes_.push_back(node);
        return nodeIdx;
    }

    void addEdge(uint32_t fromNodeIdx, SnapshotEdgeType type,
                  const std::string& name, uint32_t toNodeIdx) {
        SnapshotEdge edge;
        edge.type = type;
        edge.nameOrIndex = stringId(name);
        edge.toNode = toNodeIdx;
        edges_.push_back({fromNodeIdx, edge});
        if (fromNodeIdx < nodes_.size()) {
            nodes_[fromNodeIdx].edgeCount++;
        }
    }

    void addElementEdge(uint32_t fromNodeIdx, uint32_t index,
                          uint32_t toNodeIdx) {
        SnapshotEdge edge;
        edge.type = SnapshotEdgeType::Element;
        edge.nameOrIndex = index;
        edge.toNode = toNodeIdx;
        edges_.push_back({fromNodeIdx, edge});
        if (fromNodeIdx < nodes_.size()) {
            nodes_[fromNodeIdx].edgeCount++;
        }
    }

    // Serialize to DevTools-compatible JSON.
    bool serialize(FILE* out) {
        fprintf(out, "{\"snapshot\":{\"meta\":{\"node_fields\":"
            "[\"type\",\"name\",\"id\",\"self_size\",\"edge_count\","
            "\"trace_node_id\",\"detachedness\"],"
            "\"node_types\":[[\"hidden\",\"array\",\"string\",\"object\","
            "\"code\",\"closure\",\"regexp\",\"number\",\"native\","
            "\"synthetic\",\"concatenated_string\",\"sliced_string\","
            "\"symbol\",\"bigint\"],\"string\",\"number\",\"number\","
            "\"number\",\"number\",\"number\"],"
            "\"edge_fields\":[\"type\",\"name_or_index\",\"to_node\"],"
            "\"edge_types\":[[\"context\",\"element\",\"property\","
            "\"internal\",\"hidden\",\"shortcut\",\"weak\"],"
            "\"string_or_number\",\"node\"]},"
            "\"node_count\":%zu,\"edge_count\":%zu},",
            nodes_.size(), edges_.size());

        // Nodes array
        fprintf(out, "\"nodes\":[");
        for (size_t i = 0; i < nodes_.size(); i++) {
            if (i > 0) fprintf(out, ",");
            auto& n = nodes_[i];
            fprintf(out, "%u,%u,%u,%u,%u,%u,%u",
                static_cast<unsigned>(n.type), n.nameIdx, n.id,
                n.selfSize, n.edgeCount, n.traceNodeId, n.detachedness);
        }
        fprintf(out, "],");

        // Edges array (sorted by source node)
        std::sort(edges_.begin(), edges_.end(),
            [](const auto& a, const auto& b) {
                return a.first < b.first;
            });

        fprintf(out, "\"edges\":[");
        for (size_t i = 0; i < edges_.size(); i++) {
            if (i > 0) fprintf(out, ",");
            auto& e = edges_[i].second;
            // toNode is in units of node fields (7 per node)
            fprintf(out, "%u,%u,%u",
                static_cast<unsigned>(e.type), e.nameOrIndex,
                e.toNode * 7);
        }
        fprintf(out, "],");

        // Strings
        fprintf(out, "\"strings\":[");
        for (size_t i = 0; i < strings_.size(); i++) {
            if (i > 0) fprintf(out, ",");
            fprintf(out, "\"");
            for (char c : strings_[i]) {
                if (c == '"') fprintf(out, "\\\"");
                else if (c == '\\') fprintf(out, "\\\\");
                else if (c == '\n') fprintf(out, "\\n");
                else fprintf(out, "%c", c);
            }
            fprintf(out, "\"");
        }
        fprintf(out, "]}");

        return true;
    }

    size_t nodeCount() const { return nodes_.size(); }
    size_t edgeCount() const { return edges_.size(); }

private:
    std::vector<SnapshotNode> nodes_;
    std::vector<std::pair<uint32_t, SnapshotEdge>> edges_;
    std::vector<std::string> strings_;
    std::unordered_map<std::string, uint32_t> stringIndex_;
};

} // namespace Zepra::Heap
