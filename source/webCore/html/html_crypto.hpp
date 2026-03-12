/**
 * @file html_crypto.hpp
 * @brief Web Crypto API (simplified interface)
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <memory>

namespace Zepra::WebCore {

/**
 * @brief Crypto key type
 */
enum class CryptoKeyType {
    Public,
    Private,
    Secret
};

/**
 * @brief Key usage
 */
enum class CryptoKeyUsage {
    Encrypt,
    Decrypt,
    Sign,
    Verify,
    DeriveKey,
    DeriveBits,
    WrapKey,
    UnwrapKey
};

/**
 * @brief Key algorithm
 */
struct CryptoKeyAlgorithm {
    std::string name;
    // Additional algorithm-specific parameters
};

/**
 * @brief Crypto key
 */
class CryptoKey {
public:
    CryptoKeyType type() const { return type_; }
    bool extractable() const { return extractable_; }
    CryptoKeyAlgorithm algorithm() const { return algorithm_; }
    std::vector<CryptoKeyUsage> usages() const { return usages_; }
    
private:
    CryptoKeyType type_;
    bool extractable_ = false;
    CryptoKeyAlgorithm algorithm_;
    std::vector<CryptoKeyUsage> usages_;
    std::vector<uint8_t> keyData_;
    
    friend class SubtleCrypto;
};

/**
 * @brief Crypto key pair
 */
struct CryptoKeyPair {
    std::unique_ptr<CryptoKey> publicKey;
    std::unique_ptr<CryptoKey> privateKey;
};

/**
 * @brief Subtle crypto operations
 */
class SubtleCrypto {
public:
    // Encryption
    void encrypt(const std::string& algorithm,
                 const CryptoKey& key,
                 const std::vector<uint8_t>& data,
                 std::function<void(const std::vector<uint8_t>&)> callback);
    
    void decrypt(const std::string& algorithm,
                 const CryptoKey& key,
                 const std::vector<uint8_t>& data,
                 std::function<void(const std::vector<uint8_t>&)> callback);
    
    // Signing
    void sign(const std::string& algorithm,
              const CryptoKey& key,
              const std::vector<uint8_t>& data,
              std::function<void(const std::vector<uint8_t>&)> callback);
    
    void verify(const std::string& algorithm,
                const CryptoKey& key,
                const std::vector<uint8_t>& signature,
                const std::vector<uint8_t>& data,
                std::function<void(bool)> callback);
    
    // Digest
    void digest(const std::string& algorithm,
                const std::vector<uint8_t>& data,
                std::function<void(const std::vector<uint8_t>&)> callback);
    
    // Key generation
    void generateKey(const std::string& algorithm,
                     bool extractable,
                     const std::vector<CryptoKeyUsage>& usages,
                     std::function<void(CryptoKey*)> callback);
    
    void generateKeyPair(const std::string& algorithm,
                         bool extractable,
                         const std::vector<CryptoKeyUsage>& usages,
                         std::function<void(CryptoKeyPair*)> callback);
    
    // Key import/export
    void importKey(const std::string& format,
                   const std::vector<uint8_t>& keyData,
                   const std::string& algorithm,
                   bool extractable,
                   const std::vector<CryptoKeyUsage>& usages,
                   std::function<void(CryptoKey*)> callback);
    
    void exportKey(const std::string& format,
                   const CryptoKey& key,
                   std::function<void(const std::vector<uint8_t>&)> callback);
    
    // Key derivation
    void deriveKey(const std::string& algorithm,
                   const CryptoKey& baseKey,
                   const std::string& derivedKeyType,
                   bool extractable,
                   const std::vector<CryptoKeyUsage>& usages,
                   std::function<void(CryptoKey*)> callback);
    
    void deriveBits(const std::string& algorithm,
                    const CryptoKey& baseKey,
                    unsigned long length,
                    std::function<void(const std::vector<uint8_t>&)> callback);
};

/**
 * @brief Crypto object
 */
class Crypto {
public:
    Crypto() = default;
    ~Crypto() = default;
    
    SubtleCrypto& subtle() { return subtle_; }
    
    std::vector<uint8_t> getRandomValues(size_t length);
    std::string randomUUID();
    
private:
    SubtleCrypto subtle_;
};

} // namespace Zepra::WebCore
