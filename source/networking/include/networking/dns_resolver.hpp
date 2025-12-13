/**
 * @file dns_resolver.hpp
 * @brief DNS resolution for hostname lookups
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <future>
#include <chrono>

namespace Zepra::Networking {

/**
 * @brief DNS record types
 */
enum class DnsRecordType {
    A,      // IPv4
    AAAA,   // IPv6
    CNAME,
    MX,
    TXT
};

/**
 * @brief DNS resolution result
 */
struct DnsResult {
    bool success = false;
    std::string hostname;
    std::vector<std::string> addresses;
    std::string canonicalName;
    std::chrono::milliseconds ttl{0};
    std::string error;
};

/**
 * @brief DNS Resolver
 */
class DnsResolver {
public:
    DnsResolver();
    ~DnsResolver();
    
    /**
     * @brief Resolve hostname synchronously
     */
    DnsResult resolve(const std::string& hostname);
    
    /**
     * @brief Resolve hostname asynchronously
     */
    std::future<DnsResult> resolveAsync(const std::string& hostname);
    
    /**
     * @brief Resolve with callback
     */
    void resolve(const std::string& hostname, 
                 std::function<void(const DnsResult&)> callback);
    
    /**
     * @brief Prefetch DNS for URL
     */
    void prefetch(const std::string& hostname);
    
    /**
     * @brief Clear DNS cache
     */
    void clearCache();
    
    /**
     * @brief Set custom DNS servers
     */
    void setDnsServers(const std::vector<std::string>& servers);
    
    /**
     * @brief Enable/disable DoH (DNS over HTTPS)
     */
    void setDoHEnabled(bool enabled, const std::string& server = "");
    
    /**
     * @brief Get cached result if available
     */
    DnsResult getCached(const std::string& hostname);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Global DNS resolver
 */
DnsResolver& getDnsResolver();

} // namespace Zepra::Networking
