/**
 * @file wasm.cpp
 * @brief WebAssembly implementation
 */

#include "wasm/wasm.hpp"
#include <cstring>
#include <stdexcept>
#include <cmath>

namespace Zepra::Wasm {

// =============================================================================
// WasmValue
// =============================================================================

Value WasmValue::toJSValue() const {
    switch (type) {
        case ValType::I32: return Value::number(static_cast<double>(i32));
        case ValType::I64: return Value::number(static_cast<double>(i64));
        case ValType::F32: return Value::number(static_cast<double>(f32));
        case ValType::F64: return Value::number(f64);
        default: return Value::undefined();
    }
}

WasmValue WasmValue::fromJSValue(Value v, ValType expected) {
    WasmValue result;
    result.type = expected;
    
    double num = v.isNumber() ? v.asNumber() : 0.0;
    
    switch (expected) {
        case ValType::I32:
            result.i32 = static_cast<int32_t>(num);
            break;
        case ValType::I64:
            result.i64 = static_cast<int64_t>(num);
            break;
        case ValType::F32:
            result.f32 = static_cast<float>(num);
            break;
        case ValType::F64:
            result.f64 = num;
            break;
        default:
            break;
    }
    return result;
}

// =============================================================================
// WasmMemory
// =============================================================================

WasmMemory::WasmMemory(const MemoryType& type) 
    : currentPages_(type.limits.min)
    , maxPages_(type.limits.hasMax ? type.limits.max : 65536)
    , shared_(type.shared) {
    
    if (currentPages_ > 0) {
        data_ = static_cast<uint8_t*>(std::calloc(currentPages_ * PAGE_SIZE, 1));
    }
}

WasmMemory::~WasmMemory() {
    if (data_) {
        std::free(data_);
    }
}

size_t WasmMemory::grow(size_t deltaPages) {
    size_t oldPages = currentPages_;
    size_t newPages = oldPages + deltaPages;
    
    if (newPages > maxPages_) {
        return SIZE_MAX;  // -1 in unsigned
    }
    
    uint8_t* newData = static_cast<uint8_t*>(
        std::realloc(data_, newPages * PAGE_SIZE));
    
    if (!newData && newPages > 0) {
        return SIZE_MAX;
    }
    
    // Zero-initialize new pages
    if (newData && deltaPages > 0) {
        std::memset(newData + oldPages * PAGE_SIZE, 0, deltaPages * PAGE_SIZE);
    }
    
    data_ = newData;
    currentPages_ = newPages;
    return oldPages;
}

template<typename T>
T WasmMemory::load(uint32_t offset) const {
    if (offset + sizeof(T) > byteLength()) {
        throw std::runtime_error("Memory access out of bounds");
    }
    T value;
    std::memcpy(&value, data_ + offset, sizeof(T));
    return value;
}

template<typename T>
void WasmMemory::store(uint32_t offset, T value) {
    if (offset + sizeof(T) > byteLength()) {
        throw std::runtime_error("Memory access out of bounds");
    }
    std::memcpy(data_ + offset, &value, sizeof(T));
}

// Explicit instantiations
template int32_t WasmMemory::load<int32_t>(uint32_t) const;
template int64_t WasmMemory::load<int64_t>(uint32_t) const;
template float WasmMemory::load<float>(uint32_t) const;
template double WasmMemory::load<double>(uint32_t) const;
template void WasmMemory::store<int32_t>(uint32_t, int32_t);
template void WasmMemory::store<int64_t>(uint32_t, int64_t);
template void WasmMemory::store<float>(uint32_t, float);
template void WasmMemory::store<double>(uint32_t, double);

// =============================================================================
// WasmTable
// =============================================================================

WasmTable::WasmTable(const TableType& type)
    : elemType_(type.elemType)
    , maxSize_(type.limits.hasMax ? type.limits.max : UINT32_MAX) {
    elements_.resize(type.limits.min, nullptr);
}

size_t WasmTable::grow(size_t delta, void* initValue) {
    size_t oldSize = elements_.size();
    size_t newSize = oldSize + delta;
    
    if (newSize > maxSize_) {
        return SIZE_MAX;
    }
    
    elements_.resize(newSize, initValue);
    return oldSize;
}

void* WasmTable::getElement(uint32_t idx) const {
    if (idx >= elements_.size()) {
        throw std::runtime_error("Table index out of bounds");
    }
    return elements_[idx];
}

void WasmTable::setElement(uint32_t idx, void* value) {
    if (idx >= elements_.size()) {
        throw std::runtime_error("Table index out of bounds");
    }
    elements_[idx] = value;
}

// =============================================================================
// WasmGlobal
// =============================================================================

WasmGlobal::WasmGlobal(const GlobalType& type, WasmValue initialValue)
    : valType_(type.valType)
    , mutable_(type.mutable_)
    , value_(initialValue) {
}

void WasmGlobal::setValue(WasmValue v) {
    if (!mutable_) {
        throw std::runtime_error("Cannot set immutable global");
    }
    value_ = v;
}

// =============================================================================
// WasmModule
// =============================================================================

WasmModule::WasmModule() = default;

std::unique_ptr<WasmModule> WasmModule::parse(const uint8_t* bytes, size_t length) {
    WasmParser parser(bytes, length);
    return parser.parse();
}

bool WasmModule::validate() const {
    // Basic validation
    // Check type indices are valid
    for (uint32_t idx : funcTypeIndices_) {
        if (idx >= types_.size()) return false;
    }
    return true;
}

std::vector<uint8_t> WasmModule::customSection(const std::string& name) const {
    auto it = customSections_.find(name);
    if (it != customSections_.end()) {
        return it->second;
    }
    return {};
}

// =============================================================================
// WasmParser
// =============================================================================

WasmParser::WasmParser(const uint8_t* bytes, size_t length)
    : data_(bytes), length_(length), pos_(0) {
}

std::unique_ptr<WasmModule> WasmParser::parse() {
    auto module = std::make_unique<WasmModule>();
    
    // Check magic number
    if (length_ < 8) {
        throw std::runtime_error("Invalid WASM: too short");
    }
    
    uint32_t magic = readU32();
    if (magic != 0x6D736100) {  // "\0asm"
        throw std::runtime_error("Invalid WASM magic number");
    }
    
    uint32_t version = readU32();
    if (version != 1) {
        throw std::runtime_error("Unsupported WASM version");
    }
    
    // Parse sections
    while (pos_ < length_) {
        uint8_t sectionId = readByte();
        uint32_t sectionLen = readVarU32();
        size_t sectionEnd = pos_ + sectionLen;
        
        parseSection(module.get(), sectionId, sectionLen);
        
        pos_ = sectionEnd;  // Skip any unread bytes
    }
    
    return module;
}

// Reading primitives
uint8_t WasmParser::readByte() {
    if (pos_ >= length_) throw std::runtime_error("Unexpected end of WASM");
    return data_[pos_++];
}

uint32_t WasmParser::readU32() {
    uint32_t result = 0;
    for (int i = 0; i < 4; i++) {
        result |= static_cast<uint32_t>(readByte()) << (i * 8);
    }
    return result;
}

uint64_t WasmParser::readU64() {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result |= static_cast<uint64_t>(readByte()) << (i * 8);
    }
    return result;
}

