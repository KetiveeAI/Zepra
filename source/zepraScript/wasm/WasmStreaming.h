/**
 * @file WasmStreaming.h
 * @brief WebAssembly Streaming Compilation Support
 * 
 * Implements streaming compilation APIs:
 * - WebAssembly.compileStreaming()
 * - WebAssembly.instantiateStreaming()
 */

#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>

namespace Zepra::Wasm {

// Forward declarations
class WasmModule;
class WasmInstance;

// =============================================================================
// Streaming State
// =============================================================================

enum class StreamingState {
    Pending,      // Waiting for data
    Receiving,    // Actively receiving bytes
    Compiling,    // Compilation in progress
    Complete,     // Successfully completed
    Failed        // Error occurred
};

// =============================================================================
// Streaming Compiler
// =============================================================================

/**
 * @brief Progressive WASM module compilation
 * 
 * Compiles WASM bytecode as it arrives, enabling
 * faster startup for large modules.
 */
class StreamingCompiler {
public:
    using ChunkCallback = std::function<void(size_t bytesProcessed)>;
    using CompleteCallback = std::function<void(std::shared_ptr<WasmModule>)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    
    StreamingCompiler()
        : state_(StreamingState::Pending)
        , bytesReceived_(0)
        , headerParsed_(false) {}
    
    ~StreamingCompiler() {
        abort();
        if (compileThread_.joinable()) {
            compileThread_.join();
        }
    }
    
    // Add chunk of WASM bytes
    void addBytes(const uint8_t* data, size_t length) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (state_ == StreamingState::Failed) return;
        
        buffer_.insert(buffer_.end(), data, data + length);
        bytesReceived_ += length;
        state_ = StreamingState::Receiving;
        
        // Try to parse header if not done
        if (!headerParsed_ && buffer_.size() >= 8) {
            if (!parseHeader()) {
                state_ = StreamingState::Failed;
                if (errorCallback_) {
                    errorCallback_("Invalid WASM magic or version");
                }
                return;
            }
            headerParsed_ = true;
        }
        
        // Notify progress
        if (chunkCallback_) {
            chunkCallback_(bytesReceived_);
        }
        
        cv_.notify_one();
    }
    
    // Signal end of stream
    void finish() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (state_ == StreamingState::Failed) return;
        
        state_ = StreamingState::Compiling;
        cv_.notify_one();
        
        // Start compilation in background
        compileThread_ = std::thread([this]() {
            compile();
        });
    }
    
    // Abort streaming
    void abort() {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = StreamingState::Failed;
        cv_.notify_all();
    }
    
    // Callbacks
    void onChunk(ChunkCallback cb) { chunkCallback_ = std::move(cb); }
    void onComplete(CompleteCallback cb) { completeCallback_ = std::move(cb); }
    void onError(ErrorCallback cb) { errorCallback_ = std::move(cb); }
    
    StreamingState state() const { return state_; }
    size_t bytesReceived() const { return bytesReceived_; }
    
private:
    bool parseHeader() {
        if (buffer_.size() < 8) return false;
        
        // Check magic: 0x00 0x61 0x73 0x6D ("\0asm")
        if (buffer_[0] != 0x00 || buffer_[1] != 0x61 ||
            buffer_[2] != 0x73 || buffer_[3] != 0x6D) {
            return false;
        }
        
        // Check version: 0x01 0x00 0x00 0x00
        if (buffer_[4] != 0x01 || buffer_[5] != 0x00 ||
            buffer_[6] != 0x00 || buffer_[7] != 0x00) {
            return false;
        }
        
        return true;
    }
    
    void compile() {
        try {
            // Compile the complete buffer
            auto module = compileModule(buffer_.data(), buffer_.size());
            
            {
                std::lock_guard<std::mutex> lock(mutex_);
                state_ = StreamingState::Complete;
            }
            
            if (completeCallback_) {
                completeCallback_(std::move(module));
            }
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(mutex_);
            state_ = StreamingState::Failed;
            
            if (errorCallback_) {
                errorCallback_(e.what());
            }
        }
    }
    
    std::shared_ptr<WasmModule> compileModule(const uint8_t* data, size_t size);
    
    std::vector<uint8_t> buffer_;
    StreamingState state_;
    size_t bytesReceived_;
    bool headerParsed_;
    
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread compileThread_;
    
    ChunkCallback chunkCallback_;
    CompleteCallback completeCallback_;
    ErrorCallback errorCallback_;
};

// =============================================================================
// Streaming APIs
// =============================================================================

/**
 * @brief WebAssembly.compileStreaming() equivalent
 */
class CompileStreaming {
public:
    static std::shared_ptr<StreamingCompiler> create() {
        return std::make_shared<StreamingCompiler>();
    }
    
    // For Response object from fetch
    static std::shared_ptr<StreamingCompiler> fromResponse(
        std::function<void(std::function<void(const uint8_t*, size_t)>)> streamReader
    ) {
        auto compiler = create();
        
        streamReader([compiler](const uint8_t* data, size_t len) {
            if (len == 0) {
                compiler->finish();
            } else {
                compiler->addBytes(data, len);
            }
        });
        
        return compiler;
    }
};

/**
 * @brief WebAssembly.instantiateStreaming() equivalent
 */
class InstantiateStreaming {
public:
    using ResultCallback = std::function<void(
        std::shared_ptr<WasmModule>,
        std::shared_ptr<WasmInstance>
    )>;
    
    static void start(
        std::shared_ptr<StreamingCompiler> compiler,
        void* importObject,
        ResultCallback callback
    ) {
        compiler->onComplete([importObject, callback](std::shared_ptr<WasmModule> module) {
            // Instantiate the compiled module
            auto instance = instantiateModule(module, importObject);
            callback(module, instance);
        });
    }
    
private:
    static std::shared_ptr<WasmInstance> instantiateModule(
        std::shared_ptr<WasmModule> module,
        void* importObject
    );
};

// =============================================================================
// Progressive Section Parser
// =============================================================================

/**
 * @brief Parses WASM sections incrementally
 */
class SectionParser {
public:
    enum class SectionId : uint8_t {
        Custom = 0,
        Type = 1,
        Import = 2,
        Function = 3,
        Table = 4,
        Memory = 5,
        Global = 6,
        Export = 7,
        Start = 8,
        Element = 9,
        Code = 10,
        Data = 11,
        DataCount = 12
    };
    
    struct Section {
        SectionId id;
        size_t offset;
        size_t size;
        bool parsed = false;
    };
    
    // Parse available sections from buffer
    std::vector<Section> parse(const uint8_t* data, size_t size) {
        std::vector<Section> sections;
        size_t offset = 8;  // Skip header
        
        while (offset < size) {
            if (offset >= size) break;
            
            uint8_t id = data[offset++];
            
            // Read section size (LEB128)
            uint32_t sectionSize = 0;
            uint32_t shift = 0;
            while (offset < size) {
                uint8_t byte = data[offset++];
                sectionSize |= (byte & 0x7F) << shift;
                if ((byte & 0x80) == 0) break;
                shift += 7;
            }
            
            sections.push_back({
                static_cast<SectionId>(id),
                offset,
                sectionSize,
                false
            });
            
            offset += sectionSize;
        }
        
        return sections;
    }
};

} // namespace Zepra::Wasm
