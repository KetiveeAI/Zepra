/**
 * @file wasm_tests.cpp
 * @brief WebAssembly module parsing, validation, and execution tests
 */
#include <gtest/gtest.h>
#include "wasm/wasm.hpp"
#include <stdexcept>

using namespace Zepra::Wasm;

// =============================================================================
// Module Parsing Tests
// =============================================================================

// Minimal valid WASM module: magic number + version
static const uint8_t MINIMAL_MODULE[] = {
    0x00, 0x61, 0x73, 0x6D,  // \0asm magic
    0x01, 0x00, 0x00, 0x00   // version 1
};

TEST(WasmParsingTests, ParseMinimalModule) {
    auto module = WasmModule::parse(MINIMAL_MODULE, sizeof(MINIMAL_MODULE));
    ASSERT_NE(module, nullptr);
    EXPECT_EQ(module->types().size(), 0);
    EXPECT_EQ(module->imports().size(), 0);
    EXPECT_EQ(module->exports().size(), 0);
}

TEST(WasmParsingTests, RejectInvalidMagic) {
    uint8_t invalid[] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
    EXPECT_THROW(WasmModule::parse(invalid, sizeof(invalid)), std::runtime_error);
}

TEST(WasmParsingTests, RejectTruncatedModule) {
    uint8_t truncated[] = {0x00, 0x61, 0x73};  // Missing magic byte and version
    EXPECT_THROW(WasmModule::parse(truncated, sizeof(truncated)), std::runtime_error);
}

// Module with type section: (type $t (func (param i32) (result i32)))
static const uint8_t MODULE_WITH_TYPE[] = {
    0x00, 0x61, 0x73, 0x6D,  // magic
    0x01, 0x00, 0x00, 0x00,  // version
    0x01,                     // type section id
    0x06,                     // section size
    0x01,                     // 1 type
    0x60,                     // func type
    0x01, 0x7F,               // 1 param: i32
    0x01, 0x7F                // 1 result: i32
};

TEST(WasmParsingTests, ParseTypeSection) {
    auto module = WasmModule::parse(MODULE_WITH_TYPE, sizeof(MODULE_WITH_TYPE));
    ASSERT_NE(module, nullptr);
    EXPECT_EQ(module->types().size(), 1);
    
    const auto& funcType = module->types()[0];
    EXPECT_EQ(funcType.params.size(), 1);
    EXPECT_EQ(funcType.results.size(), 1);
}

// =============================================================================
// Memory Tests
// =============================================================================

TEST(WasmMemoryTests, CreateMemory) {
    MemoryType type;
    type.limits.min = 1;
    type.limits.max = 10;
    type.limits.hasMax = true;
    
    WasmMemory memory(type);
    EXPECT_EQ(memory.pageCount(), 1);
    EXPECT_EQ(memory.byteLength(), 65536);  // 1 page = 64KB
}

TEST(WasmMemoryTests, GrowMemory) {
    MemoryType type;
    type.limits.min = 1;
    type.limits.max = 10;
    type.limits.hasMax = true;
    
    WasmMemory memory(type);
    size_t oldPages = memory.grow(2);  // Grow by 2 pages
    
    EXPECT_EQ(oldPages, 1);
    EXPECT_EQ(memory.pageCount(), 3);
}

TEST(WasmMemoryTests, GrowBeyondMax) {
    MemoryType type;
    type.limits.min = 1;
    type.limits.max = 2;
    type.limits.hasMax = true;
    
    WasmMemory memory(type);
    size_t result = memory.grow(5);  // Try to grow beyond max
    
    EXPECT_EQ(result, static_cast<size_t>(-1));  // Should fail
    EXPECT_EQ(memory.pageCount(), 1);  // Unchanged
}

TEST(WasmMemoryTests, LoadStoreI32) {
    MemoryType type;
    type.limits.min = 1;
    
    WasmMemory memory(type);
    
    memory.store<int32_t>(0, 42);
    int32_t value = memory.load<int32_t>(0);
    EXPECT_EQ(value, 42);
    
    memory.store<int32_t>(100, -12345);
    value = memory.load<int32_t>(100);
    EXPECT_EQ(value, -12345);
}

