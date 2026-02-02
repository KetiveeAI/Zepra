/**
 * @file wasm.hpp
 * @brief WebAssembly support for ZepraScript
 * 
 * Implements WebAssembly 1.0 (MVP) with extensions:
 * - Module parsing and validation
 * - Compilation to native code or interpreter
 * - Memory/Table instances
 * - Import/Export binding with JavaScript
 */

#pragma once

#include "../runtime/value.hpp"
#include "../runtime/object.hpp"
#include "../runtime/promise.hpp"
#include <cstdint>
#include <cstring>
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>

namespace Zepra::Wasm {

using Runtime::Value;
using Runtime::Object;
using Runtime::Promise;
using Runtime::Function;

// Forward declarations
class WasmModule;
class WasmInstance;
class WasmMemory;
class WasmTable;
class WasmGlobal;

// =============================================================================
// WASM Types
// =============================================================================

/**
 * @brief WebAssembly value types
 */
enum class ValType : uint8_t {
    I32 = 0x7F,
    I64 = 0x7E,
    F32 = 0x7D,
    F64 = 0x7C,
    V128 = 0x7B,      // SIMD
    FuncRef = 0x70,
    ExternRef = 0x6F
};

/**
 * @brief WebAssembly function signature
 */
struct FuncType {
    std::vector<ValType> params;
    std::vector<ValType> results;
    
    bool operator==(const FuncType& other) const {
        return params == other.params && results == other.results;
    }
};

/**
 * @brief WebAssembly limits (for memories and tables)
 */
struct Limits {
    uint32_t min = 0;
    uint32_t max = UINT32_MAX;  // No max if UINT32_MAX
    bool hasMax = false;
};

/**
 * @brief Memory type
 */
struct MemoryType {
    Limits limits;
    bool shared = false;  // threads proposal
};

/**
 * @brief Table type
 */
struct TableType {
    ValType elemType = ValType::FuncRef;
    Limits limits;
};

/**
 * @brief Global type
 */
struct GlobalType {
    ValType valType;
    bool mutable_ = false;
};

/**
 * @brief Import descriptor
 */
struct ImportDesc {
    enum class Kind { Func, Table, Memory, Global } kind = Kind::Func;
    uint32_t funcTypeIdx = 0;
    TableType tableType;
    MemoryType memoryType;
    GlobalType globalType;
};

/**
 * @brief Export descriptor
 */
struct ExportDesc {
    enum class Kind { Func, Table, Memory, Global } kind;
    uint32_t idx;
};

/**
 * @brief Import entry
 */
struct Import {
    std::string module;
    std::string name;
    ImportDesc desc;
};

/**
 * @brief Export entry
 */
struct Export {
    std::string name;
    ExportDesc desc;
};

// =============================================================================
// WASM Runtime Value
// =============================================================================

/**
 * @brief WASM runtime value (distinct from JS Value)
 */
struct WasmValue {
    ValType type;
    union {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
        void* ref;
        uint8_t v128[16];  // SIMD 128-bit vector
    };
    
    static WasmValue fromI32(int32_t v) { WasmValue w; w.type = ValType::I32; w.i32 = v; return w; }
    static WasmValue fromI64(int64_t v) { WasmValue w; w.type = ValType::I64; w.i64 = v; return w; }
    static WasmValue fromF32(float v) { WasmValue w; w.type = ValType::F32; w.f32 = v; return w; }
    static WasmValue fromF64(double v) { WasmValue w; w.type = ValType::F64; w.f64 = v; return w; }
    static WasmValue fromV128(const uint8_t data[16]) { 
        WasmValue w; 
        w.type = ValType::V128; 
        std::memcpy(w.v128, data, 16); 
        return w; 
    }
    
    Value toJSValue() const;
    static WasmValue fromJSValue(Value v, ValType expected);
};

// =============================================================================
// WASM Memory
// =============================================================================

/**
 * @brief WebAssembly.Memory - Linear memory buffer
 */
class WasmMemory : public Object {
public:
    static constexpr size_t PAGE_SIZE = 65536;  // 64KB
    
    explicit WasmMemory(const MemoryType& type);
    ~WasmMemory();
    
    // JS API
    size_t grow(size_t deltaPages);
    size_t pageCount() const { return currentPages_; }
    uint8_t* buffer() { return data_; }
    size_t byteLength() const { return currentPages_ * PAGE_SIZE; }
    
