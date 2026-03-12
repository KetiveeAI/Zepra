/**
 * @file html_autofill.hpp
 * @brief Autofill and autocomplete support
 */

#pragma once

#include <string>
#include <vector>
#include <functional>

namespace Zepra::WebCore {

/**
 * @brief Autofill token types
 */
enum class AutofillTokenType {
    Section,
    AddressType,
    FieldName
};

/**
 * @brief Autofill hint
 */
struct AutofillHint {
    std::string section;  // e.g., "section-payment"
    std::string addressType;  // "shipping", "billing"
    std::string fieldName;  // "name", "email", "tel", etc.
};

/**
 * @brief Parse autocomplete attribute
 */
class AutofillParser {
public:
    static AutofillHint parse(const std::string& autocomplete);
    static std::string serialize(const AutofillHint& hint);
    
    static bool isValidFieldName(const std::string& name);
    static std::vector<std::string> getValidFieldNames();
};

/**
 * @brief Autofill field names per spec
 */
enum class AutofillFieldName {
    // Personal
    Name,
    HonorificPrefix,
    GivenName,
    AdditionalName,
    FamilyName,
    HonorificSuffix,
    Nickname,
    
    // Username/password
    Username,
    NewPassword,
    CurrentPassword,
    OneTimeCode,
    
    // Organization
    Organization,
    OrganizationTitle,
    
    // Address
    StreetAddress,
    AddressLine1,
    AddressLine2,
    AddressLine3,
    AddressLevel1,
    AddressLevel2,
    AddressLevel3,
    AddressLevel4,
    Country,
    CountryName,
    PostalCode,
    
    // Contact
    Email,
    Tel,
    TelCountryCode,
    TelNational,
    TelAreaCode,
    TelLocal,
    TelLocalPrefix,
    TelLocalSuffix,
    TelExtension,
    
    // Payment
    CcName,
    CcGivenName,
    CcAdditionalName,
    CcFamilyName,
    CcNumber,
    CcExp,
    CcExpMonth,
    CcExpYear,
    CcCsc,
    CcType,
    
    // Dates
    Bday,
    BdayDay,
    BdayMonth,
    BdayYear,
    
    // Other
    Sex,
    Url,
    Photo,
    Impp,
    Language
};

/**
 * @brief Autofill suggestion
 */
struct AutofillSuggestion {
    std::string label;
    std::string value;
    std::string sublabel;
    AutofillFieldName fieldType;
};

/**
 * @brief Autofill manager interface
 */
class AutofillManager {
public:
    virtual ~AutofillManager() = default;
    
    virtual std::vector<AutofillSuggestion> getSuggestions(
        const AutofillHint& hint,
        const std::string& currentValue) = 0;
    
    virtual void saveSuggestion(const AutofillHint& hint,
                                const std::string& value) = 0;
};

} // namespace Zepra::WebCore
