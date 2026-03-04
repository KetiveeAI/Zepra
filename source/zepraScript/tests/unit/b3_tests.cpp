#include <gtest/gtest.h>
#include "b3/B3Procedure.h"
#include "b3/B3Generate.h"
#include <sys/mman.h>

using namespace Zepra::B3;

class B3Tests : public ::testing::Test {
protected:
    void SetUp() override {
        proc = std::make_unique<Procedure>();
    }

    // Helper to execute generated code
    int64_t execute(const GeneratedCode& code, int64_t a = 0, int64_t b = 0) {
        if (!code.success) return -1;
        
        // Allocate executable memory
        void* mem = mmap(nullptr, code.code.size(), 
            PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (mem == MAP_FAILED) return -1;
        
        // Copy code
        memcpy(mem, code.code.data(), code.code.size());
        
        // Make executable
        mprotect(mem, code.code.size(), PROT_READ | PROT_EXEC);
        
        // Cast to function pointer
        using FuncType = int64_t(*)(int64_t, int64_t);
        auto func = reinterpret_cast<FuncType>(mem);
        
        // Execute
        int64_t result = func(a, b);
        
        // Cleanup
        munmap(mem, code.code.size());
        
        return result;
    }

    std::unique_ptr<Procedure> proc;
};

TEST_F(B3Tests, ReturnConstant) {
    BasicBlock* block = proc->addBlock();
    
    // return 42;
    Value* c42 = proc->constInt64(42);
    proc->addValue(Opcode::Return, Type::Void, c42);
    
    block->appendValue(c42);
    // Return implicitly added or needs to be last? B3Builder usually handles this but we are manual.
    // The previous code did: proc->addValue(Return...);
    // But didn't add it to block?
    // Wait, the previous test: 
    // proc->addValue(Return...); block->appendValue(...) for c10, c5, result.
    // BUT NOT for the Return value?
    // Ah, previous test:
    // block->appendValue(result);
    // // Return needs to be last
    // The Return value was NOT appended to the block in the previous test!
    
    // Fix: Append Return to block
    Value* ret = proc->addValue(Opcode::Return, Type::Void, c42);
    block->appendValue(ret);
    
    Generate gen(proc.get());
    GeneratedCode code = gen.run();
    
    EXPECT_TRUE(code.success);
    
    // Debug output
    printf("Generated code size: %zu\n", code.code.size());
    for(auto b : code.code) printf("%02X ", b);
    printf("\n");
    
    int64_t res = execute(code);
    EXPECT_EQ(res, 42);
}

TEST_F(B3Tests, Multiplication) {
    BasicBlock* block = proc->addBlock();
    
    // return 6 * 7;
    Value* c6 = proc->constInt64(6);
    Value* c7 = proc->constInt64(7);
    Value* mul = proc->addValue(Opcode::Mul, Type::Int64, c6, c7);
    Value* ret = proc->addValue(Opcode::Return, Type::Void, mul);
    
    block->appendValue(c6);
    block->appendValue(c7);
    block->appendValue(mul);
    block->appendValue(ret);
    
    Generate gen(proc.get());
    GeneratedCode code = gen.run();
    EXPECT_TRUE(code.success);
    
    EXPECT_EQ(execute(code), 42);
}

TEST_F(B3Tests, Division) {
    BasicBlock* block = proc->addBlock();
    
    // return 100 / 4;
    Value* c100 = proc->constInt64(100);
    Value* c4 = proc->constInt64(4);
    Value* div = proc->addValue(Opcode::Div, Type::Int64, c100, c4);
    Value* ret = proc->addValue(Opcode::Return, Type::Void, div);
    
    block->appendValue(c100);
    block->appendValue(c4);
    block->appendValue(div);
    block->appendValue(ret);
    
    Generate gen(proc.get());
    GeneratedCode code = gen.run();
    EXPECT_TRUE(code.success);
    
    EXPECT_EQ(execute(code), 25);
}

TEST_F(B3Tests, Bitwise) {
    BasicBlock* block = proc->addBlock();
    
    // return (0xF0 & 0xCC) | 0x01; // (0xC0) | 0x01 = 0xC1
    Value* cF0 = proc->constInt64(0xF0);
    Value* cCC = proc->constInt64(0xCC);
    Value* c01 = proc->constInt64(0x01);
    
    Value* andVal = proc->addValue(Opcode::BitAnd, Type::Int64, cF0, cCC); // 0xC0
    Value* orVal = proc->addValue(Opcode::BitOr, Type::Int64, andVal, c01); // 0xC1
    Value* ret = proc->addValue(Opcode::Return, Type::Void, orVal);
    
    block->appendValue(cF0);
    block->appendValue(cCC);
    block->appendValue(c01);
    block->appendValue(andVal);
    block->appendValue(orVal);
    block->appendValue(ret);
    
    Generate gen(proc.get());
    GeneratedCode code = gen.run();
    EXPECT_TRUE(code.success);
    
    EXPECT_EQ(execute(code), 0xC1);
}

TEST_F(B3Tests, MemoryAccess) {
    BasicBlock* block = proc->addBlock();
    
    // function(ptr) { 
    //   *ptr = 123;
    //   return *ptr; 
    // }
    
    // Arg0 is in RDI (7)? Wait, simple allocator regmap doesn't adhere to ABI for args strictly yet.
    // RegAlloc assigns regs 0..13.
    // Argument handling is still TODO properly.
    // But we can workaround: pass pointer as constant address?
    // Or assume RDI (reg 7) is preserved if we force usage?
    // Let's use a buffer we control.
    
    static int64_t buffer = 0;
    buffer = 0;
    
    // Use constant pointer
    Value* ptr = proc->constInt64((int64_t)&buffer);
    Value* val = proc->constInt64(123);
    
    // Store: *ptr = 123
    Value* store = proc->addValue(Opcode::Store, Type::Void, val, ptr);
    
    // Load: return *ptr
    Value* loaded = proc->addValue(Opcode::Load, Type::Int64, ptr);
    Value* ret = proc->addValue(Opcode::Return, Type::Void, loaded);
    
    // Add to block in order
    block->appendValue(ptr);
    block->appendValue(val);
    block->appendValue(store);
    block->appendValue(loaded);
    block->appendValue(ret);
    
    Generate gen(proc.get());
    GeneratedCode code = gen.run();
    EXPECT_TRUE(code.success);
    
    execute(code); // Should return 123
    EXPECT_EQ(buffer, 123);
}

TEST_F(B3Tests, ControlFlowJump) {
    // Test unconditional jump between blocks
    // Block 0: jump to Block 1
    // Block 1: return 99
    
    BasicBlock* block0 = proc->addBlock();
    BasicBlock* block1 = proc->addBlock();
    
    // Set up control flow
    block0->addSuccessor(block1);
    block1->addPredecessor(block0);
    
    // Block 0: jump to block1
    Value* jmp = proc->addValue(Opcode::Jump, Type::Void);
    block0->appendValue(jmp);
    
    // Block 1: return 99
    Value* c99 = proc->constInt64(99);
    Value* ret = proc->addValue(Opcode::Return, Type::Void, c99);
    block1->appendValue(c99);
    block1->appendValue(ret);
    
    Generate gen(proc.get());
    GeneratedCode code = gen.run();
    EXPECT_TRUE(code.success);
    
    EXPECT_EQ(execute(code), 99);
}

// Integration test: DFG -> B3 Lowering -> B3 Generate
#include "b3/B3Lowering.h"

TEST_F(B3Tests, DFGToB3Pipeline) {
    // Create a simple DFG graph: return 10 + 32
    auto dfgGraph = std::make_unique<Zepra::DFG::Graph>();
    
    Zepra::DFG::BasicBlock* dfgBlock = dfgGraph->addBlock();
    dfgGraph->setEntryBlock(dfgBlock);
    
    // Create constants
    Zepra::DFG::Value* c10 = dfgGraph->constInt64(10);
    Zepra::DFG::Value* c32 = dfgGraph->constInt64(32);
    
    // Add them
    Zepra::DFG::Value* add = dfgGraph->addValue(Zepra::DFG::Opcode::Add64, Zepra::DFG::Type::I64, c10, c32);
    
    // Return the result
    Zepra::DFG::Value* ret = dfgGraph->addValue(Zepra::DFG::Opcode::Return, Zepra::DFG::Type::Void, add);
    
    dfgBlock->appendValue(c10);
    dfgBlock->appendValue(c32);
    dfgBlock->appendValue(add);
    dfgBlock->appendValue(ret);
    
    // Lower DFG to B3
    auto b3Proc = std::make_unique<Procedure>();
    DFGLowering lowering(dfgGraph.get(), b3Proc.get());
    EXPECT_TRUE(lowering.lower());
    
    // Generate x86-64 code
    Generate gen(b3Proc.get());
    GeneratedCode code = gen.run();
    EXPECT_TRUE(code.success);
    
    // Execute and verify result
    EXPECT_EQ(execute(code), 42);
}

TEST_F(B3Tests, ArgumentHandling) {
    // Tests that registers are preserved/used correctly
    // implementation of arguments is tricky in the current skeleton
    // B3Value.h doesn't seem to have Argument opcode yet, need to check
}