    // Memory access (with bounds checking)
    template<typename T>
    T load(uint32_t offset) const;
    
    template<typename T>
    void store(uint32_t offset, T value);
    
    // Get ArrayBuffer for JS
    Object* toArrayBuffer();
    
private:
    uint8_t* data_ = nullptr;
    size_t currentPages_ = 0;
    size_t maxPages_ = 0;
    bool shared_ = false;
};

// =============================================================================
// WASM Table
// =============================================================================

/**
 * @brief WebAssembly.Table - Function table
 */
class WasmTable : public Object {
public:
    explicit WasmTable(const TableType& type);
    
    // JS API
    size_t grow(size_t delta, void* initValue = nullptr);
    void* getElement(uint32_t idx) const;
    void setElement(uint32_t idx, void* value);
    size_t length() const { return elements_.size(); }
    
private:
    ValType elemType_;
    std::vector<void*> elements_;
    size_t maxSize_;
};

// =============================================================================
// WASM Global
// =============================================================================

/**
 * @brief WebAssembly.Global - Global variable
 */
class WasmGlobal : public Object {
public:
    explicit WasmGlobal(const GlobalType& type, WasmValue initialValue);
    
    WasmValue getValue() const { return value_; }
    void setValue(WasmValue v);
    
    bool isMutable() const { return mutable_; }
    ValType valType() const { return valType_; }
    
private:
    ValType valType_;
    bool mutable_;
    WasmValue value_;
};

// =============================================================================
// WASM Module
// =============================================================================

/**
 * @brief Parsed WebAssembly module
 */
class WasmModule : public Object {
public:
    WasmModule();
    
    // Parsing
    static WasmModule* parse(const uint8_t* bytes, size_t length);
    static WasmModule* parseAsync(const uint8_t* bytes, size_t length, Promise* promise);
    
    // Validation
    bool validate() const;
    
    // Module sections
    const std::vector<FuncType>& types() const { return types_; }
    const std::vector<Import>& imports() const { return imports_; }
    const std::vector<Export>& exports() const { return exports_; }
    
    // Custom sections
    std::vector<uint8_t> customSection(const std::string& name) const;
    
    // Code access (for interpreter)
    struct FuncCode {
        std::vector<std::pair<uint32_t, ValType>> locals;
        std::vector<uint8_t> body;
    };
    const std::vector<FuncCode>& code() const { return code_; }
    
private:
    friend class WasmParser;
    friend class WasmInstance;
    friend class WasmInterpreter;
    
    // Type section
    std::vector<FuncType> types_;
    
    // Import section
    std::vector<Import> imports_;
    
    // Function section (type indices for internal functions)
    std::vector<uint32_t> funcTypeIndices_;
    
    // Table section
    std::vector<TableType> tables_;
    
    // Memory section
    std::vector<MemoryType> memories_;
    
    // Global section
    std::vector<GlobalType> globalTypes_;
    std::vector<std::vector<uint8_t>> globalInits_;
    
    // Export section
    std::vector<Export> exports_;
    
    // Start function
    int32_t startFuncIdx_ = -1;
    
    // Element section (for table initialization)
    struct ElemSegment {
        uint32_t tableIdx;
        std::vector<uint8_t> offsetExpr;
        std::vector<uint32_t> funcIndices;
    };
    std::vector<ElemSegment> elemSegments_;
    
    // Code section (moved to public for interpreter access)
    std::vector<FuncCode> code_;
    
    // Data section
    struct DataSegment {
        uint32_t memoryIdx;
        std::vector<uint8_t> offsetExpr;
        std::vector<uint8_t> data;
    };
    std::vector<DataSegment> dataSegments_;
    
    // Custom sections
    std::unordered_map<std::string, std::vector<uint8_t>> customSections_;
};

// =============================================================================
// WASM Instance
// =============================================================================

/**
 * @brief Import object for instantiation
 */
using ImportObject = std::unordered_map<std::string, 
                     std::unordered_map<std::string, Value>>;

/**
 * @brief Instantiated WebAssembly module
 */
class WasmInstance : public Object {
public:
    WasmInstance(WasmModule* module, const ImportObject& imports);
    
    // Static instantiation
    static WasmInstance* instantiate(WasmModule* module, const ImportObject& imports);
    static Promise* instantiateAsync(WasmModule* module, const ImportObject& imports);
    
