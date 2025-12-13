/**
 * @file ssl_context.hpp
 * @brief SSL/TLS context management for secure connections
 * 
 * Provides certificate verification, TLS handshake, and secure socket operations.
 * Uses OpenSSL for cryptographic operations.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

namespace Zepra::Networking {

/**
 * @brief SSL/TLS protocol version
 */
enum class TLSVersion {
    TLS_1_0 = 0x0301,
    TLS_1_1 = 0x0302,
    TLS_1_2 = 0x0303,
    TLS_1_3 = 0x0304
};

/**
 * @brief Certificate verification result
 */
enum class CertVerifyResult {
    OK = 0,
    EXPIRED,
    NOT_YET_VALID,
    REVOKED,
    UNTRUSTED_ROOT,
    HOSTNAME_MISMATCH,
    SELF_SIGNED,
    CHAIN_TOO_LONG,
    INVALID_CA,
    INVALID_PURPOSE,
    UNKNOWN_ERROR
};

/**
 * @brief Certificate information
 */
struct CertificateInfo {
    std::string subject;
    std::string issuer;
    std::string serialNumber;
    std::string fingerprint;
    std::string notBefore;
    std::string notAfter;
    std::vector<std::string> subjectAltNames;
    bool isCA = false;
    int keyBits = 0;
    std::string signatureAlgorithm;
};

/**
 * @brief SSL connection state
 */
struct SSLConnectionInfo {
    TLSVersion version = TLSVersion::TLS_1_3;
    std::string cipherSuite;
    int cipherStrength = 0;
    bool sessionReused = false;
    std::vector<CertificateInfo> certificateChain;
};

/**
 * @brief SSL Context for managing TLS connections
 */
class SSLContext {
public:
    SSLContext();
    ~SSLContext();
    
    // Prevent copying
    SSLContext(const SSLContext&) = delete;
    SSLContext& operator=(const SSLContext&) = delete;
    
    /**
     * @brief Initialize SSL context
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Shutdown SSL context
     */
    void shutdown();
    
    /**
     * @brief Set minimum TLS version
     */
    void setMinVersion(TLSVersion version);
    
    /**
     * @brief Set maximum TLS version
     */
    void setMaxVersion(TLSVersion version);
    
    /**
     * @brief Load system CA certificates
     * @return true if successful
     */
    bool loadSystemCertificates();
    
    /**
     * @brief Load CA certificate from file
     */
    bool loadCertificateFile(const std::string& path);
    
    /**
     * @brief Load CA certificate from memory
     */
    bool loadCertificateData(const std::vector<uint8_t>& data);
    
    /**
     * @brief Set cipher suites
     */
    void setCipherSuites(const std::string& ciphers);
    
    /**
     * @brief Enable/disable hostname verification
     */
    void setVerifyHostname(bool verify);
    
    /**
     * @brief Enable/disable certificate verification
     */
    void setVerifyCertificate(bool verify);
    
    /**
     * @brief Set verification callback
     */
    using VerifyCallback = std::function<bool(CertVerifyResult, const CertificateInfo&)>;
    void setVerifyCallback(VerifyCallback callback);
    
    /**
     * @brief Verify certificate chain
     */
    CertVerifyResult verifyCertificate(const std::string& hostname,
                                        const std::vector<uint8_t>& certData);
    
    /**
     * @brief Get certificate info from data
     */
    CertificateInfo getCertificateInfo(const std::vector<uint8_t>& certData);
    
    /**
     * @brief Create SSL socket wrapper
     */
    class SSLSocket {
    public:
        SSLSocket(SSLContext* ctx, int fd);
        ~SSLSocket();
        
        bool connect(const std::string& hostname);
        bool accept();
        
        int read(void* buffer, size_t size);
        int write(const void* buffer, size_t size);
        
        void close();
        
        SSLConnectionInfo getConnectionInfo() const;
        CertVerifyResult getVerifyResult() const;
        
    private:
        SSLContext* ctx_;
        int fd_;
        void* ssl_ = nullptr;
        CertVerifyResult verifyResult_ = CertVerifyResult::OK;
    };
    
    std::unique_ptr<SSLSocket> createSocket(int fd);
    
    /**
     * @brief Get OpenSSL error string
     */
    static std::string getLastError();
    
private:
    void* ctx_ = nullptr;  // SSL_CTX*
    TLSVersion minVersion_ = TLSVersion::TLS_1_2;
    TLSVersion maxVersion_ = TLSVersion::TLS_1_3;
    bool verifyHostname_ = true;
    bool verifyCertificate_ = true;
    VerifyCallback verifyCallback_;
};

/**
 * @brief Global SSL context singleton
 */
SSLContext& getSSLContext();

} // namespace Zepra::Networking
