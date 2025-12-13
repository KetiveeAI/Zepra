/**
 * @file password_vault.cpp
 * @brief Password vault implementation
 */

#include "zeprascript/browser/password_vault.hpp"
#include <algorithm>
#include <sstream>
#include <ctime>
#include <cctype>

namespace Zepra::Browser {

PasswordVault::PasswordVault() = default;
PasswordVault::~PasswordVault() = default;

bool PasswordVault::unlock(const std::string& masterPassword) {
    return storage_.unlock(masterPassword);
}

void PasswordVault::lock() {
    storage_.lock();
}

std::string PasswordVault::siteKey(const std::string& site) const {
    return "cred:" + site;
}

std::string PasswordVault::encodeCredential(const Credential& cred) const {
    // Simple JSON-like encoding
    std::ostringstream ss;
    ss << "{\"u\":\"" << cred.username << "\","
       << "\"p\":\"" << cred.password << "\","
       << "\"f\":\"" << cred.formFields << "\","
       << "\"l\":" << cred.lastUsed << ","
       << "\"c\":" << cred.created << "}";
    return ss.str();
}

Credential PasswordVault::decodeCredential(const std::string& encoded) const {
    Credential cred;
    
    // Simple parsing (would use proper JSON parser in production)
    auto findValue = [&encoded](const std::string& key) -> std::string {
        std::string needle = "\"" + key + "\":";
        size_t pos = encoded.find(needle);
        if (pos == std::string::npos) return "";
        
        pos += needle.length();
        if (pos >= encoded.length()) return "";
        
        if (encoded[pos] == '"') {
            // String value
            pos++;
            size_t end = encoded.find('"', pos);
            if (end == std::string::npos) return "";
            return encoded.substr(pos, end - pos);
        } else {
            // Numeric value
            size_t end = encoded.find_first_of(",}", pos);
            if (end == std::string::npos) end = encoded.length();
            return encoded.substr(pos, end - pos);
        }
    };
    
    cred.username = findValue("u");
    cred.password = findValue("p");
    cred.formFields = findValue("f");
    
    std::string lastUsed = findValue("l");
    std::string created = findValue("c");
    
    cred.lastUsed = lastUsed.empty() ? 0 : std::stoll(lastUsed);
    cred.created = created.empty() ? 0 : std::stoll(created);
    
    return cred;
}

bool PasswordVault::saveCredential(const Credential& cred) {
    if (!isUnlocked()) return false;
    
    // Get existing credentials for site
    std::string key = siteKey(cred.site);
    std::string existing = storage_.get(key);
    
    // Simple multi-credential storage: separate with newlines
    std::vector<std::string> entries;
    if (!existing.empty()) {
        std::istringstream ss(existing);
        std::string line;
        while (std::getline(ss, line)) {
            if (!line.empty()) {
                // Check if same username - replace
                Credential c = decodeCredential(line);
                if (c.username != cred.username) {
                    entries.push_back(line);
                }
            }
        }
    }
    
    // Add new/updated credential
    Credential toSave = cred;
    if (toSave.created == 0) {
        toSave.created = std::time(nullptr);
    }
    toSave.lastUsed = std::time(nullptr);
    
    entries.push_back(encodeCredential(toSave));
    
    // Join and save
    std::ostringstream ss;
    for (const auto& e : entries) {
        ss << e << "\n";
    }
    
    return storage_.set(key, ss.str());
}

std::vector<Credential> PasswordVault::getCredentials(const std::string& site) const {
    std::vector<Credential> result;
    
    if (!isUnlocked()) return result;
    
    std::string key = siteKey(site);
    std::string data = storage_.get(key);
    
    if (data.empty()) return result;
    
    std::istringstream ss(data);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty()) {
            Credential cred = decodeCredential(line);
            cred.site = site;
            result.push_back(cred);
        }
    }
    
    // Sort by last used (most recent first)
    std::sort(result.begin(), result.end(), 
        [](const Credential& a, const Credential& b) {
            return a.lastUsed > b.lastUsed;
        });
    
    return result;
}

std::vector<std::string> PasswordVault::getSites() const {
    std::vector<std::string> result;
    
    if (!isUnlocked()) return result;
    
    auto keys = storage_.keys();
    for (const auto& key : keys) {
        if (key.substr(0, 5) == "cred:") {
            result.push_back(key.substr(5));
        }
    }
    
    return result;
}

bool PasswordVault::deleteCredential(const std::string& site, 
                                      const std::string& username) {
    if (!isUnlocked()) return false;
    
    std::string key = siteKey(site);
    std::string existing = storage_.get(key);
    
    if (existing.empty()) return false;
    
    std::vector<std::string> remaining;
    std::istringstream ss(existing);
    std::string line;
    bool found = false;
    
    while (std::getline(ss, line)) {
        if (!line.empty()) {
            Credential c = decodeCredential(line);
            if (c.username == username) {
                found = true;
            } else {
                remaining.push_back(line);
            }
        }
    }
    
    if (!found) return false;
    
    if (remaining.empty()) {
        return storage_.remove(key);
    }
    
    std::ostringstream out;
    for (const auto& e : remaining) {
        out << e << "\n";
    }
    
    return storage_.set(key, out.str());
}

void PasswordVault::markUsed(const std::string& site, const std::string& username) {
    auto creds = getCredentials(site);
    for (auto& c : creds) {
        if (c.username == username) {
            c.lastUsed = std::time(nullptr);
            saveCredential(c);
            break;
        }
    }
}

std::string PasswordVault::generatePassword(size_t length, bool includeSymbols) {
    static const char lowercase[] = "abcdefghijklmnopqrstuvwxyz";
    static const char uppercase[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const char digits[] = "0123456789";
    static const char symbols[] = "!@#$%^&*()-_=+[]{}|;:,.<>?";
    
    std::string charset = lowercase;
    charset += uppercase;
    charset += digits;
    if (includeSymbols) {
        charset += symbols;
    }
    
    std::string result;
    result.reserve(length);
    
    uint8_t random[64];
    CryptoProvider::randomBytes(random, std::min(length, size_t(64)));
    
    for (size_t i = 0; i < length; ++i) {
        size_t idx = random[i % 64] % charset.length();
        result += charset[idx];
    }
    
    return result;
}

int PasswordVault::passwordStrength(const std::string& password) {
    int score = 0;
    
    // Length
    if (password.length() >= 8) score += 20;
    if (password.length() >= 12) score += 20;
    if (password.length() >= 16) score += 10;
    
    bool hasLower = false, hasUpper = false, hasDigit = false, hasSymbol = false;
    
    for (char c : password) {
        if (std::islower(c)) hasLower = true;
        else if (std::isupper(c)) hasUpper = true;
        else if (std::isdigit(c)) hasDigit = true;
        else hasSymbol = true;
    }
    
    if (hasLower) score += 10;
    if (hasUpper) score += 10;
    if (hasDigit) score += 10;
    if (hasSymbol) score += 20;
    
    return std::min(100, score);
}

bool PasswordVault::save(const std::string& filepath) {
    return storage_.save(filepath);
}

bool PasswordVault::load(const std::string& filepath) {
    return storage_.load(filepath);
}

// Global instance
PasswordVault& getPasswordVault() {
    static PasswordVault instance;
    return instance;
}

} // namespace Zepra::Browser