    // Get exports
    Object* exports() { return exports_; }
    
    // Call exported function
    WasmValue callExport(const std::string& name, const std::vector<WasmValue>& args);
    
    // Access memories/tables/globals
    WasmMemory* getMemory(uint32_t idx);
    WasmTable* getTable(uint32_t idx);
    WasmGlobal* getGlobal(uint32_t idx);
    
private:
    friend class WasmInterpreter;
    
    WasmModule* module_;
    Object* exports_;
    
    // Instantiated components
    std::vector<std::unique_ptr<WasmMemory>> memories_;
    std::vector<std::unique_ptr<WasmTable>> tables_;
    std::vector<std::unique_ptr<WasmGlobal>> globals_;
    
    // Resolved imports
    std::vector<Value> importedFuncs_;
    std::vector<WasmMemory*> importedMemories_;
    std::vector<WasmTable*> importedTables_;
    std::vector<WasmGlobal*> importedGlobals_;
    
    // Function instances (imported + local)
    std::vector<Value> functions_;
    
    void initializeMemories();
    void initializeTables();
    void initializeGlobals();
    void resolveImports(const ImportObject& imports);
    void buildExports();
    void executeStartFunction();
};

// =============================================================================
// WASM Parser
// =============================================================================

/**
 * @brief Binary format parser
 */
class WasmParser {
public:
    explicit WasmParser(const uint8_t* bytes, size_t length);
    
    WasmModule* parse();
    
private:
    const uint8_t* data_;
    size_t length_;
    size_t pos_ = 0;
    
    // Reading primitives
    uint8_t readByte();
    uint32_t readU32();
    uint64_t readU64();
    int32_t readI32();
    int64_t readI64();
    float readF32();
    double readF64();
    uint32_t readVarU32();
    int32_t readVarI32();
    int64_t readVarI64();
    std::string readName();
    std::vector<uint8_t> readBytes(size_t count);
    
    // Section parsing
    void parseSection(WasmModule* module, uint8_t sectionId, size_t sectionLen);
    void parseTypeSection(WasmModule* module);
    void parseImportSection(WasmModule* module);
    void parseFunctionSection(WasmModule* module);
    void parseTableSection(WasmModule* module);
    void parseMemorySection(WasmModule* module);
    void parseGlobalSection(WasmModule* module);
    void parseExportSection(WasmModule* module);
    void parseStartSection(WasmModule* module);
    void parseElementSection(WasmModule* module);
    void parseCodeSection(WasmModule* module);
    void parseDataSection(WasmModule* module);
    void parseCustomSection(WasmModule* module, const std::string& name, size_t len);
    
    // Type parsing
    ValType readValType();
    FuncType readFuncType();
    Limits readLimits();
    TableType readTableType();
    MemoryType readMemoryType();
    GlobalType readGlobalType();
};

// =============================================================================
// WASM Interpreter
// =============================================================================

/**
 * @brief Stack-based WASM interpreter
 */
class WasmInterpreter {
public:
    explicit WasmInterpreter(WasmInstance* instance);
    
    WasmValue execute(uint32_t funcIdx, const std::vector<WasmValue>& args);
    
private:
    WasmInstance* instance_;
    
    // Value stack
    std::vector<WasmValue> stack_;
    
    // Control stack for blocks
    struct Label {
        size_t arity;
        size_t stackHeight;
        const uint8_t* continuation;
        bool isTry = false;      // Exception handling: try block
        bool isCatch = false;    // Exception handling: catch block
    };
    std::vector<Label> controlStack_;
    
    // Current function locals
    std::vector<WasmValue> locals_;
    
    // Instruction execution
    void executeInstruction(uint8_t opcode, const uint8_t*& ip);
    void executeSimdInstruction(uint32_t simdOp, const uint8_t*& ip);
    
    // Stack operations
    void push(WasmValue v) { stack_.push_back(v); }
    WasmValue pop() { WasmValue v = stack_.back(); stack_.pop_back(); return v; }
    
    // Memory operations
    template<typename T> T loadMem(uint32_t addr);
    template<typename T> void storeMem(uint32_t addr, T value);
};

// =============================================================================
// WebAssembly JavaScript API
// =============================================================================

/**
 * @brief WebAssembly namespace object for JavaScript
 */
class WebAssemblyObject : public Object {
public:
    WebAssemblyObject();
    
