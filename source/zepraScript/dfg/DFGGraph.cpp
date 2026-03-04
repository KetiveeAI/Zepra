/**
 * @file DFGGraph.cpp
 * @brief DFG Graph implementation
 */

#include "dfg/DFGGraph.h"
#include <cstdio>

namespace Zepra::DFG {

void Graph::dump() const {
    printf("=== DFG Graph (func %u) ===\n", funcIndex_);
    printf("Params: %u, Locals: %u, Result: %s\n", 
           numParams_, numLocals_, typeName(resultType_));
    printf("Blocks: %u, Values: %u\n\n", numBlocks(), numValues());
    
    for (auto& block : blocks_) {
        printf("Block %u:", block->index());
        
        // Print predecessors
        if (!block->predecessors().empty()) {
            printf(" <- [");
            for (size_t i = 0; i < block->predecessors().size(); ++i) {
                if (i > 0) printf(", ");
                printf("%u", block->predecessors()[i]->index());
            }
            printf("]");
        }
        
        // Print successors
        if (!block->successors().empty()) {
            printf(" -> [");
            for (size_t i = 0; i < block->successors().size(); ++i) {
                if (i > 0) printf(", ");
                printf("%u", block->successors()[i]->index());
            }
            printf("]");
        }
        
        if (block->isLoopHeader()) {
            printf(" [LOOP]");
        }
        
        printf("\n");
        
        // Print phi nodes
        for (Value* phi : block->phis()) {
            printf("  v%u = Phi(%s)", phi->index(), typeName(phi->type()));
            for (uint32_t i = 0; i < phi->numInputs(); ++i) {
                printf(" v%u", phi->input(i)->index());
            }
            printf("\n");
        }
        
        // Print values
        for (Value* v : block->values()) {
            printf("  v%u = %s", v->index(), opcodeName(v->opcode()));
            
            if (v->isConstant()) {
                switch (v->type()) {
                    case Type::I32: printf(" %d", v->asInt32()); break;
                    case Type::I64: printf(" %lld", static_cast<long long>(v->asInt64())); break;
                    case Type::F32: printf(" %f", v->asFloat32()); break;
                    case Type::F64: printf(" %f", v->asFloat64()); break;
                    default: break;
                }
            } else {
                for (uint32_t i = 0; i < v->numInputs(); ++i) {
                    Value* input = v->input(i);
                    if (input) {
                        printf(" v%u", input->index());
                    } else {
                        printf(" null");
                    }
                }
            }
            
            printf("\n");
        }
        
        printf("\n");
    }
}

} // namespace Zepra::DFG
