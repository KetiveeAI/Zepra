#pragma once

/**
 * @file secure_storage.hpp
 * @brief Encrypted storage for sensitive user data
 * 
 * All data is encrypted with AES-256-GCM before storage.
 * Keys are derived from master password using Argon2id.
 * Memory is zeroed after use to prevent cold boot attacks.
 */

#include "../config.hpp"
#include "runtime/objects/value.hpp"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>

namespace Zepra::Browser {

/**
 * @brief Secure memory buffer that zeros on destruction
 */
class SecureBuffer {
public:
    SecureBuffer() = default;
    explicit SecureBuffer(size_t size);
    SecureBuffer(const uint8_t* data, size_t size);
    ~SecureBuffer();
    
    // Non-copyable
    SecureBuffer(const SecureBuffer&) = delete;
    SecureBuffer& operator=(const SecureBuffer&) = delete;
    
    // Moveable
    SecureBuffer(SecureBuffer&& other) noexcept;
    SecureBuffer& operator=(SecureBuffer&& other) noexcept;
    
    uint8_t* data() { return data_; }
    const uint8_t* data() const { return data_; }
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    
    void resize(size_t newSize);
    void clear();
    
private:
    void secureZero();
    
    uint8_t* data_ = nullptr;
    size_t size_ = 0;
};

/**
 * @brief Encryption key for secure storage
 */
struct EncryptionKey {
    SecureBuffer key;      // 32 bytes for AES-256
    SecureBuffer salt;     // 16 bytes
    uint32_t iterations;   // Key derivation iterations
    
    bool isValid() const { return key.size() == 32; }
};

/**
 * @brief AES-256-GCM encryption/decryption
 */
class CryptoProvider {
public:
    static constexpr size_t KEY_SIZE = 32;      // AES-256
    static constexpr size_t IV_SIZE = 12;       // GCM nonce
    static constexpr size_t TAG_SIZE = 16;      // GCM auth tag
    static constexpr size_t SALT_SIZE = 16;
    
    /**
     * @brief Derive key from password using Argon2id
     */
    static EncryptionKey deriveKey(const std::string& password, 
                                    const SecureBuffer* existingSalt = nullptr);
    
    /**
     * @brief Encrypt data with AES-256-GCM
     * @return Encrypted data: IV (12) + ciphertext + tag (16)
     */
    static SecureBuffer encrypt(const SecureBuffer& plaintext, 
                                 const EncryptionKey& key);
    
    /**
     * @brief Decrypt data with AES-256-GCM
     * @return Plaintext, or empty on failure
     */
    static SecureBuffer decrypt(const SecureBuffer& ciphertext, 
                                 const EncryptionKey& key);
    
    /**
     * @brief Generate cryptographically secure random bytes
     */
    static void randomBytes(uint8_t* buffer, size_t size);
    
private:
    // Software implementation (no external crypto lib dependency)
    static void aesGcmEncrypt(const uint8_t* key, const uint8_t* iv,
                              const uint8_t* plaintext, size_t len,
                              uint8_t* ciphertext, uint8_t* tag);
    
    static bool aesGcmDecrypt(const uint8_t* key, const uint8_t* iv,
                              const uint8_t* ciphertext, size_t len,
                              const uint8_t* tag, uint8_t* plaintext);
};

/**
 * @brief Encrypted key-value storage
 */
class SecureStorage {
public:
    SecureStorage();
    ~SecureStorage();
    
    /**
     * @brief Initialize storage with master password
     * @return true if storage unlocked successfully
     */
    bool unlock(const std::string& masterPassword);
    
    /**
     * @brief Lock storage (zeroes key in memory)
     */
    void lock();
    
    /**
     * @brief Check if storage is unlocked
     */
    bool isUnlocked() const { return isUnlocked_; }
    
    /**
     * @brief Store encrypted value
     */
    bool set(const std::string& key, const std::string& value);
    
    /**
     * @brief Retrieve and decrypt value
     */
    std::string get(const std::string& key) const;
    
    /**
     * @brief Check if key exists
     */
    bool has(const std::string& key) const;
    
    /**
     * @brief Remove key
     */
    bool remove(const std::string& key);
    
    /**
     * @brief Get all keys (not values)
     */
    std::vector<std::string> keys() const;
    
    /**
     * @brief Save storage to file
     */
    bool save(const std::string& filepath) const;
    
    /**
     * @brief Load storage from file
     */
    bool load(const std::string& filepath);
    
private:
    struct StorageItem {
        std::string key;        // Plain (for lookup)
        SecureBuffer encrypted; // Encrypted value
    };
    
    std::vector<StorageItem> items_;
    EncryptionKey masterKey_;
    bool isUnlocked_ = false;
    std::string storagePath_;
};

/**
 * @brief Global secure storage instance
 */
SecureStorage& getSecureStorage();

} // namespace Zepra::Browser
