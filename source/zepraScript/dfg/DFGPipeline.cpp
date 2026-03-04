/**
 * @file DFGPipeline.cpp
 * @brief DFG JIT Compilation Pipeline
 *
 * Orchestrates the full DFG optimization pipeline:
 *   1. Build DFG graph from bytecode (DFGBuilder)
 *   2. Constant folding
 *   3. Strength reduction
 *   4. Dead code elimination  
 *   5. Lowering to machine code
 *   6. Emit into executable buffer
 *
 * Ref: JSC DFG::Plan, SpiderMonkey Ion pipeline
 */

#include "dfg/DFGGraph.h"
#include "dfg/DFGBuilder.h"
#include "dfg/passes/DFGConstFold.h"
#include "dfg/passes/DFGDeadCodeElim.h"
#include "dfg/passes/DFGStrengthReduce.h"
#include "jit/MacroAssembler.h"
#include <memory>
#include <cstdio>
#include <chrono>

namespace Zepra::DFG {

// Forward declaration from DFGLowering.cpp
void lowerGraph(Graph* graph, JIT::MacroAssembler& masm);

// =============================================================================
// Compilation Result
// =============================================================================

struct CompilationResult {
    bool success = false;
    std::unique_ptr<JIT::ExecutableBuffer> code;
    size_t codeSize = 0;
    double compileTimeMs = 0;
    
    // Statistics
    uint32_t numBlocksBefore = 0;
    uint32_t numValuesBefore = 0;
    uint32_t numBlocksAfter = 0;
    uint32_t numValuesAfter = 0;
    uint32_t constFoldReductions = 0;
    uint32_t strengthReductions = 0;
    uint32_t deadCodeEliminated = 0;
};

// =============================================================================
// Pipeline Phases
// =============================================================================

enum class Phase : uint8_t {
    Build,
    ConstantFolding,
    StrengthReduction,
    DeadCodeElimination,
    Lowering,
    NumPhases
};

static const char* phaseName(Phase p) {
    switch (p) {
        case Phase::Build: return "Build";
        case Phase::ConstantFolding: return "ConstFold";
        case Phase::StrengthReduction: return "StrengthReduce";
        case Phase::DeadCodeElimination: return "DCE";
        case Phase::Lowering: return "Lowering";
        default: return "Unknown";
    }
}

// =============================================================================
// DFG Pipeline
// =============================================================================

class Pipeline {
public:
    explicit Pipeline(uint32_t funcIndex,
                      bool verbose = false)
        : funcIndex_(funcIndex)
        , verbose_(verbose)
        , module_(nullptr) {}

    void setModule(const Wasm::WasmModule* mod) { module_ = mod; }