float WasmParser::readF32() {
    uint32_t bits = readU32();
    float result;
    std::memcpy(&result, &bits, sizeof(float));
    return result;
}

double WasmParser::readF64() {
    uint64_t bits = readU64();
    double result;
    std::memcpy(&result, &bits, sizeof(double));
    return result;
}

uint32_t WasmParser::readVarU32() {
    uint32_t result = 0;
    uint32_t shift = 0;
    uint8_t byte;
    do {
        byte = readByte();
        result |= static_cast<uint32_t>(byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    return result;
}

int32_t WasmParser::readVarI32() {
    int32_t result = 0;
    uint32_t shift = 0;
    uint8_t byte;
    do {
        byte = readByte();
        result |= static_cast<int32_t>(byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    
    if ((shift < 32) && (byte & 0x40)) {
        result |= (~0u << shift);
    }
    return result;
}

int64_t WasmParser::readVarI64() {
    int64_t result = 0;
    uint64_t shift = 0;
    uint8_t byte;
    do {
        byte = readByte();
        result |= static_cast<int64_t>(byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    
    if ((shift < 64) && (byte & 0x40)) {
        result |= (~0ull << shift);
    }
    return result;
}

std::string WasmParser::readName() {
    uint32_t len = readVarU32();
    std::string result;
    result.reserve(len);
    for (uint32_t i = 0; i < len; i++) {
        result.push_back(static_cast<char>(readByte()));
    }
    return result;
}

std::vector<uint8_t> WasmParser::readBytes(size_t count) {
    std::vector<uint8_t> result(count);
    for (size_t i = 0; i < count; i++) {
        result[i] = readByte();
    }
    return result;
}

ValType WasmParser::readValType() {
    return static_cast<ValType>(readByte());
}

FuncType WasmParser::readFuncType() {
    FuncType ft;
    uint8_t form = readByte();
    if (form != 0x60) {
        throw std::runtime_error("Expected function type");
    }
    
    uint32_t paramCount = readVarU32();
    ft.params.reserve(paramCount);
    for (uint32_t i = 0; i < paramCount; i++) {
        ft.params.push_back(readValType());
    }
    
    uint32_t resultCount = readVarU32();
    ft.results.reserve(resultCount);
    for (uint32_t i = 0; i < resultCount; i++) {
        ft.results.push_back(readValType());
    }
    
    return ft;
}

Limits WasmParser::readLimits() {
    Limits lim;
    uint8_t flags = readByte();
    lim.min = readVarU32();
    lim.hasMax = (flags & 0x01) != 0;
    if (lim.hasMax) {
        lim.max = readVarU32();
    }
    return lim;
}

TableType WasmParser::readTableType() {
    TableType tt;
    tt.elemType = readValType();
    tt.limits = readLimits();
    return tt;
}

MemoryType WasmParser::readMemoryType() {
    MemoryType mt;
    mt.limits = readLimits();
    return mt;
}

GlobalType WasmParser::readGlobalType() {
    GlobalType gt;
    gt.valType = readValType();
    gt.mutable_ = (readByte() == 0x01);
    return gt;
}

void WasmParser::parseSection(WasmModule* module, uint8_t sectionId, size_t sectionLen) {
    switch (sectionId) {
        case 0:  // Custom
            {
                std::string name = readName();
                size_t remaining = sectionLen - name.length() - 1;
                parseCustomSection(module, name, remaining);
            }
            break;
        case 1: parseTypeSection(module); break;
        case 2: parseImportSection(module); break;
        case 3: parseFunctionSection(module); break;
        case 4: parseTableSection(module); break;
        case 5: parseMemorySection(module); break;
        case 6: parseGlobalSection(module); break;
        case 7: parseExportSection(module); break;
        case 8: parseStartSection(module); break;
        case 9: parseElementSection(module); break;
        case 10: parseCodeSection(module); break;
        case 11: parseDataSection(module); break;
        default:
            // Skip unknown section
            break;
    }
}

void WasmParser::parseTypeSection(WasmModule* module) {
    uint32_t count = readVarU32();
    module->types_.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        module->types_.push_back(readFuncType());
    }
}

void WasmParser::parseImportSection(WasmModule* module) {
    uint32_t count = readVarU32();
    module->imports_.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        Import imp;
        imp.module = readName();
        imp.name = readName();
        uint8_t kind = readByte();
        imp.desc.kind = static_cast<ImportDesc::Kind>(kind);
        
        switch (kind) {
            case 0: imp.desc.funcTypeIdx = readVarU32(); break;
            case 1: imp.desc.tableType = readTableType(); break;
            case 2: imp.desc.memoryType = readMemoryType(); break;
            case 3: imp.desc.globalType = readGlobalType(); break;
        }
        module->imports_.push_back(imp);
    }
}

void WasmParser::parseFunctionSection(WasmModule* module) {
    uint32_t count = readVarU32();
    module->funcTypeIndices_.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        module->funcTypeIndices_.push_back(readVarU32());
    }
}

void WasmParser::parseTableSection(WasmModule* module) {
    uint32_t count = readVarU32();
    module->tables_.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        module->tables_.push_back(readTableType());
    }
}

void WasmParser::parseMemorySection(WasmModule* module) {
    uint32_t count = readVarU32();
    module->memories_.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        module->memories_.push_back(readMemoryType());
    }
}

void WasmParser::parseGlobalSection(WasmModule* module) {
    uint32_t count = readVarU32();
    module->globalTypes_.reserve(count);
    module->globalInits_.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        module->globalTypes_.push_back(readGlobalType());
        
        // Read init expression (until END opcode)
        std::vector<uint8_t> expr;
        uint8_t byte;
        do {
            byte = readByte();
            expr.push_back(byte);
        } while (byte != Opcode::END);
        module->globalInits_.push_back(expr);
    }
}