TEST(WasmMemoryTests, LoadStoreI64) {
    MemoryType type;
    type.limits.min = 1;
    
    WasmMemory memory(type);
    
    int64_t large = 0x123456789ABCDEF0LL;
    memory.store<int64_t>(0, large);
    int64_t value = memory.load<int64_t>(0);
    EXPECT_EQ(value, large);
}

TEST(WasmMemoryTests, LoadStoreFloat) {
    MemoryType type;
    type.limits.min = 1;
    
    WasmMemory memory(type);
    
    memory.store<float>(0, 3.14159f);
    float f = memory.load<float>(0);
    EXPECT_FLOAT_EQ(f, 3.14159f);
    
    memory.store<double>(8, 2.718281828);
    double d = memory.load<double>(8);
    EXPECT_DOUBLE_EQ(d, 2.718281828);
}

// =============================================================================
// Table Tests
// =============================================================================

TEST(WasmTableTests, CreateTable) {
    TableType type;
    type.elemType = ValType::FuncRef;
    type.limits.min = 10;
    type.limits.max = 100;
    type.limits.hasMax = true;
    
    WasmTable table(type);
    EXPECT_EQ(table.length(), 10);
}

TEST(WasmTableTests, GrowTable) {
    TableType type;
    type.elemType = ValType::FuncRef;
    type.limits.min = 5;
    type.limits.max = 20;
    type.limits.hasMax = true;
    
    WasmTable table(type);
    size_t oldSize = table.grow(5, nullptr);
    
    EXPECT_EQ(oldSize, 5);
    EXPECT_EQ(table.length(), 10);
}

TEST(WasmTableTests, SetGetElement) {
    TableType type;
    type.elemType = ValType::FuncRef;
    type.limits.min = 10;
    
    WasmTable table(type);
    
    void* dummy = reinterpret_cast<void*>(0x12345678);
    table.setElement(5, dummy);
    void* retrieved = table.getElement(5);
    
    EXPECT_EQ(retrieved, dummy);
}

// =============================================================================
// Global Tests
// =============================================================================

TEST(WasmGlobalTests, CreateGlobal) {
    GlobalType type;
    type.valType = ValType::I32;
    type.mutable_ = true;
    
    WasmValue initValue = WasmValue::fromI32(100);
    WasmGlobal global(type, initValue);
    
    EXPECT_EQ(global.getValue().i32, 100);
}

TEST(WasmGlobalTests, MutableGlobal) {
    GlobalType type;
    type.valType = ValType::I32;
    type.mutable_ = true;
    
    WasmValue initValue = WasmValue::fromI32(0);
    WasmGlobal global(type, initValue);
    
    global.setValue(WasmValue::fromI32(42));
    EXPECT_EQ(global.getValue().i32, 42);
}

// =============================================================================
// Value Conversion Tests
// =============================================================================

TEST(WasmValueTests, I32Value) {
    WasmValue v = WasmValue::fromI32(-42);
    EXPECT_EQ(v.type, ValType::I32);
    EXPECT_EQ(v.i32, -42);
}

TEST(WasmValueTests, I64Value) {
    WasmValue v = WasmValue::fromI64(0x7FFFFFFFFFFFFFFFLL);
    EXPECT_EQ(v.type, ValType::I64);
    EXPECT_EQ(v.i64, 0x7FFFFFFFFFFFFFFFLL);
}

TEST(WasmValueTests, F32Value) {
    WasmValue v = WasmValue::fromF32(1.5f);
    EXPECT_EQ(v.type, ValType::F32);
    EXPECT_FLOAT_EQ(v.f32, 1.5f);
}

TEST(WasmValueTests, F64Value) {
    WasmValue v = WasmValue::fromF64(-0.5);
    EXPECT_EQ(v.type, ValType::F64);
    EXPECT_DOUBLE_EQ(v.f64, -0.5);
}