    // WebAssembly.compile(bytes) -> Promise<Module>
    static Promise* compile(const std::vector<uint8_t>& bytes);
    
    // WebAssembly.instantiate(bytes, imports) -> Promise<{module, instance}>
    static Promise* instantiate(const std::vector<uint8_t>& bytes, 
                                const ImportObject& imports);
    
    // WebAssembly.validate(bytes) -> boolean
    static bool validate(const std::vector<uint8_t>& bytes);
    
    // WebAssembly.Module(bytes) -> Module (synchronous)
    static WasmModule* Module(const std::vector<uint8_t>& bytes);
    
    // WebAssembly.Instance(module, imports) -> Instance
    static WasmInstance* Instance(WasmModule* module, const ImportObject& imports);
    
    // WebAssembly.Memory(descriptor) -> Memory
    static WasmMemory* Memory(const MemoryType& descriptor);
    
    // WebAssembly.Table(descriptor) -> Table
    static WasmTable* Table(const TableType& descriptor);
    
    // WebAssembly.Global(descriptor, value) -> Global
    static WasmGlobal* Global(const GlobalType& descriptor, WasmValue value);
};

// =============================================================================
// WASM Opcodes
// =============================================================================

namespace Opcode {
    // Control flow
    constexpr uint8_t UNREACHABLE = 0x00;
    constexpr uint8_t NOP = 0x01;
    constexpr uint8_t BLOCK = 0x02;
    constexpr uint8_t LOOP = 0x03;
    constexpr uint8_t IF = 0x04;
    constexpr uint8_t ELSE = 0x05;
    constexpr uint8_t END = 0x0B;
    constexpr uint8_t BR = 0x0C;
    constexpr uint8_t BR_IF = 0x0D;
    constexpr uint8_t BR_TABLE = 0x0E;
    constexpr uint8_t RETURN = 0x0F;
    constexpr uint8_t CALL = 0x10;
    constexpr uint8_t CALL_INDIRECT = 0x11;
    
    // Parametric
    constexpr uint8_t DROP = 0x1A;
    constexpr uint8_t SELECT = 0x1B;
    
    // Variable
    constexpr uint8_t LOCAL_GET = 0x20;
    constexpr uint8_t LOCAL_SET = 0x21;
    constexpr uint8_t LOCAL_TEE = 0x22;
    constexpr uint8_t GLOBAL_GET = 0x23;
    constexpr uint8_t GLOBAL_SET = 0x24;
    
    // Memory
    constexpr uint8_t I32_LOAD = 0x28;
    constexpr uint8_t I64_LOAD = 0x29;
    constexpr uint8_t F32_LOAD = 0x2A;
    constexpr uint8_t F64_LOAD = 0x2B;
    constexpr uint8_t I32_STORE = 0x36;
    constexpr uint8_t I64_STORE = 0x37;
    constexpr uint8_t F32_STORE = 0x38;
    constexpr uint8_t F64_STORE = 0x39;
    constexpr uint8_t MEMORY_SIZE = 0x3F;
    constexpr uint8_t MEMORY_GROW = 0x40;
    
    // Constants
    constexpr uint8_t I32_CONST = 0x41;
    constexpr uint8_t I64_CONST = 0x42;
    constexpr uint8_t F32_CONST = 0x43;
    constexpr uint8_t F64_CONST = 0x44;
    
    // i32 operations
    constexpr uint8_t I32_EQZ = 0x45;
    constexpr uint8_t I32_EQ = 0x46;
    constexpr uint8_t I32_NE = 0x47;
    constexpr uint8_t I32_LT_S = 0x48;
    constexpr uint8_t I32_LT_U = 0x49;
    constexpr uint8_t I32_GT_S = 0x4A;
    constexpr uint8_t I32_GT_U = 0x4B;
    constexpr uint8_t I32_LE_S = 0x4C;
    constexpr uint8_t I32_LE_U = 0x4D;
    constexpr uint8_t I32_GE_S = 0x4E;
    constexpr uint8_t I32_GE_U = 0x4F;
    
