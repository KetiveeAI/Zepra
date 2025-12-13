/**
 * @file ssl_context.cpp
 * @brief SSL/TLS context implementation using OpenSSL
 */

#include "networking/ssl_context.hpp"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>

#include <cstring>
#include <sstream>
#include <iomanip>

namespace Zepra::Networking {

// =============================================================================
// SSLContext Implementation
// =============================================================================

SSLContext::SSLContext() {}

SSLContext::~SSLContext() {
    shutdown();
}

bool SSLContext::initialize() {
    // Initialize OpenSSL (thread-safe on modern OpenSSL)
    OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr);
    
    // Create context with TLS 1.2+ only
    const SSL_METHOD* method = TLS_client_method();
    ctx_ = SSL_CTX_new(method);
    
    if (!ctx_) {
        return false;
    }
    
    SSL_CTX* sslCtx = static_cast<SSL_CTX*>(ctx_);
    
    // ===== Security Options =====
    // Disable older insecure protocols
    SSL_CTX_set_options(sslCtx, 
        SSL_OP_NO_SSLv2 | 
        SSL_OP_NO_SSLv3 | 
        SSL_OP_NO_TLSv1 |
        SSL_OP_NO_TLSv1_1 |
        SSL_OP_NO_COMPRESSION |           // Disable CRIME attack vector
        SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION |
        SSL_OP_CIPHER_SERVER_PREFERENCE   // Let server choose cipher
    );
    
    // Set TLS version range
    SSL_CTX_set_min_proto_version(sslCtx, TLS1_2_VERSION);
    SSL_CTX_set_max_proto_version(sslCtx, TLS1_3_VERSION);
    
    // ===== Modern Cipher Suites =====
    // TLS 1.3 ciphers (automatic in OpenSSL 1.1.1+)
    SSL_CTX_set_ciphersuites(sslCtx, 
        "TLS_AES_256_GCM_SHA384:"
        "TLS_CHACHA20_POLY1305_SHA256:"
        "TLS_AES_128_GCM_SHA256");
    
    // TLS 1.2 ciphers (AEAD only, no CBC)
    SSL_CTX_set_cipher_list(sslCtx,
        "ECDHE-ECDSA-AES256-GCM-SHA384:"
        "ECDHE-RSA-AES256-GCM-SHA384:"
        "ECDHE-ECDSA-CHACHA20-POLY1305:"
        "ECDHE-RSA-CHACHA20-POLY1305:"
        "ECDHE-ECDSA-AES128-GCM-SHA256:"
        "ECDHE-RSA-AES128-GCM-SHA256");
    
    // ===== Session Caching =====
    SSL_CTX_set_session_cache_mode(sslCtx, 
        SSL_SESS_CACHE_CLIENT | SSL_SESS_CACHE_NO_INTERNAL_STORE);
    SSL_CTX_sess_set_cache_size(sslCtx, 1024);
    
    // ===== Certificate Verification =====
    SSL_CTX_set_verify(sslCtx, SSL_VERIFY_PEER, nullptr);
    SSL_CTX_set_verify_depth(sslCtx, 10);
    
    // ===== ALPN (HTTP/2 support) =====
    static const unsigned char alpn[] = {
        2, 'h', '2',           // HTTP/2
        8, 'h', 't', 't', 'p', '/', '1', '.', '1'  // HTTP/1.1 fallback
    };
    SSL_CTX_set_alpn_protos(sslCtx, alpn, sizeof(alpn));
    
    // ===== OCSP Stapling =====
    SSL_CTX_set_tlsext_status_type(sslCtx, TLSEXT_STATUSTYPE_ocsp);
    
    // Load system certificates
    loadSystemCertificates();
    
    return true;
}

void SSLContext::shutdown() {
    if (ctx_) {
        SSL_CTX_free(static_cast<SSL_CTX*>(ctx_));
        ctx_ = nullptr;
    }
}

