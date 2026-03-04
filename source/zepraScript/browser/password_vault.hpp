#pragma once

/**
 * @file password_vault.hpp
 * @brief Secure password manager for browser autofill
 * 
 * Stores credentials encrypted with per-site keys.
 * Master password unlocks the vault but is never stored.
 */

#include "secure_storage.hpp"
#include <string>
#include <vector>
#include <optional>

namespace Zepra::Browser {

/**
 * @brief Stored credential
 */
struct Credential {
    std::string site;       // e.g., "example.com"
    std::string username;
    std::string password;
    std::string formFields; // JSON of additional form fields
    int64_t lastUsed;       // Unix timestamp
    int64_t created;
};

/**
 * @brief Password vault for secure credential storage
 */
class PasswordVault {
public:
    PasswordVault();
    ~PasswordVault();
    
    /**
     * @brief Unlock vault with master password
     */
    bool unlock(const std::string& masterPassword);
    
    /**
     * @brief Lock vault (secure erase keys)
     */
    void lock();
    
    /**
     * @brief Check if vault is unlocked
     */
    bool isUnlocked() const { return storage_.isUnlocked(); }
    
    /**
     * @brief Save credential for site
     */
    bool saveCredential(const Credential& cred);
    
    /**
     * @brief Get credentials for site
     */
    std::vector<Credential> getCredentials(const std::string& site) const;
    
    /**
     * @brief Get all saved sites
     */
    std::vector<std::string> getSites() const;
    
    /**
     * @brief Delete credential
     */
    bool deleteCredential(const std::string& site, const std::string& username);
    
    /**
     * @brief Update last used timestamp
     */
    void markUsed(const std::string& site, const std::string& username);
    
    /**
     * @brief Generate secure random password
     */
    static std::string generatePassword(size_t length = 16, 
                                         bool includeSymbols = true);
    
    /**
     * @brief Check password strength (0-100)
     */
    static int passwordStrength(const std::string& password);
    
    /**
     * @brief Save vault to disk
     */
    bool save(const std::string& filepath);
    
    /**
     * @brief Load vault from disk
     */
    bool load(const std::string& filepath);
    
private:
    SecureStorage storage_;
    
    std::string encodeCredential(const Credential& cred) const;
    Credential decodeCredential(const std::string& encoded) const;
    
    std::string siteKey(const std::string& site) const;
};

/**
 * @brief Global password vault instance
 */
PasswordVault& getPasswordVault();

} // namespace Zepra::Browser
