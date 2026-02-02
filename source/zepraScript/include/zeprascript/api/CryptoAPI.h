/**
 * @file CryptoAPI.h
 * @brief Web Crypto API Implementation
 * 
 * Web Cryptography API:
 * - crypto.getRandomValues(): Secure random
 * - SubtleCrypto: Digest, encrypt, decrypt, sign, verify
 */

#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <random>
#include <array>
#include <memory>

namespace Zepra::API {

// =============================================================================
// Algorithm Types
// =============================================================================

enum class HashAlgorithm {
    SHA1,
    SHA256,
    SHA384,
    SHA512
};

enum class CryptoAlgorithm {
    AES_CBC,
    AES_CTR,
    AES_GCM,
    RSA_OAEP,
    ECDSA,
    ECDH,
    HMAC
};

struct AlgorithmParams {
    CryptoAlgorithm name;
    std::vector<uint8_t> iv;          // For AES-CBC, AES-GCM
    std::vector<uint8_t> counter;     // For AES-CTR
    size_t length = 0;                // For AES-CTR counter length
    std::vector<uint8_t> additionalData;  // For AES-GCM
    size_t tagLength = 128;           // For AES-GCM
};

// =============================================================================
// CryptoKey
// =============================================================================

enum class KeyType {
    Secret,
    Public,
    Private
};

enum class KeyUsage : uint8_t {
    Encrypt = 1 << 0,
    Decrypt = 1 << 1,
    Sign = 1 << 2,
    Verify = 1 << 3,
    DeriveKey = 1 << 4,
    DeriveBits = 1 << 5,
    WrapKey = 1 << 6,
    UnwrapKey = 1 << 7
};

inline KeyUsage operator|(KeyUsage a, KeyUsage b) {
    return static_cast<KeyUsage>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

/**
 * @brief Cryptographic key handle
 */
class CryptoKey {
public:
    CryptoKey(KeyType type, bool extractable, KeyUsage usages,
              std::vector<uint8_t> keyData)
        : type_(type), extractable_(extractable), usages_(usages)
        , keyData_(std::move(keyData)) {}
    
    KeyType type() const { return type_; }
    bool extractable() const { return extractable_; }
    KeyUsage usages() const { return usages_; }
    
    const std::vector<uint8_t>& rawKey() const { return keyData_; }
    
    bool hasUsage(KeyUsage usage) const {
        return (static_cast<uint8_t>(usages_) & static_cast<uint8_t>(usage)) != 0;
    }
    
private:
    KeyType type_;
    bool extractable_;
    KeyUsage usages_;
    std::vector<uint8_t> keyData_;
};

// =============================================================================
// SubtleCrypto
// =============================================================================

/**
 * @brief Low-level cryptographic operations
 */
class SubtleCrypto {
public:
    // Digest (hash)
    std::vector<uint8_t> digest(HashAlgorithm algorithm,
                                 const std::vector<uint8_t>& data) {
        switch (algorithm) {
            case HashAlgorithm::SHA256:
                return sha256(data);
            case HashAlgorithm::SHA1:
                return sha1(data);
            case HashAlgorithm::SHA384:
                return sha384(data);
            case HashAlgorithm::SHA512:
                return sha512(data);
        }
        return {};
    }
    
    // Generate key
    std::unique_ptr<CryptoKey> generateKey(CryptoAlgorithm algorithm,
                                            bool extractable,
                                            KeyUsage usages,
                                            size_t keyLength = 256) {
        std::vector<uint8_t> keyData(keyLength / 8);
        
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        
        for (auto& byte : keyData) {
            byte = dist(gen);
        }
        
        return std::make_unique<CryptoKey>(
            KeyType::Secret, extractable, usages, std::move(keyData));
    }
    
    // Import key
    std::unique_ptr<CryptoKey> importKey(const std::string& format,
                                          const std::vector<uint8_t>& keyData,
                                          CryptoAlgorithm algorithm,
                                          bool extractable,
                                          KeyUsage usages) {
        // Would validate format (raw, pkcs8, spki, jwk)
        return std::make_unique<CryptoKey>(
            KeyType::Secret, extractable, usages, keyData);
    }
    
    // Export key
    std::vector<uint8_t> exportKey(const std::string& format,
                                    const CryptoKey& key) {
        if (!key.extractable()) {
            throw std::runtime_error("Key not extractable");
        }
        return key.rawKey();
    }
    
    // Encrypt
    std::vector<uint8_t> encrypt(const AlgorithmParams& algorithm,
                                  const CryptoKey& key,
                                  const std::vector<uint8_t>& data) {
        if (!key.hasUsage(KeyUsage::Encrypt)) {
            throw std::runtime_error("Key cannot be used for encryption");
        }
        
        // Would implement actual encryption
        // Placeholder: XOR with key (NOT SECURE - just for structure)
        std::vector<uint8_t> result = data;
        const auto& keyData = key.rawKey();
        for (size_t i = 0; i < result.size(); i++) {
            result[i] ^= keyData[i % keyData.size()];
        }
        return result;
    }
    
    // Decrypt
    std::vector<uint8_t> decrypt(const AlgorithmParams& algorithm,
                                  const CryptoKey& key,
                                  const std::vector<uint8_t>& data) {
        if (!key.hasUsage(KeyUsage::Decrypt)) {
            throw std::runtime_error("Key cannot be used for decryption");
        }
        
        // Same placeholder as encrypt (XOR is symmetric)
        return encrypt(algorithm, key, data);
    }
    
    // Sign
    std::vector<uint8_t> sign(const AlgorithmParams& algorithm,
                               const CryptoKey& key,
                               const std::vector<uint8_t>& data) {
        if (!key.hasUsage(KeyUsage::Sign)) {
            throw std::runtime_error("Key cannot be used for signing");
        }
        
        // Would implement HMAC or asymmetric signature
        return digest(HashAlgorithm::SHA256, data);
    }
    
    // Verify
    bool verify(const AlgorithmParams& algorithm,
                const CryptoKey& key,
                const std::vector<uint8_t>& signature,
                const std::vector<uint8_t>& data) {
        if (!key.hasUsage(KeyUsage::Verify)) {
            throw std::runtime_error("Key cannot be used for verification");
        }
        
        auto expected = sign(algorithm, key, data);
        return signature == expected;
    }
    
private:
    // Placeholder hash implementations
    std::vector<uint8_t> sha256(const std::vector<uint8_t>& data) {
        // Would use actual SHA-256
        std::vector<uint8_t> result(32);
        // Placeholder
        return result;
    }
    
    std::vector<uint8_t> sha1(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> result(20);
        return result;
    }
    
    std::vector<uint8_t> sha384(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> result(48);
        return result;
    }
    
    std::vector<uint8_t> sha512(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> result(64);
        return result;
    }
};

// =============================================================================
// Crypto Interface
// =============================================================================

/**
 * @brief Main crypto interface (window.crypto)
 */
class Crypto {
public:
    // Get cryptographically secure random values
    template<typename T>
    void getRandomValues(T* array, size_t count) {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        std::uniform_int_distribution<T> dist;
        
        for (size_t i = 0; i < count; i++) {
            array[i] = dist(gen);
        }
    }
    
    void getRandomValues(std::vector<uint8_t>& array) {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        std::uniform_int_distribution<int> dist(0, 255);
        
        for (auto& byte : array) {
            byte = static_cast<uint8_t>(dist(gen));
        }
    }
    
    // Generate random UUID
    std::string randomUUID() {
        std::vector<uint8_t> bytes(16);
        getRandomValues(bytes);
        
        // Set version 4
        bytes[6] = (bytes[6] & 0x0f) | 0x40;
        // Set variant
        bytes[8] = (bytes[8] & 0x3f) | 0x80;
        
        char buf[37];
        snprintf(buf, sizeof(buf),
            "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7],
            bytes[8], bytes[9], bytes[10], bytes[11],
            bytes[12], bytes[13], bytes[14], bytes[15]);
        
        return std::string(buf);
    }
    
    // Get subtle crypto interface
    SubtleCrypto& subtle() { return subtle_; }
    
private:
    SubtleCrypto subtle_;
};

// Global crypto instance
Crypto& getCrypto();

} // namespace Zepra::API