void SSLContext::setMinVersion(TLSVersion version) {
    minVersion_ = version;
    if (ctx_) {
        int v = TLS1_2_VERSION;
        switch (version) {
            case TLSVersion::TLS_1_0: v = TLS1_VERSION; break;
            case TLSVersion::TLS_1_1: v = TLS1_1_VERSION; break;
            case TLSVersion::TLS_1_2: v = TLS1_2_VERSION; break;
            case TLSVersion::TLS_1_3: v = TLS1_3_VERSION; break;
        }
        SSL_CTX_set_min_proto_version(static_cast<SSL_CTX*>(ctx_), v);
    }
}

void SSLContext::setMaxVersion(TLSVersion version) {
    maxVersion_ = version;
    if (ctx_) {
        int v = TLS1_3_VERSION;
        switch (version) {
            case TLSVersion::TLS_1_0: v = TLS1_VERSION; break;
            case TLSVersion::TLS_1_1: v = TLS1_1_VERSION; break;
            case TLSVersion::TLS_1_2: v = TLS1_2_VERSION; break;
            case TLSVersion::TLS_1_3: v = TLS1_3_VERSION; break;
        }
        SSL_CTX_set_max_proto_version(static_cast<SSL_CTX*>(ctx_), v);
    }
}

bool SSLContext::loadSystemCertificates() {
    if (!ctx_) return false;
    
    SSL_CTX* sslCtx = static_cast<SSL_CTX*>(ctx_);
    
    // Try common system certificate locations
    const char* paths[] = {
        "/etc/ssl/certs/ca-certificates.crt",  // Debian/Ubuntu
        "/etc/pki/tls/certs/ca-bundle.crt",    // RHEL/CentOS
        "/etc/ssl/ca-bundle.pem",               // OpenSUSE
        "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",
        nullptr
    };
    
    for (int i = 0; paths[i]; ++i) {
        if (SSL_CTX_load_verify_locations(sslCtx, paths[i], nullptr) == 1) {
            return true;
        }
    }
    
    // Try default paths
    return SSL_CTX_set_default_verify_paths(sslCtx) == 1;
}

bool SSLContext::loadCertificateFile(const std::string& path) {
    if (!ctx_) return false;
    
    SSL_CTX* sslCtx = static_cast<SSL_CTX*>(ctx_);
    return SSL_CTX_load_verify_locations(sslCtx, path.c_str(), nullptr) == 1;
}

bool SSLContext::loadCertificateData(const std::vector<uint8_t>& data) {
    if (!ctx_ || data.empty()) return false;
    
    BIO* bio = BIO_new_mem_buf(data.data(), static_cast<int>(data.size()));
    if (!bio) return false;
    
    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!cert) return false;
    
    X509_STORE* store = SSL_CTX_get_cert_store(static_cast<SSL_CTX*>(ctx_));
    int result = X509_STORE_add_cert(store, cert);
    X509_free(cert);
    
    return result == 1;
}

void SSLContext::setCipherSuites(const std::string& ciphers) {
    if (!ctx_) return;
    SSL_CTX_set_cipher_list(static_cast<SSL_CTX*>(ctx_), ciphers.c_str());
}

void SSLContext::setVerifyHostname(bool verify) {
    verifyHostname_ = verify;
}

void SSLContext::setVerifyCertificate(bool verify) {
    verifyCertificate_ = verify;
    if (ctx_) {
        SSL_CTX* sslCtx = static_cast<SSL_CTX*>(ctx_);
        SSL_CTX_set_verify(sslCtx, 
            verify ? SSL_VERIFY_PEER : SSL_VERIFY_NONE, nullptr);
    }
}

void SSLContext::setVerifyCallback(VerifyCallback callback) {
    verifyCallback_ = std::move(callback);
}