    constexpr uint8_t I32_CLZ = 0x67;
    constexpr uint8_t I32_CTZ = 0x68;
    constexpr uint8_t I32_POPCNT = 0x69;
    constexpr uint8_t I32_ADD = 0x6A;
    constexpr uint8_t I32_SUB = 0x6B;
    constexpr uint8_t I32_MUL = 0x6C;
    constexpr uint8_t I32_DIV_S = 0x6D;
    constexpr uint8_t I32_DIV_U = 0x6E;
    constexpr uint8_t I32_REM_S = 0x6F;
    constexpr uint8_t I32_REM_U = 0x70;
    constexpr uint8_t I32_AND = 0x71;
    constexpr uint8_t I32_OR = 0x72;
    constexpr uint8_t I32_XOR = 0x73;
    constexpr uint8_t I32_SHL = 0x74;
    constexpr uint8_t I32_SHR_S = 0x75;
    constexpr uint8_t I32_SHR_U = 0x76;
    constexpr uint8_t I32_ROTL = 0x77;
    constexpr uint8_t I32_ROTR = 0x78;
    
    // i64 comparisons
    constexpr uint8_t I64_EQZ = 0x50;
    constexpr uint8_t I64_EQ = 0x51;
    constexpr uint8_t I64_NE = 0x52;
    constexpr uint8_t I64_LT_S = 0x53;
    constexpr uint8_t I64_LT_U = 0x54;
    constexpr uint8_t I64_GT_S = 0x55;
    constexpr uint8_t I64_GT_U = 0x56;
    constexpr uint8_t I64_LE_S = 0x57;
    constexpr uint8_t I64_LE_U = 0x58;
    constexpr uint8_t I64_GE_S = 0x59;
    constexpr uint8_t I64_GE_U = 0x5A;
    
    // i64 arithmetic
    constexpr uint8_t I64_CLZ = 0x79;
    constexpr uint8_t I64_CTZ = 0x7A;
    constexpr uint8_t I64_POPCNT = 0x7B;
    constexpr uint8_t I64_ADD = 0x7C;
    constexpr uint8_t I64_SUB = 0x7D;
    constexpr uint8_t I64_MUL = 0x7E;
    constexpr uint8_t I64_DIV_S = 0x7F;
    constexpr uint8_t I64_DIV_U = 0x80;
    constexpr uint8_t I64_REM_S = 0x81;
    constexpr uint8_t I64_REM_U = 0x82;
    constexpr uint8_t I64_AND = 0x83;
    constexpr uint8_t I64_OR = 0x84;
    constexpr uint8_t I64_XOR = 0x85;
    constexpr uint8_t I64_SHL = 0x86;
    constexpr uint8_t I64_SHR_S = 0x87;
    constexpr uint8_t I64_SHR_U = 0x88;
    constexpr uint8_t I64_ROTL = 0x89;
    constexpr uint8_t I64_ROTR = 0x8A;
    
    // f32 comparisons
    constexpr uint8_t F32_EQ = 0x5B;
    constexpr uint8_t F32_NE = 0x5C;
    constexpr uint8_t F32_LT = 0x5D;
    constexpr uint8_t F32_GT = 0x5E;
    constexpr uint8_t F32_LE = 0x5F;
    constexpr uint8_t F32_GE = 0x60;
    
    // f64 comparisons
    constexpr uint8_t F64_EQ = 0x61;
    constexpr uint8_t F64_NE = 0x62;
    constexpr uint8_t F64_LT = 0x63;
    constexpr uint8_t F64_GT = 0x64;
    constexpr uint8_t F64_LE = 0x65;
    constexpr uint8_t F64_GE = 0x66;
    
    // f32 arithmetic
    constexpr uint8_t F32_ABS = 0x8B;
    constexpr uint8_t F32_NEG = 0x8C;
    constexpr uint8_t F32_CEIL = 0x8D;
    constexpr uint8_t F32_FLOOR = 0x8E;
    constexpr uint8_t F32_TRUNC = 0x8F;
    constexpr uint8_t F32_NEAREST = 0x90;
    constexpr uint8_t F32_SQRT = 0x91;
    constexpr uint8_t F32_ADD = 0x92;
    constexpr uint8_t F32_SUB = 0x93;
    constexpr uint8_t F32_MUL = 0x94;
    constexpr uint8_t F32_DIV = 0x95;
    constexpr uint8_t F32_MIN = 0x96;
    constexpr uint8_t F32_MAX = 0x97;
    constexpr uint8_t F32_COPYSIGN = 0x98;
    
