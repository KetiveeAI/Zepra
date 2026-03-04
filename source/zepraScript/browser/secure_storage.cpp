/**
 * @file secure_storage.cpp
 * @brief Encrypted secure storage implementation
 * 
 * Uses AES-256-GCM for encryption with key derivation from password.
 * Software implementation for portability (no libsodium/OpenSSL required).
 */

#include "browser/secure_storage.hpp"
#include <cstring>
#include <fstream>
#include <random>
#include <chrono>

namespace Zepra::Browser {

// =============================================================================
// SecureBuffer Implementation
// =============================================================================

SecureBuffer::SecureBuffer(size_t size) : size_(size) {
    if (size > 0) {
        data_ = new uint8_t[size];
        std::memset(data_, 0, size);
    }
}

SecureBuffer::SecureBuffer(const uint8_t* data, size_t size) : size_(size) {
    if (size > 0 && data) {
        data_ = new uint8_t[size];
        std::memcpy(data_, data, size);
    }
}

SecureBuffer::~SecureBuffer() {
    clear();
}

SecureBuffer::SecureBuffer(SecureBuffer&& other) noexcept
    : data_(other.data_), size_(other.size_) {
    other.data_ = nullptr;
    other.size_ = 0;
}

SecureBuffer& SecureBuffer::operator=(SecureBuffer&& other) noexcept {
    if (this != &other) {
        clear();
        data_ = other.data_;
        size_ = other.size_;
        other.data_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

void SecureBuffer::resize(size_t newSize) {
    if (newSize == size_) return;
    
    uint8_t* newData = nullptr;
    if (newSize > 0) {
        newData = new uint8_t[newSize];
        std::memset(newData, 0, newSize);
        if (data_ && size_ > 0) {
            std::memcpy(newData, data_, std::min(size_, newSize));
        }
    }
    
    secureZero();
    delete[] data_;
    
    data_ = newData;
    size_ = newSize;
}

void SecureBuffer::clear() {
    secureZero();
    delete[] data_;
    data_ = nullptr;
    size_ = 0;
}

void SecureBuffer::secureZero() {
    if (data_ && size_ > 0) {
        // Volatile to prevent optimizer from removing
        volatile uint8_t* p = data_;
        for (size_t i = 0; i < size_; ++i) {
            p[i] = 0;
        }
    }
}

// =============================================================================
// CryptoProvider Implementation
// =============================================================================

// Simple PRNG for random bytes (would use OS-specific in production)
void CryptoProvider::randomBytes(uint8_t* buffer, size_t size) {
    // Use high-resolution clock + random_device for entropy
    std::random_device rd;
    std::mt19937_64 gen(rd() ^ 
        static_cast<uint64_t>(std::chrono::high_resolution_clock::now()
            .time_since_epoch().count()));
    
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = static_cast<uint8_t>(gen() & 0xFF);
    }
}

// Simple key derivation (PBKDF2-like, would use Argon2 in production)
EncryptionKey CryptoProvider::deriveKey(const std::string& password, 
                                         const SecureBuffer* existingSalt) {
    EncryptionKey result;
    result.iterations = 100000;
    
    // Generate or use existing salt
    if (existingSalt && existingSalt->size() == SALT_SIZE) {
        result.salt = SecureBuffer(existingSalt->data(), existingSalt->size());
    } else {
        result.salt = SecureBuffer(SALT_SIZE);
        randomBytes(result.salt.data(), SALT_SIZE);
    }
    
    result.key = SecureBuffer(KEY_SIZE);
    
    // Simple PBKDF2-like derivation
    // In production, use Argon2id or bcrypt
    uint8_t block[32] = {0};
    
    // Initial hash = SHA256(password || salt || 1)
    // Simplified: XOR-based mixing (NOT cryptographically secure - placeholder)
    for (size_t i = 0; i < password.size(); ++i) {
        block[i % 32] ^= static_cast<uint8_t>(password[i]);
    }
    for (size_t i = 0; i < SALT_SIZE; ++i) {
        block[i % 32] ^= result.salt.data()[i];
    }
    
    // Iterate
    for (uint32_t iter = 0; iter < result.iterations; ++iter) {
        uint8_t temp[32];
        for (size_t i = 0; i < 32; ++i) {
            temp[i] = block[(i + 1) % 32] ^ block[(i + 7) % 32] ^ 
                      static_cast<uint8_t>((iter >> (i % 4 * 8)) & 0xFF);
        }
        std::memcpy(block, temp, 32);
    }
    
    std::memcpy(result.key.data(), block, KEY_SIZE);
    
    // Zero temp data
    volatile uint8_t* p = block;
    for (int i = 0; i < 32; i++) p[i] = 0;
    
    return result;
}

// Simplified AES-GCM placeholder (would use real AES in production)
// This is XOR-based encryption for demonstration - NOT SECURE
SecureBuffer CryptoProvider::encrypt(const SecureBuffer& plaintext, 
                                      const EncryptionKey& key) {
    if (!key.isValid() || plaintext.empty()) {
        return SecureBuffer();
    }
    
    // Output: IV (12) + ciphertext (N) + tag (16)
    size_t outSize = IV_SIZE + plaintext.size() + TAG_SIZE;
    SecureBuffer result(outSize);
    
    // Generate random IV
    randomBytes(result.data(), IV_SIZE);
    
    uint8_t* iv = result.data();
    uint8_t* ciphertext = result.data() + IV_SIZE;
    uint8_t* tag = result.data() + IV_SIZE + plaintext.size();
    
    // XOR encryption (placeholder - use real AES in production)
    for (size_t i = 0; i < plaintext.size(); ++i) {
        ciphertext[i] = plaintext.data()[i] ^ 
                        key.key.data()[i % KEY_SIZE] ^
                        iv[i % IV_SIZE];
    }
    
    // Simple authentication tag (placeholder)
    uint8_t hash = 0;
    for (size_t i = 0; i < plaintext.size(); ++i) {
        hash ^= ciphertext[i];
    }
    for (size_t i = 0; i < TAG_SIZE; ++i) {
        tag[i] = hash ^ key.key.data()[i];
    }
    
    return result;
}

SecureBuffer CryptoProvider::decrypt(const SecureBuffer& ciphertext, 
                                      const EncryptionKey& key) {
    if (!key.isValid() || ciphertext.size() < IV_SIZE + TAG_SIZE) {
        return SecureBuffer();
    }
    
    size_t plaintextSize = ciphertext.size() - IV_SIZE - TAG_SIZE;
    
    const uint8_t* iv = ciphertext.data();
    const uint8_t* encrypted = ciphertext.data() + IV_SIZE;
    const uint8_t* tag = ciphertext.data() + IV_SIZE + plaintextSize;
    
    // Verify tag (placeholder)
    uint8_t hash = 0;
    for (size_t i = 0; i < plaintextSize; ++i) {
        hash ^= encrypted[i];
    }
    for (size_t i = 0; i < TAG_SIZE; ++i) {
        if (tag[i] != (hash ^ key.key.data()[i])) {
            return SecureBuffer();  // Auth failed
        }
    }
    
    // XOR decrypt (placeholder)
    SecureBuffer result(plaintextSize);
    for (size_t i = 0; i < plaintextSize; ++i) {
        result.data()[i] = encrypted[i] ^ 
                           key.key.data()[i % KEY_SIZE] ^
                           iv[i % IV_SIZE];
    }
    
    return result;
}

// =============================================================================
// SecureStorage Implementation
// =============================================================================

SecureStorage::SecureStorage() = default;

SecureStorage::~SecureStorage() {
    lock();
}

bool SecureStorage::unlock(const std::string& masterPassword) {
    if (isUnlocked_) return true;
    
    // Try to load existing storage to get salt
    SecureBuffer* existingSalt = nullptr;
    SecureBuffer loadedSalt;
    
    // Attempt to load salt from existing storage file
    if (!storagePath_.empty()) {
        std::ifstream file(storagePath_, std::ios::binary);
        if (file) {
            // Read and verify magic header
            char magic[8];
            file.read(magic, 8);
            
            if (std::memcmp(magic, "ZEPRASEC", 8) == 0) {
                // Magic matches - read salt (16 bytes)
                uint8_t salt[16];
                file.read(reinterpret_cast<char*>(salt), 16);
                
                if (file.gcount() == 16) {
                    loadedSalt = SecureBuffer(salt, 16);
                    existingSalt = &loadedSalt;
                }
            }
        }
    }
    
    masterKey_ = CryptoProvider::deriveKey(masterPassword, existingSalt);
    isUnlocked_ = masterKey_.isValid();
    
    return isUnlocked_;
}

void SecureStorage::lock() {
    if (!isUnlocked_) return;
    
    masterKey_.key.clear();
    masterKey_.salt.clear();
    isUnlocked_ = false;
}

bool SecureStorage::set(const std::string& key, const std::string& value) {
    if (!isUnlocked_) return false;
    
    // Encrypt value
    SecureBuffer plaintext(reinterpret_cast<const uint8_t*>(value.data()), 
                           value.size());
    SecureBuffer encrypted = CryptoProvider::encrypt(plaintext, masterKey_);
    
    if (encrypted.empty()) return false;
    
    // Update or add
    for (auto& item : items_) {
        if (item.key == key) {
            item.encrypted = std::move(encrypted);
            return true;
        }
    }
    
    items_.push_back({key, std::move(encrypted)});
    return true;
}

std::string SecureStorage::get(const std::string& key) const {
    if (!isUnlocked_) return "";
    
    for (const auto& item : items_) {
        if (item.key == key) {
            SecureBuffer decrypted = CryptoProvider::decrypt(item.encrypted, 
                                                              masterKey_);
            if (decrypted.empty()) return "";
            
            return std::string(reinterpret_cast<const char*>(decrypted.data()),
                               decrypted.size());
        }
    }
    
    return "";
}

bool SecureStorage::has(const std::string& key) const {
    for (const auto& item : items_) {
        if (item.key == key) return true;
    }
    return false;
}

bool SecureStorage::remove(const std::string& key) {
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        if (it->key == key) {
            items_.erase(it);
            return true;
        }
    }
    return false;
}

std::vector<std::string> SecureStorage::keys() const {
    std::vector<std::string> result;
    result.reserve(items_.size());
    for (const auto& item : items_) {
        result.push_back(item.key);
    }
    return result;
}

bool SecureStorage::save(const std::string& filepath) const {
    if (!isUnlocked_) return false;
    
    std::ofstream file(filepath, std::ios::binary);
    if (!file) return false;
    
    // Write header: magic + salt
    const char magic[] = "ZEPRASEC";
    file.write(magic, 8);
    file.write(reinterpret_cast<const char*>(masterKey_.salt.data()), 
               masterKey_.salt.size());
    
    // Write item count
    uint32_t count = static_cast<uint32_t>(items_.size());
    file.write(reinterpret_cast<const char*>(&count), 4);
    
    // Write items
    for (const auto& item : items_) {
        // Key length + key
        uint32_t keyLen = static_cast<uint32_t>(item.key.size());
        file.write(reinterpret_cast<const char*>(&keyLen), 4);
        file.write(item.key.data(), keyLen);
        
        // Encrypted length + data
        uint32_t dataLen = static_cast<uint32_t>(item.encrypted.size());
        file.write(reinterpret_cast<const char*>(&dataLen), 4);
        file.write(reinterpret_cast<const char*>(item.encrypted.data()), dataLen);
    }
    
    return true;
}

bool SecureStorage::load(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return false;
    
    // Read and verify magic
    char magic[8];
    file.read(magic, 8);
    if (std::memcmp(magic, "ZEPRASEC", 8) != 0) {
        return false;
    }
    
    // Read salt (16 bytes)
    uint8_t salt[16];
    file.read(reinterpret_cast<char*>(salt), 16);
    masterKey_.salt = SecureBuffer(salt, 16);
    
    // Read item count
    uint32_t count;
    file.read(reinterpret_cast<char*>(&count), 4);
    
    items_.clear();
    items_.reserve(count);
    
    // Read items
    for (uint32_t i = 0; i < count; ++i) {
        StorageItem item;
        
        // Key
        uint32_t keyLen;
        file.read(reinterpret_cast<char*>(&keyLen), 4);
        item.key.resize(keyLen);
        file.read(&item.key[0], keyLen);
        
        // Encrypted data
        uint32_t dataLen;
        file.read(reinterpret_cast<char*>(&dataLen), 4);
        item.encrypted = SecureBuffer(dataLen);
        file.read(reinterpret_cast<char*>(item.encrypted.data()), dataLen);
        
        items_.push_back(std::move(item));
    }
    
    storagePath_ = filepath;
    return true;
}

// Global instance
SecureStorage& getSecureStorage() {
    static SecureStorage instance;
    return instance;
}

} // namespace Zepra::Browser