CertVerifyResult SSLContext::verifyCertificate(const std::string& hostname,
                                                const std::vector<uint8_t>& certData) {
    if (certData.empty()) {
        return CertVerifyResult::UNKNOWN_ERROR;
    }
    
    BIO* bio = BIO_new_mem_buf(certData.data(), static_cast<int>(certData.size()));
    if (!bio) return CertVerifyResult::UNKNOWN_ERROR;
    
    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!cert) return CertVerifyResult::UNKNOWN_ERROR;
    
    // Check expiration
    if (X509_cmp_current_time(X509_get_notBefore(cert)) > 0) {
        X509_free(cert);
        return CertVerifyResult::NOT_YET_VALID;
    }
    
    if (X509_cmp_current_time(X509_get_notAfter(cert)) < 0) {
        X509_free(cert);
        return CertVerifyResult::EXPIRED;
    }
    
    // Check hostname
    if (verifyHostname_ && !hostname.empty()) {
        if (X509_check_host(cert, hostname.c_str(), hostname.length(), 0, nullptr) != 1) {
            X509_free(cert);
            return CertVerifyResult::HOSTNAME_MISMATCH;
        }
    }
    
    // Check if self-signed
    if (X509_check_issued(cert, cert) == X509_V_OK) {
        X509_free(cert);
        return CertVerifyResult::SELF_SIGNED;
    }
    
    X509_free(cert);
    return CertVerifyResult::OK;
}

CertificateInfo SSLContext::getCertificateInfo(const std::vector<uint8_t>& certData) {
    CertificateInfo info;
    
    if (certData.empty()) return info;
    
    BIO* bio = BIO_new_mem_buf(certData.data(), static_cast<int>(certData.size()));
    if (!bio) return info;
    
    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!cert) return info;
    
    // Subject
    X509_NAME* subject = X509_get_subject_name(cert);
    char subjectBuf[256] = {0};
    X509_NAME_oneline(subject, subjectBuf, sizeof(subjectBuf));
    info.subject = subjectBuf;
    
    // Issuer
    X509_NAME* issuer = X509_get_issuer_name(cert);
    char issuerBuf[256] = {0};
    X509_NAME_oneline(issuer, issuerBuf, sizeof(issuerBuf));
    info.issuer = issuerBuf;
    
    // Serial number
    ASN1_INTEGER* serial = X509_get_serialNumber(cert);
    BIGNUM* bn = ASN1_INTEGER_to_BN(serial, nullptr);
    if (bn) {
        char* hex = BN_bn2hex(bn);
        if (hex) {
            info.serialNumber = hex;
            OPENSSL_free(hex);
        }
        BN_free(bn);
    }
    
    // Fingerprint (SHA256)
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int mdLen = 0;
    if (X509_digest(cert, EVP_sha256(), md, &mdLen)) {
        std::ostringstream oss;
        for (unsigned int i = 0; i < mdLen; ++i) {
            if (i > 0) oss << ":";
            oss << std::hex << std::uppercase << std::setfill('0') 
                << std::setw(2) << static_cast<int>(md[i]);
        }
        info.fingerprint = oss.str();
    }
    
    // Validity
    ASN1_TIME* notBefore = X509_get_notBefore(cert);
    ASN1_TIME* notAfter = X509_get_notAfter(cert);
    
    BIO* timeBio = BIO_new(BIO_s_mem());
    if (timeBio) {
        ASN1_TIME_print(timeBio, notBefore);
        char timeBuf[64] = {0};
        BIO_read(timeBio, timeBuf, sizeof(timeBuf) - 1);
        info.notBefore = timeBuf;
        
        BIO_reset(timeBio);
        ASN1_TIME_print(timeBio, notAfter);
        std::memset(timeBuf, 0, sizeof(timeBuf));
        BIO_read(timeBio, timeBuf, sizeof(timeBuf) - 1);
        info.notAfter = timeBuf;
        
        BIO_free(timeBio);
    }
    
    // Key bits
    EVP_PKEY* pkey = X509_get_pubkey(cert);
    if (pkey) {
        info.keyBits = EVP_PKEY_bits(pkey);
        EVP_PKEY_free(pkey);
    }
    
    // Is CA
    info.isCA = X509_check_ca(cert) > 0;
    
    X509_free(cert);
    return info;
}

std::unique_ptr<SSLContext::SSLSocket> SSLContext::createSocket(int fd) {
    return std::make_unique<SSLSocket>(this, fd);
}

std::string SSLContext::getLastError() {
    char buf[256];
    ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
    return buf;
}

// =============================================================================
// SSLSocket Implementation
// =============================================================================

SSLContext::SSLSocket::SSLSocket(SSLContext* ctx, int fd) 
    : ctx_(ctx), fd_(fd) {
    
    if (ctx_->ctx_) {
        ssl_ = SSL_new(static_cast<SSL_CTX*>(ctx_->ctx_));
        if (ssl_) {
            SSL_set_fd(static_cast<SSL*>(ssl_), fd);
        }
    }
}