    // f64 arithmetic
    constexpr uint8_t F64_ABS = 0x99;
    constexpr uint8_t F64_NEG = 0x9A;
    constexpr uint8_t F64_CEIL = 0x9B;
    constexpr uint8_t F64_FLOOR = 0x9C;
    constexpr uint8_t F64_TRUNC = 0x9D;
    constexpr uint8_t F64_NEAREST = 0x9E;
    constexpr uint8_t F64_SQRT = 0x9F;
    constexpr uint8_t F64_ADD = 0xA0;
    constexpr uint8_t F64_SUB = 0xA1;
    constexpr uint8_t F64_MUL = 0xA2;
    constexpr uint8_t F64_DIV = 0xA3;
    constexpr uint8_t F64_MIN = 0xA4;
    constexpr uint8_t F64_MAX = 0xA5;
    constexpr uint8_t F64_COPYSIGN = 0xA6;
    
    // Conversions
    constexpr uint8_t I32_WRAP_I64 = 0xA7;
    constexpr uint8_t I32_TRUNC_F32_S = 0xA8;
    constexpr uint8_t I32_TRUNC_F32_U = 0xA9;
    constexpr uint8_t I32_TRUNC_F64_S = 0xAA;
    constexpr uint8_t I32_TRUNC_F64_U = 0xAB;
    constexpr uint8_t I64_EXTEND_I32_S = 0xAC;
    constexpr uint8_t I64_EXTEND_I32_U = 0xAD;
    constexpr uint8_t I64_TRUNC_F32_S = 0xAE;
    constexpr uint8_t I64_TRUNC_F32_U = 0xAF;
    constexpr uint8_t I64_TRUNC_F64_S = 0xB0;
    constexpr uint8_t I64_TRUNC_F64_U = 0xB1;
    constexpr uint8_t F32_CONVERT_I32_S = 0xB2;
    constexpr uint8_t F32_CONVERT_I32_U = 0xB3;
    constexpr uint8_t F32_CONVERT_I64_S = 0xB4;
    constexpr uint8_t F32_CONVERT_I64_U = 0xB5;
    constexpr uint8_t F32_DEMOTE_F64 = 0xB6;
    constexpr uint8_t F64_CONVERT_I32_S = 0xB7;
    constexpr uint8_t F64_CONVERT_I32_U = 0xB8;
    constexpr uint8_t F64_CONVERT_I64_S = 0xB9;
    constexpr uint8_t F64_CONVERT_I64_U = 0xBA;
    constexpr uint8_t F64_PROMOTE_F32 = 0xBB;
    
    // Reinterpretations
    constexpr uint8_t I32_REINTERPRET_F32 = 0xBC;
    constexpr uint8_t I64_REINTERPRET_F64 = 0xBD;
    constexpr uint8_t F32_REINTERPRET_I32 = 0xBE;
    constexpr uint8_t F64_REINTERPRET_I64 = 0xBF;
    
    // Prefix bytes
    constexpr uint8_t SIMD_PREFIX = 0xFD;
    
    // SIMD opcodes (following 0xFD prefix, use LEB128 encoded)
    namespace Simd {
        constexpr uint32_t V128_LOAD = 0x00;
        constexpr uint32_t V128_STORE = 0x0B;
        constexpr uint32_t V128_CONST = 0x0C;
        
        // Splat
        constexpr uint32_t I8X16_SPLAT = 0x0F;
        constexpr uint32_t I16X8_SPLAT = 0x10;
        constexpr uint32_t I32X4_SPLAT = 0x11;
        constexpr uint32_t I64X2_SPLAT = 0x12;
        constexpr uint32_t F32X4_SPLAT = 0x13;
        constexpr uint32_t F64X2_SPLAT = 0x14;
        
