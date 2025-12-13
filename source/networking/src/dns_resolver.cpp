/**
 * @file dns_resolver.cpp
 * @brief DNS resolution implementation
 */

#include "networking/dns_resolver.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unordered_map>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>

namespace Zepra::Networking {

// =============================================================================
// DNS Cache Entry
// =============================================================================

struct CacheEntry {
    DnsResult result;
    std::chrono::steady_clock::time_point expiry;
    
    bool isValid() const {
        return std::chrono::steady_clock::now() < expiry;
    }
};

// =============================================================================
// DNS Resolver Implementation
// =============================================================================

class DnsResolver::Impl {
public:
    Impl() {
        // Start worker thread
        running_ = true;
        worker_ = std::thread(&Impl::workerLoop, this);
    }
    
    ~Impl() {
        running_ = false;
        cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
    }
    
    DnsResult resolve(const std::string& hostname) {
        // Check cache first
        {
            std::lock_guard<std::mutex> lock(cacheMutex_);
            auto it = cache_.find(hostname);
            if (it != cache_.end() && it->second.isValid()) {
                return it->second.result;
            }
        }
        
        // Perform resolution
        DnsResult result = doResolve(hostname);
        
        // Cache result
        if (result.success) {
            std::lock_guard<std::mutex> lock(cacheMutex_);
            CacheEntry entry;
            entry.result = result;
            entry.expiry = std::chrono::steady_clock::now() + result.ttl;
            cache_[hostname] = entry;
        }
        
        return result;
    }
    
    std::future<DnsResult> resolveAsync(const std::string& hostname) {
        return std::async(std::launch::async, [this, hostname]() {
            return resolve(hostname);
        });
    }
    
    void resolve(const std::string& hostname, 
                 std::function<void(const DnsResult&)> callback) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        asyncQueue_.push({hostname, std::move(callback)});
        cv_.notify_one();
    }
    
    void prefetch(const std::string& hostname) {
        // Resolve in background, ignore result
        std::thread([this, hostname]() {
            resolve(hostname);
        }).detach();
    }
    
    void clearCache() {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        cache_.clear();
    }
    
    void setDnsServers(const std::vector<std::string>& servers) {
        std::lock_guard<std::mutex> lock(configMutex_);
        customDnsServers_ = servers;
    }
    
    void setDoHEnabled(bool enabled, const std::string& server) {
        std::lock_guard<std::mutex> lock(configMutex_);
        dohEnabled_ = enabled;
        dohServer_ = server;
    }
    
    DnsResult getCached(const std::string& hostname) {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto it = cache_.find(hostname);
        if (it != cache_.end() && it->second.isValid()) {
            return it->second.result;
        }
        return DnsResult{false, hostname, {}, "", {}, "Not in cache"};
    }
    
private:
    DnsResult doResolve(const std::string& hostname) {
        DnsResult result;
        result.hostname = hostname;
        
        // Use getaddrinfo for resolution
        struct addrinfo hints = {};
        hints.ai_family = AF_UNSPEC;  // IPv4 or IPv6
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_CANONNAME;
        
        struct addrinfo* res = nullptr;
        int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);
        
        if (status != 0) {
            result.success = false;
            result.error = gai_strerror(status);
            return result;
        }
        
        result.success = true;
        
        // Get canonical name
        if (res->ai_canonname) {
            result.canonicalName = res->ai_canonname;
        }
        
        // Extract addresses
        for (struct addrinfo* p = res; p != nullptr; p = p->ai_next) {
            char addrStr[INET6_ADDRSTRLEN];
            void* addr = nullptr;
            
            if (p->ai_family == AF_INET) {
                struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
                addr = &(ipv4->sin_addr);
            } else if (p->ai_family == AF_INET6) {
                struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
                addr = &(ipv6->sin6_addr);
            } else {
                continue;
            }
            
            inet_ntop(p->ai_family, addr, addrStr, sizeof(addrStr));
            result.addresses.push_back(addrStr);
        }
        
        freeaddrinfo(res);
        
        // Set default TTL
        result.ttl = std::chrono::milliseconds(300000);  // 5 minutes
        
        return result;
    }
    
    void workerLoop() {
        while (running_) {
            std::unique_lock<std::mutex> lock(queueMutex_);
            cv_.wait(lock, [this]() {
                return !asyncQueue_.empty() || !running_;
            });
            
            if (!running_) break;
            
            if (!asyncQueue_.empty()) {
                auto task = std::move(asyncQueue_.front());
                asyncQueue_.pop();
                lock.unlock();
                
                DnsResult result = resolve(task.hostname);
                if (task.callback) {
                    task.callback(result);
                }
            }
        }
    }
    
    struct AsyncTask {
        std::string hostname;
        std::function<void(const DnsResult&)> callback;
    };
    
    std::mutex cacheMutex_;
    std::unordered_map<std::string, CacheEntry> cache_;
    
    std::mutex queueMutex_;
    std::condition_variable cv_;
    std::queue<AsyncTask> asyncQueue_;
    
    std::mutex configMutex_;
    std::vector<std::string> customDnsServers_;
    bool dohEnabled_ = false;
    std::string dohServer_;
    
    std::atomic<bool> running_{false};
    std::thread worker_;
};

// =============================================================================
// DnsResolver Implementation
// =============================================================================

DnsResolver::DnsResolver() : impl_(std::make_unique<Impl>()) {}

DnsResolver::~DnsResolver() = default;

DnsResult DnsResolver::resolve(const std::string& hostname) {
    return impl_->resolve(hostname);
}

std::future<DnsResult> DnsResolver::resolveAsync(const std::string& hostname) {
    return impl_->resolveAsync(hostname);
}

void DnsResolver::resolve(const std::string& hostname,
                           std::function<void(const DnsResult&)> callback) {
    impl_->resolve(hostname, std::move(callback));
}

void DnsResolver::prefetch(const std::string& hostname) {
    impl_->prefetch(hostname);
}

void DnsResolver::clearCache() {
    impl_->clearCache();
}

void DnsResolver::setDnsServers(const std::vector<std::string>& servers) {
    impl_->setDnsServers(servers);
}

void DnsResolver::setDoHEnabled(bool enabled, const std::string& server) {
    impl_->setDoHEnabled(enabled, server);
}

DnsResult DnsResolver::getCached(const std::string& hostname) {
    return impl_->getCached(hostname);
}

// =============================================================================
// Global Instance
// =============================================================================

DnsResolver& getDnsResolver() {
    static DnsResolver instance;
    return instance;
}

} // namespace Zepra::Networking