void WasmParser::parseExportSection(WasmModule* module) {
    uint32_t count = readVarU32();
    module->exports_.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        Export exp;
        exp.name = readName();
        exp.desc.kind = static_cast<ExportDesc::Kind>(readByte());
        exp.desc.idx = readVarU32();
        module->exports_.push_back(exp);
    }
}

void WasmParser::parseStartSection(WasmModule* module) {
    module->startFuncIdx_ = static_cast<int32_t>(readVarU32());
}

void WasmParser::parseElementSection(WasmModule* module) {
    uint32_t count = readVarU32();
    module->elemSegments_.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        WasmModule::ElemSegment seg;
        seg.tableIdx = readVarU32();
        
        // Read offset expression
        uint8_t byte;
        do {
            byte = readByte();
            seg.offsetExpr.push_back(byte);
        } while (byte != Opcode::END);
        
        uint32_t funcCount = readVarU32();
        seg.funcIndices.reserve(funcCount);
        for (uint32_t j = 0; j < funcCount; j++) {
            seg.funcIndices.push_back(readVarU32());
        }
        module->elemSegments_.push_back(seg);
    }
}

void WasmParser::parseCodeSection(WasmModule* module) {
    uint32_t count = readVarU32();
    module->code_.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        WasmModule::FuncCode code;
        uint32_t bodySize = readVarU32();
        size_t bodyEnd = pos_ + bodySize;
        
        // Local declarations
        uint32_t localGroups = readVarU32();
        for (uint32_t j = 0; j < localGroups; j++) {
            uint32_t localCount = readVarU32();
            ValType localType = readValType();
            code.locals.push_back({localCount, localType});
        }
        
        // Function body
        size_t codeLen = bodyEnd - pos_;
        code.body = readBytes(codeLen);
        
        module->code_.push_back(code);
    }
}