        // Extract/Replace lane
        constexpr uint32_t I8X16_EXTRACT_LANE_S = 0x15;
        constexpr uint32_t I8X16_EXTRACT_LANE_U = 0x16;
        constexpr uint32_t I8X16_REPLACE_LANE = 0x17;
        constexpr uint32_t I16X8_EXTRACT_LANE_S = 0x18;
        constexpr uint32_t I16X8_EXTRACT_LANE_U = 0x19;
        constexpr uint32_t I16X8_REPLACE_LANE = 0x1A;
        constexpr uint32_t I32X4_EXTRACT_LANE = 0x1B;
        constexpr uint32_t I32X4_REPLACE_LANE = 0x1C;
        constexpr uint32_t I64X2_EXTRACT_LANE = 0x1D;
        constexpr uint32_t I64X2_REPLACE_LANE = 0x1E;
        constexpr uint32_t F32X4_EXTRACT_LANE = 0x1F;
        constexpr uint32_t F32X4_REPLACE_LANE = 0x20;
        constexpr uint32_t F64X2_EXTRACT_LANE = 0x21;
        constexpr uint32_t F64X2_REPLACE_LANE = 0x22;
        
        // Bitwise
        constexpr uint32_t V128_NOT = 0x4D;
        constexpr uint32_t V128_AND = 0x4E;
        constexpr uint32_t V128_ANDNOT = 0x4F;
        constexpr uint32_t V128_OR = 0x50;
        constexpr uint32_t V128_XOR = 0x51;
        constexpr uint32_t V128_BITSELECT = 0x52;
        constexpr uint32_t V128_ANY_TRUE = 0x53;
        
        // i32x4 arithmetic
        constexpr uint32_t I32X4_ADD = 0xAE;
        constexpr uint32_t I32X4_SUB = 0xB1;
        constexpr uint32_t I32X4_MUL = 0xB5;
        
        // i64x2 arithmetic
        constexpr uint32_t I64X2_ADD = 0xCE;
        constexpr uint32_t I64X2_SUB = 0xD1;
        constexpr uint32_t I64X2_MUL = 0xD5;
        
        // f32x4 arithmetic
        constexpr uint32_t F32X4_ADD = 0xE4;
        constexpr uint32_t F32X4_SUB = 0xE5;
        constexpr uint32_t F32X4_MUL = 0xE6;
        constexpr uint32_t F32X4_DIV = 0xE7;
        
        // f64x2 arithmetic
        constexpr uint32_t F64X2_ADD = 0xF0;
        constexpr uint32_t F64X2_SUB = 0xF1;
        constexpr uint32_t F64X2_MUL = 0xF2;
        constexpr uint32_t F64X2_DIV = 0xF3;
        
        // SIMD comparisons
        constexpr uint32_t I8X16_EQ = 0x23;
        constexpr uint32_t I8X16_NE = 0x24;
        constexpr uint32_t I16X8_EQ = 0x2D;
        constexpr uint32_t I16X8_NE = 0x2E;
        constexpr uint32_t I32X4_EQ = 0x37;
        constexpr uint32_t I32X4_NE = 0x38;
        constexpr uint32_t I32X4_LT_S = 0x39;
        constexpr uint32_t I32X4_GT_S = 0x3B;
        constexpr uint32_t I32X4_LE_S = 0x3D;
        constexpr uint32_t I32X4_GE_S = 0x3F;
        constexpr uint32_t F32X4_EQ = 0x41;
        constexpr uint32_t F32X4_NE = 0x42;
        constexpr uint32_t F32X4_LT = 0x43;
        constexpr uint32_t F32X4_GT = 0x44;
        constexpr uint32_t F32X4_LE = 0x45;
        constexpr uint32_t F32X4_GE = 0x46;
        constexpr uint32_t F64X2_EQ = 0x47;
        constexpr uint32_t F64X2_NE = 0x48;
        constexpr uint32_t F64X2_LT = 0x49;
        constexpr uint32_t F64X2_GT = 0x4A;
        constexpr uint32_t F64X2_LE = 0x4B;
        constexpr uint32_t F64X2_GE = 0x4C;
        
        // SIMD shuffle/swizzle
        constexpr uint32_t I8X16_SHUFFLE = 0x0D;
        constexpr uint32_t I8X16_SWIZZLE = 0x0E;
    }
    
    // Exception handling opcodes (0x06, 0x07, 0x08, 0x0A, 0x19)
    constexpr uint8_t TRY = 0x06;
    constexpr uint8_t CATCH = 0x07;
    constexpr uint8_t THROW = 0x08;
    constexpr uint8_t RETHROW = 0x09;
    constexpr uint8_t CATCH_ALL = 0x19;
    constexpr uint8_t DELEGATE = 0x18;
    constexpr uint8_t TRY_TABLE = 0x1F;
}

} // namespace Zepra::Wasm
