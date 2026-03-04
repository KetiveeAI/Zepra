/**
 * @file MessageChannelAPI.h
 * @brief MessageChannel and MessagePort Implementation
 */

#pragma once

#include <functional>
#include <memory>
#include <queue>
#include <mutex>
#include <any>
#include <string>
#include <vector>

namespace Zepra::Runtime {

class MessagePort;

// =============================================================================
// Message Event
// =============================================================================

struct MessageEvent {
    std::any data;
    std::string origin;
    std::string lastEventId;
    std::shared_ptr<MessagePort> source;
    std::vector<std::shared_ptr<MessagePort>> ports;
};

// =============================================================================
// MessagePort
// =============================================================================

class MessagePort : public std::enable_shared_from_this<MessagePort> {
public:
    using MessageHandler = std::function<void(const MessageEvent&)>;
    using ErrorHandler = std::function<void(const std::string&)>;
    
    MessagePort() : started_(false), closed_(false) {}
    
    void postMessage(std::any data) {
        if (closed_) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        MessageEvent event;
        event.data = std::move(data);
        event.origin = "";
        event.source = shared_from_this();
        
        if (partner_ && !partner_->closed_) {
            partner_->enqueue(event);
        }
    }
    
    void postMessage(std::any data, const std::vector<std::shared_ptr<MessagePort>>& transfer) {
        if (closed_) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        MessageEvent event;
        event.data = std::move(data);
        event.origin = "";
        event.source = shared_from_this();
        event.ports = transfer;
        
        if (partner_ && !partner_->closed_) {
            partner_->enqueue(event);
        }
    }
    
    void start() {
        started_ = true;
        processQueue();
    }
    
    void close() {
        closed_ = true;
        if (partner_) {
            partner_->partner_ = nullptr;
        }
        partner_ = nullptr;
    }
    
    void setOnMessage(MessageHandler handler) { onMessage_ = std::move(handler); }
    void setOnMessageError(ErrorHandler handler) { onMessageError_ = std::move(handler); }
    
    bool isStarted() const { return started_; }
    bool isClosed() const { return closed_; }
    
    void setPartner(std::shared_ptr<MessagePort> partner) {
        partner_ = partner;
    }

private:
    void enqueue(const MessageEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(event);
        if (started_) {
            processQueue();
        }
    }
    
    void processQueue() {
        while (!queue_.empty() && onMessage_) {
            MessageEvent event = std::move(queue_.front());
            queue_.pop();
            try {
                onMessage_(event);
            } catch (const std::exception& e) {
                if (onMessageError_) {
                    onMessageError_(e.what());
                }
            }
        }
    }
    
    std::shared_ptr<MessagePort> partner_;
    MessageHandler onMessage_;
    ErrorHandler onMessageError_;
    std::queue<MessageEvent> queue_;
    std::mutex mutex_;
    bool started_;
    bool closed_;
};

// =============================================================================
// MessageChannel
// =============================================================================

class MessageChannel {
public:
    MessageChannel() {
        port1_ = std::make_shared<MessagePort>();
        port2_ = std::make_shared<MessagePort>();
        port1_->setPartner(port2_);
        port2_->setPartner(port1_);
    }
    
    std::shared_ptr<MessagePort> port1() { return port1_; }
    std::shared_ptr<MessagePort> port2() { return port2_; }

private:
    std::shared_ptr<MessagePort> port1_;
    std::shared_ptr<MessagePort> port2_;
};

// =============================================================================
// BroadcastChannel
// =============================================================================

class BroadcastChannel {
public:
    using MessageHandler = std::function<void(const MessageEvent&)>;
    
    explicit BroadcastChannel(const std::string& name) : name_(name), closed_(false) {
        registerChannel(this);
    }
    
    ~BroadcastChannel() {
        unregisterChannel(this);
    }
    
    void postMessage(std::any data) {
        if (closed_) return;
        
        MessageEvent event;
        event.data = std::move(data);
        
        broadcastToAll(name_, event, this);
    }
    
    void close() { closed_ = true; }
    
    void setOnMessage(MessageHandler handler) { onMessage_ = std::move(handler); }
    
    const std::string& name() const { return name_; }

private:
    static void registerChannel(BroadcastChannel* channel);
    static void unregisterChannel(BroadcastChannel* channel);
    static void broadcastToAll(const std::string& name, const MessageEvent& event, BroadcastChannel* sender);
    
    void receive(const MessageEvent& event) {
        if (onMessage_ && !closed_) {
            onMessage_(event);
        }
    }
    
    std::string name_;
    MessageHandler onMessage_;
    bool closed_;
};

} // namespace Zepra::Runtime