void WasmParser::parseDataSection(WasmModule* module) {
    uint32_t count = readVarU32();
    module->dataSegments_.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        WasmModule::DataSegment seg;
        seg.memoryIdx = readVarU32();
        
        // Read offset expression
        uint8_t byte;
        do {
            byte = readByte();
            seg.offsetExpr.push_back(byte);
        } while (byte != Opcode::END);
        
        uint32_t dataLen = readVarU32();
        seg.data = readBytes(dataLen);
        
        module->dataSegments_.push_back(seg);
    }
}

void WasmParser::parseCustomSection(WasmModule* module, const std::string& name, size_t len) {
    module->customSections_[name] = readBytes(len);
}

// =============================================================================
// WasmInstance
// =============================================================================

WasmInstance::WasmInstance(WasmModule* module, const ImportObject& imports)
    : module_(module) {
    exports_ = new Object();
    
    resolveImports(imports);
    initializeMemories();
    initializeTables();
    initializeGlobals();
    buildExports();
    executeStartFunction();
}

std::unique_ptr<WasmInstance> WasmInstance::instantiate(WasmModule* module, const ImportObject& imports) {
    return std::make_unique<WasmInstance>(module, imports);
}

void WasmInstance::resolveImports(const ImportObject& imports) {
    for (const auto& imp : module_->imports_) {
        auto modIt = imports.find(imp.module);
        if (modIt == imports.end()) {
            throw std::runtime_error("Missing import module: " + imp.module);
        }
        
        auto nameIt = modIt->second.find(imp.name);
        if (nameIt == modIt->second.end()) {
            throw std::runtime_error("Missing import: " + imp.module + "." + imp.name);
        }
        
        Value val = nameIt->second;
        
        switch (imp.desc.kind) {
            case ImportDesc::Kind::Func:
                importedFuncs_.push_back(val);
                functions_.push_back(val);
                break;
            case ImportDesc::Kind::Memory:
                // Would extract WasmMemory from val
                break;
            case ImportDesc::Kind::Table:
                // Would extract WasmTable from val
                break;
            case ImportDesc::Kind::Global:
                // Would extract WasmGlobal from val
                break;
        }
    }
}