SSLContext::SSLSocket::~SSLSocket() {
    close();
}

bool SSLContext::SSLSocket::connect(const std::string& hostname) {
    if (!ssl_) return false;
    
    SSL* ssl = static_cast<SSL*>(ssl_);
    
    // Set hostname for SNI
    SSL_set_tlsext_host_name(ssl, hostname.c_str());
    
    // Set hostname for verification
    SSL_set1_host(ssl, hostname.c_str());
    
    // Perform handshake
    int result = SSL_connect(ssl);
    if (result != 1) {
        int err = SSL_get_error(ssl, result);
        verifyResult_ = CertVerifyResult::UNKNOWN_ERROR;
        return false;
    }
    
    // Verify certificate
    long verifyResult = SSL_get_verify_result(ssl);
    switch (verifyResult) {
        case X509_V_OK:
            verifyResult_ = CertVerifyResult::OK;
            break;
        case X509_V_ERR_CERT_HAS_EXPIRED:
            verifyResult_ = CertVerifyResult::EXPIRED;
            break;
        case X509_V_ERR_CERT_NOT_YET_VALID:
            verifyResult_ = CertVerifyResult::NOT_YET_VALID;
            break;
        case X509_V_ERR_CERT_REVOKED:
            verifyResult_ = CertVerifyResult::REVOKED;
            break;
        case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
        case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
            verifyResult_ = CertVerifyResult::SELF_SIGNED;
            break;
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
            verifyResult_ = CertVerifyResult::UNTRUSTED_ROOT;
            break;
        default:
            verifyResult_ = CertVerifyResult::UNKNOWN_ERROR;
    }
    
    return verifyResult_ == CertVerifyResult::OK;
}

bool SSLContext::SSLSocket::accept() {
    if (!ssl_) return false;
    return SSL_accept(static_cast<SSL*>(ssl_)) == 1;
}

int SSLContext::SSLSocket::read(void* buffer, size_t size) {
    if (!ssl_) return -1;
    return SSL_read(static_cast<SSL*>(ssl_), buffer, static_cast<int>(size));
}

int SSLContext::SSLSocket::write(const void* buffer, size_t size) {
    if (!ssl_) return -1;
    return SSL_write(static_cast<SSL*>(ssl_), buffer, static_cast<int>(size));
}

void SSLContext::SSLSocket::close() {
    if (ssl_) {
        SSL_shutdown(static_cast<SSL*>(ssl_));
        SSL_free(static_cast<SSL*>(ssl_));
        ssl_ = nullptr;
    }
}

SSLConnectionInfo SSLContext::SSLSocket::getConnectionInfo() const {
    SSLConnectionInfo info;
    
    if (!ssl_) return info;
    
    SSL* ssl = static_cast<SSL*>(ssl_);
    
    // Protocol version
    int version = SSL_version(ssl);
    switch (version) {
        case TLS1_VERSION: info.version = TLSVersion::TLS_1_0; break;
        case TLS1_1_VERSION: info.version = TLSVersion::TLS_1_1; break;
        case TLS1_2_VERSION: info.version = TLSVersion::TLS_1_2; break;
        case TLS1_3_VERSION: info.version = TLSVersion::TLS_1_3; break;
    }
    
    // Cipher
    const SSL_CIPHER* cipher = SSL_get_current_cipher(ssl);
    if (cipher) {
        info.cipherSuite = SSL_CIPHER_get_name(cipher);
        info.cipherStrength = SSL_CIPHER_get_bits(cipher, nullptr);
    }
    
    // Session reused
    info.sessionReused = SSL_session_reused(ssl) != 0;
    
    return info;
}

CertVerifyResult SSLContext::SSLSocket::getVerifyResult() const {
    return verifyResult_;
}

// =============================================================================
// Global Instance
// =============================================================================

SSLContext& getSSLContext() {
    static SSLContext instance;
    static bool initialized = false;
    
    if (!initialized) {
        instance.initialize();
        initialized = true;
    }
    
    return instance;
}

} // namespace Zepra::Networking