    CompilationResult compile() {
        CompilationResult result;
        auto start = std::chrono::steady_clock::now();

        // Phase 1: Build DFG graph from bytecode
        if (!runPhase(Phase::Build, result)) return result;

        // Record pre-optimization stats
        result.numBlocksBefore = graph_->numBlocks();
        result.numValuesBefore = graph_->numValues();

        // Phase 2: Constant folding (iterate until fixpoint)
        {
            uint32_t preValues = graph_->numValues();
            runPhase(Phase::ConstantFolding, result);
            result.constFoldReductions = preValues - graph_->numValues();
        }

        // Phase 3: Strength reduction
        {
            uint32_t preValues = graph_->numValues();
            runPhase(Phase::StrengthReduction, result);
            result.strengthReductions = preValues - graph_->numValues();
        }

        // Phase 4: Dead code elimination
        {
            uint32_t preValues = graph_->numValues();
            runPhase(Phase::DeadCodeElimination, result);
            result.deadCodeEliminated = preValues - graph_->numValues();
        }

        // Record post-optimization stats
        result.numBlocksAfter = graph_->numBlocks();
        result.numValuesAfter = graph_->numValues();

        if (verbose_) {
            printf("DFG Pipeline: %u blocks, %u→%u values "
                   "(fold=%u, sr=%u, dce=%u)\n",
                   result.numBlocksAfter,
                   result.numValuesBefore, result.numValuesAfter,
                   result.constFoldReductions,
                   result.strengthReductions,
                   result.deadCodeEliminated);
        }

        // Phase 5: Lower to machine code
        if (!runPhase(Phase::Lowering, result)) return result;

        // Finalize: Copy to executable buffer
        auto execBuf = std::make_unique<JIT::ExecutableBuffer>();
        if (!execBuf->allocate(masm_.size())) {
            fprintf(stderr, "DFG: Failed to allocate executable buffer (%zu bytes)\n",
                    masm_.size());
            return result;
        }

        if (!execBuf->copyFrom(masm_)) {
            fprintf(stderr, "DFG: Failed to copy code to executable buffer\n");
            return result;
        }

        execBuf->makeExecutable();

        auto end = std::chrono::steady_clock::now();
        result.compileTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        result.codeSize = masm_.size();
        result.code = std::move(execBuf);
        result.success = true;

        if (verbose_) {
            printf("DFG Pipeline: compiled func %u → %zu bytes in %.2fms\n",
                   funcIndex_, result.codeSize, result.compileTimeMs);
        }

        return result;
    }

private:
    bool runPhase(Phase phase, CompilationResult& result) {
        auto phaseStart = std::chrono::steady_clock::now();

        switch (phase) {
            case Phase::Build: {
                graph_ = std::make_unique<Graph>();
                graph_->setFunctionIndex(funcIndex_);

                if (!module_) {
                    fprintf(stderr, "DFG Build: no module set for func %u\n", funcIndex_);
                    return false;
                }

                Builder builder(module_);
                if (!builder.build(funcIndex_, graph_.get())) {
                    fprintf(stderr, "DFG Build: failed for func %u: %s\n",
                            funcIndex_, builder.error().c_str());
                    return false;
                }

                if (graph_->numBlocks() == 0) {
                    fprintf(stderr, "DFG Build: empty graph for func %u\n", funcIndex_);
                    return false;
                }

                if (verbose_) {
                    printf("  [%s] %u blocks, %u values\n",
                           phaseName(phase),
                           graph_->numBlocks(), graph_->numValues());
                }
                break;
            }

            case Phase::ConstantFolding: {
                ConstantFolding cf(graph_.get());
                bool changed = true;
                int iters = 0;
                while (changed && iters < 10) {
                    changed = cf.run();
                    iters++;
                }
                if (verbose_) {
                    printf("  [%s] %d iterations\n", phaseName(phase), iters);
                }
                break;
            }

            case Phase::StrengthReduction: {
                StrengthReduction sr(graph_.get());
                sr.run();
                break;
            }

            case Phase::DeadCodeElimination: {
                DeadCodeElimination dce(graph_.get());
                dce.run();
                break;
            }

            case Phase::Lowering: {
                lowerGraph(graph_.get(), masm_);
                if (verbose_) {
                    printf("  [%s] emitted %zu bytes\n",
                           phaseName(phase), masm_.size());
                }
                break;
            }

            default:
                break;
        }

        auto phaseEnd = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(phaseEnd - phaseStart).count();

        if (verbose_) {
            printf("  [%s] %.3fms\n", phaseName(phase), ms);
        }

        return true;
    }

    uint32_t funcIndex_;
    bool verbose_;
    const Wasm::WasmModule* module_;

    std::unique_ptr<Graph> graph_;
    JIT::MacroAssembler masm_;
};

// =============================================================================
// Public API
// =============================================================================

CompilationResult compileDFG(uint32_t funcIndex,
                              const Wasm::WasmModule* module,
                              bool verbose) {
    Pipeline pipeline(funcIndex, verbose);
    pipeline.setModule(module);
    return pipeline.compile();
}

} // namespace Zepra::DFG