// LEB128 signed 32-bit reader for init expressions
static int32_t readLEB128_i32(const std::vector<uint8_t>& data, size_t& pos) {
    int32_t result = 0;
    int shift = 0;
    uint8_t byte;
    do {
        if (pos >= data.size()) return 0;
        byte = data[pos++];
        result |= static_cast<int32_t>(byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    // Sign-extend
    if (shift < 32 && (byte & 0x40)) {
        result |= -(1 << shift);
    }
    return result;
}

static int64_t readLEB128_i64(const std::vector<uint8_t>& data, size_t& pos) {
    int64_t result = 0;
    int shift = 0;
    uint8_t byte;
    do {
        if (pos >= data.size()) return 0;
        byte = data[pos++];
        result |= static_cast<int64_t>(byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    if (shift < 64 && (byte & 0x40)) {
        result |= -(static_cast<int64_t>(1) << shift);
    }
    return result;
}

// Evaluate a WASM init expression to produce WasmValue
static WasmValue evaluateInitExpr(const std::vector<uint8_t>& expr) {
    WasmValue val;
    val.i64 = 0;
    val.type = ValType::I32;
    
    if (expr.empty()) return val;
    
    size_t pos = 0;
    uint8_t opcode = expr[pos++];
    
    switch (opcode) {
        case Opcode::I32_CONST:
            val.type = ValType::I32;
            val.i32 = readLEB128_i32(expr, pos);
            break;
        case Opcode::I64_CONST:
            val.type = ValType::I64;
            val.i64 = readLEB128_i64(expr, pos);
            break;
        case Opcode::F32_CONST:
            val.type = ValType::F32;
            if (pos + sizeof(float) <= expr.size()) {
                std::memcpy(&val.f32, &expr[pos], sizeof(float));
            }
            break;
        case Opcode::F64_CONST:
            val.type = ValType::F64;
            if (pos + sizeof(double) <= expr.size()) {
                std::memcpy(&val.f64, &expr[pos], sizeof(double));
            }
            break;
        default:
            break;
    }
    return val;
}

void WasmInstance::initializeMemories() {
    for (const auto& memType : module_->memories_) {
        memories_.push_back(std::make_unique<WasmMemory>(memType));
    }
    
    // Initialize data segments
    for (const auto& seg : module_->dataSegments_) {
        if (seg.memoryIdx < memories_.size()) {
            WasmMemory* mem = memories_[seg.memoryIdx].get();
            WasmValue offsetVal = evaluateInitExpr(seg.offsetExpr);
            uint32_t offset = static_cast<uint32_t>(offsetVal.i32);
            
            if (offset + seg.data.size() <= mem->byteLength()) {
                std::memcpy(mem->buffer() + offset, seg.data.data(), seg.data.size());
            }
        }
    }
}

void WasmInstance::initializeTables() {
    for (const auto& tabType : module_->tables_) {
        tables_.push_back(std::make_unique<WasmTable>(tabType));
    }
}

void WasmInstance::initializeGlobals() {
    for (size_t i = 0; i < module_->globalTypes_.size(); i++) {
        const auto& gt = module_->globalTypes_[i];
        const auto& initExpr = module_->globalInits_[i];
        
        WasmValue initVal = evaluateInitExpr(initExpr);
        initVal.type = gt.valType;
        
        globals_.push_back(std::make_unique<WasmGlobal>(gt, initVal));
    }
}

void WasmInstance::buildExports() {
    for (const auto& exp : module_->exports_) {
        switch (exp.desc.kind) {
            case ExportDesc::Kind::Func: {
                // Store function index so callExport can invoke it
                uint32_t funcIdx = exp.desc.idx;
                if (funcIdx < functions_.size()) {
                    exports_->set(exp.name, functions_[funcIdx]);
                } else {
                    // Function is defined in this module (beyond imports)
                    // Store the index as a number for now; callExport resolves it
                    exports_->set(exp.name, Value::number(static_cast<double>(funcIdx)));
                }
                break;
            }
            case ExportDesc::Kind::Memory:
                if (exp.desc.idx < memories_.size()) {
                    exports_->set(exp.name, Value::object(memories_[exp.desc.idx].get()));
                }
                break;
            case ExportDesc::Kind::Table:
                if (exp.desc.idx < tables_.size()) {
                    exports_->set(exp.name, Value::object(tables_[exp.desc.idx].get()));
                }
                break;
            case ExportDesc::Kind::Global:
                if (exp.desc.idx < globals_.size()) {
                    exports_->set(exp.name, Value::object(globals_[exp.desc.idx].get()));
                }
                break;
        }
    }
}

void WasmInstance::executeStartFunction() {
    if (module_->startFuncIdx_ >= 0) {
        uint32_t funcIdx = static_cast<uint32_t>(module_->startFuncIdx_);
        WasmInterpreter interp(this);
        interp.execute(funcIdx, {});  // start function takes no args
    }
}

WasmMemory* WasmInstance::getMemory(uint32_t idx) {
    if (idx < memories_.size()) {
        return memories_[idx].get();
    }
    return nullptr;
}

WasmTable* WasmInstance::getTable(uint32_t idx) {
    if (idx < tables_.size()) {
        return tables_[idx].get();
    }
    return nullptr;
}

WasmGlobal* WasmInstance::getGlobal(uint32_t idx) {
    if (idx < globals_.size()) {
        return globals_[idx].get();
    }
    return nullptr;
}

// =============================================================================
// WasmInterpreter
// =============================================================================

WasmInterpreter::WasmInterpreter(WasmInstance* instance)
    : instance_(instance) {
}

WasmValue WasmInterpreter::execute(uint32_t funcIdx, const std::vector<WasmValue>& args) {
    // Push arguments as locals
    locals_ = args;
    stack_.clear();
    controlStack_.clear();
    
    // Get function code
    auto* module = instance_->module_;
    size_t importedFuncCount = 0;
    for (const auto& imp : module->imports_) {
        if (imp.desc.kind == ImportDesc::Kind::Func) {
            importedFuncCount++;
        }
    }
    
    if (funcIdx < importedFuncCount) {
        // Call imported function
        return WasmValue::fromI32(0);  // Placeholder
    }
    
    size_t codeIdx = funcIdx - importedFuncCount;
    if (codeIdx >= module->code_.size()) {
        throw std::runtime_error("Invalid function index");
    }
    
    const auto& code = module->code_[codeIdx];
    
    // Expand locals
    for (const auto& [count, type] : code.locals) {
        for (uint32_t i = 0; i < count; i++) {
            WasmValue local;
            local.type = type;
            local.i64 = 0;
            locals_.push_back(local);
        }
    }
    
    // Execute bytecode
    const uint8_t* ip = code.body.data();
    const uint8_t* end = ip + code.body.size();
    
    while (ip < end) {
        uint8_t opcode = *ip++;
        executeInstruction(opcode, ip);
        
        if (opcode == Opcode::RETURN || opcode == Opcode::END) {
            break;
        }
    }
    
    // Return result
    if (!stack_.empty()) {
        return stack_.back();
    }
    return WasmValue::fromI32(0);
}

void WasmInterpreter::executeInstruction(uint8_t opcode, const uint8_t*& ip) {
    switch (opcode) {
        case Opcode::UNREACHABLE:
            throw std::runtime_error("Unreachable executed");
            
        case Opcode::NOP:
            break;
            
        case Opcode::DROP:
            pop();
            break;
            
        case Opcode::SELECT: {
            WasmValue cond = pop();
            WasmValue val2 = pop();
            WasmValue val1 = pop();
            push(cond.i32 ? val1 : val2);
            break;
        }
            
        case Opcode::LOCAL_GET: {
            uint32_t idx = *ip++;  // Simplified: assume single byte
            push(locals_[idx]);
            break;
        }
            
        case Opcode::LOCAL_SET: {
            uint32_t idx = *ip++;
            locals_[idx] = pop();
            break;
        }
            
        case Opcode::LOCAL_TEE: {
            uint32_t idx = *ip++;
            locals_[idx] = stack_.back();
            break;
        }
            
        case Opcode::I32_CONST: {
            // LEB128 signed 32-bit
            int32_t result = 0;
            uint32_t shift = 0;
            uint8_t byte;
            do {
                byte = *ip++;
                result |= static_cast<int32_t>(byte & 0x7F) << shift;
                shift += 7;
            } while (byte & 0x80);
            if (shift < 32 && (byte & 0x40)) {
                result |= -(1 << shift);
            }
            push(WasmValue::fromI32(result));
            break;
        }
            
        case Opcode::I64_CONST: {
            // LEB128 signed 64-bit
            int64_t result = 0;
            uint32_t shift = 0;
            uint8_t byte;
            do {
                byte = *ip++;
                result |= static_cast<int64_t>(byte & 0x7F) << shift;
                shift += 7;
            } while (byte & 0x80);
            if (shift < 64 && (byte & 0x40)) {
                result |= -(static_cast<int64_t>(1) << shift);
            }
            push(WasmValue::fromI64(result));
            break;
        }
            
        case Opcode::F32_CONST: {
            float val;
            std::memcpy(&val, ip, sizeof(float));
            ip += sizeof(float);
            push(WasmValue::fromF32(val));
            break;
        }
            
        case Opcode::F64_CONST: {
            double val;
            std::memcpy(&val, ip, sizeof(double));
            ip += sizeof(double);
            push(WasmValue::fromF64(val));
            break;
        }
            
        // i32 arithmetic
        case Opcode::I32_ADD: {
            WasmValue b = pop();
            WasmValue a = pop();
            push(WasmValue::fromI32(a.i32 + b.i32));
            break;
        }
            
        case Opcode::I32_SUB: {
            WasmValue b = pop();
            WasmValue a = pop();
            push(WasmValue::fromI32(a.i32 - b.i32));
            break;
        }
            
        case Opcode::I32_MUL: {
            WasmValue b = pop();
            WasmValue a = pop();
            push(WasmValue::fromI32(a.i32 * b.i32));
            break;
        }
            
        case Opcode::I32_DIV_S: {
            WasmValue b = pop();
            WasmValue a = pop();
            if (b.i32 == 0) throw std::runtime_error("Division by zero");
            push(WasmValue::fromI32(a.i32 / b.i32));
            break;
        }
            
        case Opcode::I32_AND: {
            WasmValue b = pop();
            WasmValue a = pop();
            push(WasmValue::fromI32(a.i32 & b.i32));
            break;
        }
            
        case Opcode::I32_OR: {
            WasmValue b = pop();
            WasmValue a = pop();
            push(WasmValue::fromI32(a.i32 | b.i32));
            break;
        }
            
        case Opcode::I32_XOR: {
            WasmValue b = pop();
            WasmValue a = pop();
            push(WasmValue::fromI32(a.i32 ^ b.i32));
            break;
        }
            
        // Comparisons
        case Opcode::I32_EQZ:
            push(WasmValue::fromI32(pop().i32 == 0 ? 1 : 0));
            break;
            
        case Opcode::I32_EQ: {
            WasmValue b = pop();
            WasmValue a = pop();
            push(WasmValue::fromI32(a.i32 == b.i32 ? 1 : 0));
            break;
        }
            
        case Opcode::I32_NE: {
            WasmValue b = pop();
            WasmValue a = pop();
            push(WasmValue::fromI32(a.i32 != b.i32 ? 1 : 0));
            break;
        }
            
        case Opcode::I32_LT_S: {
            WasmValue b = pop();
            WasmValue a = pop();
            push(WasmValue::fromI32(a.i32 < b.i32 ? 1 : 0));
            break;
        }
            
        case Opcode::I32_GT_S: {
            WasmValue b = pop();
            WasmValue a = pop();
            push(WasmValue::fromI32(a.i32 > b.i32 ? 1 : 0));
            break;
        }
            
        case Opcode::END:
        case Opcode::RETURN:
            // Handled in execute loop
            break;
            
        default:
            // Unimplemented opcode
            break;
    }
}

// =============================================================================
// WebAssemblyObject (JS API)
// =============================================================================

WebAssemblyObject::WebAssemblyObject() = default;

WasmModule* WebAssemblyObject::Module(const std::vector<uint8_t>& bytes) {
    auto module = WasmModule::parse(bytes.data(), bytes.size());
    return module.release();
}

WasmInstance* WebAssemblyObject::Instance(WasmModule* module, const ImportObject& imports) {
    auto instance = WasmInstance::instantiate(module, imports);
    return instance.release();
}

WasmMemory* WebAssemblyObject::Memory(const MemoryType& descriptor) {
    return new WasmMemory(descriptor);
}

WasmTable* WebAssemblyObject::Table(const TableType& descriptor) {
    return new WasmTable(descriptor);
}

WasmGlobal* WebAssemblyObject::Global(const GlobalType& descriptor, WasmValue value) {
    return new WasmGlobal(descriptor, value);
}

bool WebAssemblyObject::validate(const std::vector<uint8_t>& bytes) {
    try {
        auto module = WasmModule::parse(bytes.data(), bytes.size());
        return module && module->validate();
    } catch (...) {
        return false;
    }
}

} // namespace Zepra::Wasm
