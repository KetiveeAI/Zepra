/**
 * @file WorkerAPI.h
 * @brief Web Workers API Implementation
 * 
 * HTML Standard Workers:
 * - Worker: Dedicated worker
 * - MessagePort, MessageChannel
 * - postMessage, onmessage
 */

#pragma once

#include <string>
#include <functional>
#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <variant>
#include <vector>

namespace Zepra::API {

// Forward declarations
class MessagePort;
class Worker;

// =============================================================================
// Message Data (Structured Clone)
// =============================================================================

using TransferableObject = std::variant<
    std::shared_ptr<std::vector<uint8_t>>,  // ArrayBuffer
    std::shared_ptr<MessagePort>             // MessagePort
>;

struct MessageData {
    std::string serialized;                      // JSON or structured clone
    std::vector<TransferableObject> transferList;
};

// =============================================================================
// MessagePort
// =============================================================================

/**
 * @brief One end of a message channel
 */
class MessagePort : public std::enable_shared_from_this<MessagePort> {
public:
    MessagePort() = default;
    
    // Post message to other end
    void postMessage(const std::string& data) {
        postMessage(MessageData{data, {}});
    }
    
    void postMessage(MessageData data) {
        if (!entangledPort_) return;
        entangledPort_->enqueueMessage(std::move(data));
    }
    
    // Message handler
    using MessageHandler = std::function<void(const MessageData&)>;
    void setOnMessage(MessageHandler handler) {
        onMessage_ = std::move(handler);
    }
    
    // Error handler
    using ErrorHandler = std::function<void(const std::string&)>;
    void setOnMessageError(ErrorHandler handler) {
        onError_ = std::move(handler);
    }
    
    // Start receiving messages
    void start() {
        started_ = true;
        processMessages();
    }
    
    // Close port
    void close() {
        closed_ = true;
        if (entangledPort_) {
            entangledPort_->entangledPort_ = nullptr;
            entangledPort_ = nullptr;
        }
    }
    
    // Entangle with another port
    void entangle(std::shared_ptr<MessagePort> other) {
        entangledPort_ = other;
        other->entangledPort_ = shared_from_this();
    }
    
private:
    void enqueueMessage(MessageData data) {
        std::lock_guard lock(mutex_);
        messageQueue_.push(std::move(data));
        if (started_) {
            processMessages();
        }
    }
    
    void processMessages() {
        while (true) {
            MessageData msg;
            {
                std::lock_guard lock(mutex_);
                if (messageQueue_.empty()) break;
                msg = std::move(messageQueue_.front());
                messageQueue_.pop();
            }
            
            if (onMessage_) {
                onMessage_(msg);
            }
        }
    }
    
    std::shared_ptr<MessagePort> entangledPort_;
    MessageHandler onMessage_;
    ErrorHandler onError_;
    
    std::mutex mutex_;
    std::queue<MessageData> messageQueue_;
    bool started_ = false;
    bool closed_ = false;
};

// =============================================================================
// MessageChannel
// =============================================================================

/**
 * @brief Creates a pair of entangled message ports
 */
class MessageChannel {
public:
    MessageChannel() {
        port1_ = std::make_shared<MessagePort>();
        port2_ = std::make_shared<MessagePort>();
        port1_->entangle(port2_);
    }
    
    std::shared_ptr<MessagePort> port1() const { return port1_; }
    std::shared_ptr<MessagePort> port2() const { return port2_; }
    
private:
    std::shared_ptr<MessagePort> port1_;
    std::shared_ptr<MessagePort> port2_;
};

// =============================================================================
// Worker Options
// =============================================================================

struct WorkerOptions {
    std::string type = "classic";  // "classic" or "module"
    std::string credentials = "same-origin";
    std::string name;
};

// =============================================================================
// Worker
// =============================================================================

/**
 * @brief Dedicated Web Worker
 */
class Worker {
public:
    explicit Worker(const std::string& scriptURL, 
                    const WorkerOptions& options = {})
        : scriptURL_(scriptURL), options_(options) {
        
        // Create internal message channel
        auto channel = MessageChannel();
        outsidePort_ = channel.port1();
        insidePort_ = channel.port2();
        
        // Start worker thread
        startWorker();
    }
    
    ~Worker() {
        terminate();
    }
    
    // Post message to worker
    void postMessage(const std::string& data) {
        outsidePort_->postMessage(data);
    }
    
    void postMessage(MessageData data) {
        outsidePort_->postMessage(std::move(data));
    }
    
    // Message handler
    using MessageHandler = std::function<void(const MessageData&)>;
    void setOnMessage(MessageHandler handler) {
        outsidePort_->setOnMessage(std::move(handler));
        outsidePort_->start();
    }
    
    // Error handler
    using ErrorHandler = std::function<void(const std::string&)>;
    void setOnError(ErrorHandler handler) {
        onError_ = std::move(handler);
    }
    
    // Terminate worker
    void terminate() {
        if (terminated_.exchange(true)) return;
        
        // Signal worker to stop
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
    }
    
private:
    void startWorker() {
        workerThread_ = std::thread([this]() {
            runWorker();
        });
    }
    
    void runWorker() {
        // Would:
        // 1. Create new JS context
        // 2. Load and execute script
        // 3. Set up message handling
        // 4. Run event loop
        
        insidePort_->start();
        
        while (!terminated_) {
            // Would process worker tasks
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    std::string scriptURL_;
    WorkerOptions options_;
    
    std::shared_ptr<MessagePort> outsidePort_;
    std::shared_ptr<MessagePort> insidePort_;
    
    ErrorHandler onError_;
    
    std::thread workerThread_;
    std::atomic<bool> terminated_{false};
};

// =============================================================================
// SharedWorker (Stub)
// =============================================================================

/**
 * @brief Shared Web Worker (multiple contexts)
 */
class SharedWorker {
public:
    explicit SharedWorker(const std::string& scriptURL,
                          const std::string& name = "")
        : scriptURL_(scriptURL), name_(name) {
        port_ = std::make_shared<MessagePort>();
    }
    
    std::shared_ptr<MessagePort> port() const { return port_; }
    
private:
    std::string scriptURL_;
    std::string name_;
    std::shared_ptr<MessagePort> port_;
};

// =============================================================================
// Global Worker Scope Functions
// =============================================================================

// Called from inside worker
void postMessage(const std::string& data);
void close();

} // namespace Zepra::API
