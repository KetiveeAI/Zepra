/**
 * @file worker.cpp
 * @brief JavaScript Web Workers implementation
 */

#include "browser/worker.hpp"
#include "runtime/execution/vm.hpp"
#include "runtime/objects/function.hpp"

namespace Zepra::Browser {

// =============================================================================
// Worker Implementation
// =============================================================================

Worker::Worker(const std::string& scriptUrl)
    : Object(Runtime::ObjectType::Ordinary)
    , scriptUrl_(scriptUrl) {
    
    running_ = true;
    thread_ = std::thread(&Worker::workerThread, this);
}

Worker::~Worker() {
    terminate();
}

void Worker::postMessage(Value data) {
    std::lock_guard<std::mutex> lock(mutex_);
    WorkerMessage msg;
    msg.data = data;
    incomingMessages_.push(msg);
    cv_.notify_one();
}

void Worker::terminate() {
    shouldTerminate_ = true;
    cv_.notify_all();
    
    if (thread_.joinable()) {
        thread_.join();
    }
    
    running_ = false;
}

void Worker::setOnMessage(std::function<void(Value)> handler) {
    onMessage_ = std::move(handler);
}

void Worker::setOnError(std::function<void(const std::string&)> handler) {
    onError_ = std::move(handler);
}

void Worker::workerThread() {
    // Create worker VM for script execution
    workerVM_ = new Runtime::VM(nullptr);
    
    // Create worker global scope
    WorkerGlobalScope* globalScope = new WorkerGlobalScope();
    
    // Set up postMessage handler to send messages back to main thread
    globalScope->setPostMessageHandler([this](Value data) {
        std::lock_guard<std::mutex> lock(mutex_);
        WorkerMessage msg;
        msg.data = data;
        outgoingMessages_.push(msg);
    });
    
    // TODO: Load and compile script from scriptUrl_
    // For now, worker is ready to receive messages
    
    while (!shouldTerminate_) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // Wait for messages or termination
        cv_.wait(lock, [this] {
            return !incomingMessages_.empty() || shouldTerminate_;
        });
        
        if (shouldTerminate_) break;
        
        // Process incoming messages
        while (!incomingMessages_.empty()) {
            WorkerMessage msg = std::move(incomingMessages_.front());
            incomingMessages_.pop();
            
            lock.unlock();
            
            // Dispatch 'message' event to worker script via VM
            // Create MessageEvent object with data property
            Runtime::Object* messageEvent = new Runtime::Object();
            messageEvent->set("type", Value::string(new Runtime::String("message")));
            messageEvent->set("data", msg.data);
            
            // Execute onmessage handler if registered in worker script
            // For now, echo back the message to demonstrate functionality
            {
                std::lock_guard<std::mutex> outLock(mutex_);
                outgoingMessages_.push(msg);
            }
            
            lock.lock();
        }
    }
    
    // Cleanup
    delete workerVM_;
    workerVM_ = nullptr;
}

void Worker::processMessages() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    while (!outgoingMessages_.empty()) {
        WorkerMessage msg = std::move(outgoingMessages_.front());
        outgoingMessages_.pop();
        
        if (onMessage_) {
            onMessage_(msg.data);
        }
    }
}

// =============================================================================
// SharedWorker Implementation
// =============================================================================

SharedWorker::SharedWorker(const std::string& scriptUrl, const std::string& name)
    : Object(Runtime::ObjectType::Ordinary)
    , scriptUrl_(scriptUrl)
    , name_(name) {
    
    port_ = new Object();
}

// =============================================================================
// WorkerBuiltin Implementation
// =============================================================================

Value WorkerBuiltin::constructor(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isString()) {
        return Value::undefined();
    }
    
    std::string url = static_cast<Runtime::String*>(args[0].asObject())->value();
    return Value::object(new Worker(url));
}

Value WorkerBuiltin::postMessage(Runtime::Context*, const std::vector<Value>& args) {
    if (args.size() < 2 || !args[0].isObject()) {
        return Value::undefined();
    }
    
    Worker* worker = dynamic_cast<Worker*>(args[0].asObject());
    if (worker) {
        worker->postMessage(args[1]);
    }
    
    return Value::undefined();
}

Value WorkerBuiltin::terminate(Runtime::Context*, const std::vector<Value>& args) {
    if (args.empty() || !args[0].isObject()) {
        return Value::undefined();
    }
    
    Worker* worker = dynamic_cast<Worker*>(args[0].asObject());
    if (worker) {
        worker->terminate();
    }
    
    return Value::undefined();
}

// =============================================================================
// WorkerGlobalScope Implementation
// =============================================================================

WorkerGlobalScope::WorkerGlobalScope()
    : Object(Runtime::ObjectType::Global) {}

void WorkerGlobalScope::postMessage(Value data) {
    if (postMessageHandler_) {
        postMessageHandler_(data);
    }
}

void WorkerGlobalScope::close() {
    // Signal worker thread to terminate
}

void WorkerGlobalScope::importScripts(const std::vector<std::string>& urls) {
    // Load and execute scripts synchronously
    for (const auto& url : urls) {
        // Fetch script content from URL
        // For file:// URLs or bundled scripts, load from filesystem
        // For http:// URLs, fetch via network
        
        // Parse and compile the script
        // Execute in the worker's VM context
        
        // Log the import attempt for debugging
        std::printf("[Worker] importScripts: %s\n", url.c_str());
    }
}

void WorkerGlobalScope::setPostMessageHandler(std::function<void(Value)> handler) {
    postMessageHandler_ = std::move(handler);
}

} // namespace Zepra::Browser
