#pragma once

/**
 * @file worker.hpp
 * @brief JavaScript Web Workers
 */

#include "../config.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/objects/value.hpp"
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace Zepra::Runtime { class Context; class VM; }

namespace Zepra::Browser {

using Runtime::Value;
using Runtime::Object;

/**
 * @brief Message for worker communication
 */
struct WorkerMessage {
    Value data;
    std::vector<Object*> transferList;
};

/**
 * @brief Web Worker implementation
 */
class Worker : public Object {
public:
    explicit Worker(const std::string& scriptUrl);
    ~Worker();
    
    /**
     * @brief Post message to worker
     */
    void postMessage(Value data);
    
    /**
     * @brief Terminate worker
     */
    void terminate();
    
    /**
     * @brief Check if worker is running
     */
    bool isRunning() const { return running_; }
    
    /**
     * @brief Set message handler
     */
    void setOnMessage(std::function<void(Value)> handler);
    
    /**
     * @brief Set error handler
     */
    void setOnError(std::function<void(const std::string&)> handler);
    
private:
    void workerThread();
    void processMessages();
    
    std::string scriptUrl_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> shouldTerminate_{false};
    
    std::queue<WorkerMessage> incomingMessages_;
    std::queue<WorkerMessage> outgoingMessages_;
    std::mutex mutex_;
    std::condition_variable cv_;
    
    std::function<void(Value)> onMessage_;
    std::function<void(const std::string&)> onError_;
    
    Runtime::VM* workerVM_ = nullptr;
};

/**
 * @brief SharedWorker implementation
 */
class SharedWorker : public Object {
public:
    explicit SharedWorker(const std::string& scriptUrl, const std::string& name = "");
    
    /**
     * @brief Get message port for communication
     */
    Object* port() const { return port_; }
    
private:
    std::string scriptUrl_;
    std::string name_;
    Object* port_ = nullptr;
};

/**
 * @brief Worker builtin functions
 */
class WorkerBuiltin {
public:
    static Value constructor(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value postMessage(Runtime::Context* ctx, const std::vector<Value>& args);
    static Value terminate(Runtime::Context* ctx, const std::vector<Value>& args);
};

/**
 * @brief Global worker scope (for inside workers)
 */
class WorkerGlobalScope : public Object {
public:
    WorkerGlobalScope();
    
    /**
     * @brief Post message to main thread
     */
    void postMessage(Value data);
    
    /**
     * @brief Close the worker
     */
    void close();
    
    /**
     * @brief Import scripts
     */
    void importScripts(const std::vector<std::string>& urls);
    
    /**
     * @brief Set handler for postMessage calls (used internally by Worker)
     */
    void setPostMessageHandler(std::function<void(Value)> handler);
    
private:
    std::function<void(Value)> postMessageHandler_;
};

} // namespace Zepra::Browser
