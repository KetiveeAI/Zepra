/**
 * @file html_workers.hpp
 * @brief Web Workers API
 */

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <any>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief Message event
 */
struct MessageEvent {
    std::any data;
    std::string origin;
    std::string lastEventId;
    std::vector<class MessagePort*> ports;
};

/**
 * @brief Error event
 */
struct ErrorEvent {
    std::string message;
    std::string filename;
    int lineno = 0;
    int colno = 0;
    std::any error;
};

/**
 * @brief Message port for communication
 */
class MessagePort {
public:
    MessagePort() = default;
    virtual ~MessagePort() = default;
    
    virtual void postMessage(const std::any& message) = 0;
    virtual void start() = 0;
    virtual void close() = 0;
    
    std::function<void(const MessageEvent&)> onMessage;
    std::function<void(const MessageEvent&)> onMessageError;
};

/**
 * @brief Channel for two-way communication
 */
class MessageChannel {
public:
    MessageChannel();
    ~MessageChannel();
    
    MessagePort* port1() { return port1_.get(); }
    MessagePort* port2() { return port2_.get(); }
    
private:
    std::unique_ptr<MessagePort> port1_;
    std::unique_ptr<MessagePort> port2_;
};

/**
 * @brief Worker options
 */
struct WorkerOptions {
    std::string type = "classic";  // classic, module
    std::string credentials = "same-origin";
    std::string name;
};

/**
 * @brief Abstract worker base
 */
class AbstractWorker {
public:
    virtual ~AbstractWorker() = default;
    
    std::function<void(const ErrorEvent&)> onError;
};

/**
 * @brief Dedicated worker
 */
class Worker : public AbstractWorker {
public:
    Worker(const std::string& scriptURL, const WorkerOptions& options = {});
    ~Worker() override;
    
    void postMessage(const std::any& message);
    void terminate();
    
    std::function<void(const MessageEvent&)> onMessage;
    std::function<void(const MessageEvent&)> onMessageError;
    
private:
    std::string scriptURL_;
    WorkerOptions options_;
};

/**
 * @brief Shared worker
 */
class SharedWorker : public AbstractWorker {
public:
    SharedWorker(const std::string& scriptURL, const std::string& name = "");
    ~SharedWorker() override;
    
    MessagePort* port() { return port_.get(); }
    
private:
    std::string scriptURL_;
    std::string name_;
    std::unique_ptr<MessagePort> port_;
};

/**
 * @brief Service worker state
 */
enum class ServiceWorkerState {
    Parsed,
    Installing,
    Installed,
    Activating,
    Activated,
    Redundant
};

/**
 * @brief Service worker
 */
class ServiceWorker : public AbstractWorker {
public:
    ServiceWorker(const std::string& scriptURL);
    ~ServiceWorker() override;
    
    std::string scriptURL() const { return scriptURL_; }
    ServiceWorkerState state() const { return state_; }
    
    void postMessage(const std::any& message);
    
    std::function<void()> onStateChange;
    
private:
    std::string scriptURL_;
    ServiceWorkerState state_ = ServiceWorkerState::Parsed;
};

/**
 * @brief Service worker registration
 */
class ServiceWorkerRegistration {
public:
    ServiceWorkerRegistration(const std::string& scope);
    ~ServiceWorkerRegistration();
    
    std::string scope() const { return scope_; }
    
    ServiceWorker* installing() { return installing_.get(); }
    ServiceWorker* waiting() { return waiting_.get(); }
    ServiceWorker* active() { return active_.get(); }
    
    void update();
    void unregister(std::function<void(bool)> callback);
    
    std::function<void()> onUpdateFound;
    
private:
    std::string scope_;
    std::unique_ptr<ServiceWorker> installing_;
    std::unique_ptr<ServiceWorker> waiting_;
    std::unique_ptr<ServiceWorker> active_;
};

/**
 * @brief Service worker container
 */
class ServiceWorkerContainer {
public:
    ServiceWorkerContainer() = default;
    ~ServiceWorkerContainer() = default;
    
    ServiceWorker* controller() { return controller_.get(); }
    
    void getRegistration(const std::string& scope,
                         std::function<void(ServiceWorkerRegistration*)> callback);
    void getRegistrations(std::function<void(std::vector<ServiceWorkerRegistration*>)> callback);
    
    void registerServiceWorker(const std::string& scriptURL, const std::string& scope,
                               std::function<void(ServiceWorkerRegistration*)> callback);
    
    std::function<void()> onControllerChange;
    std::function<void(const MessageEvent&)> onMessage;
    
private:
    std::unique_ptr<ServiceWorker> controller_;
    std::vector<std::unique_ptr<ServiceWorkerRegistration>> registrations_;
};

/**
 * @brief Broadcast channel
 */
class BroadcastChannel {
public:
    BroadcastChannel(const std::string& name);
    ~BroadcastChannel();
    
    std::string name() const { return name_; }
    
    void postMessage(const std::any& message);
    void close();
    
    std::function<void(const MessageEvent&)> onMessage;
    std::function<void(const MessageEvent&)> onMessageError;
    
private:
    std::string name_;
    bool closed_ = false;
};

} // namespace Zepra::WebCore
