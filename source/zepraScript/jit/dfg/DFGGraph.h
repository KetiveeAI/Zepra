/**
 * @file DFGGraph.h
 * @brief Data Flow Graph Container
 * 
 * Manages the entire function's DFG, including nodes and basic blocks.
 */

#pragma once

#include "DFGBasicBlock.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace Zepra::JIT::DFG {

class Graph {
public:
    Graph() = default;
    
    // Factory methods
    BasicBlock* createBlock() {
        uint32_t id = static_cast<uint32_t>(blocks_.size());
        auto block = std::make_unique<BasicBlock>(id);
        BasicBlock* ptr = block.get();
        blocks_.push_back(std::move(block));
        return ptr;
    }
    
    Node* createNode(NodeType type) {
        uint32_t id = static_cast<uint32_t>(nodes_.size());
        auto node = std::make_unique<Node>(id, type);
        Node* ptr = node.get();
        nodes_.push_back(std::move(node));
        return ptr;
    }
    
    // Accessors
    const std::vector<std::unique_ptr<BasicBlock>>& blocks() const { return blocks_; }
    const std::vector<std::unique_ptr<Node>>& nodes() const { return nodes_; }
    
    // Simple verification
    bool verify() const;
    
    // Dump to string (for debugging)
    void dump() const {
        std::cout << "DFG Graph (" << blocks_.size() << " blocks)" << std::endl;
        for (const auto& block : blocks_) {
            std::cout << "  Block " << block->id() << ":" << std::endl;
            for (const auto* node : block->nodes()) {
                std::cout << "    Node " << node->id << " " << nodeTypeToString(node->type) << std::endl;
            }
        }
    }

private:
    std::vector<std::unique_ptr<BasicBlock>> blocks_;
    std::vector<std::unique_ptr<Node>> nodes_;
};

} // namespace Zepra::JIT::DFG
