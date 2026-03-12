/**
 * @file html_validation.hpp
 * @brief HTML Form Validation interfaces
 */

#pragma once

#include "html/html_element.hpp"
#include <string>
#include <vector>

namespace Zepra::WebCore {

/**
 * @brief Validity state for form controls
 */
class ValidityState {
public:
    ValidityState() = default;
    
    // States
    bool valueMissing() const { return valueMissing_; }
    bool typeMismatch() const { return typeMismatch_; }
    bool patternMismatch() const { return patternMismatch_; }
    bool tooLong() const { return tooLong_; }
    bool tooShort() const { return tooShort_; }
    bool rangeUnderflow() const { return rangeUnderflow_; }
    bool rangeOverflow() const { return rangeOverflow_; }
    bool stepMismatch() const { return stepMismatch_; }
    bool badInput() const { return badInput_; }
    bool customError() const { return customError_; }
    
    bool valid() const {
        return !valueMissing_ && !typeMismatch_ && !patternMismatch_ &&
               !tooLong_ && !tooShort_ && !rangeUnderflow_ && !rangeOverflow_ &&
               !stepMismatch_ && !badInput_ && !customError_;
    }
    
    // Setters
    void setValueMissing(bool v) { valueMissing_ = v; }
    void setTypeMismatch(bool v) { typeMismatch_ = v; }
    void setPatternMismatch(bool v) { patternMismatch_ = v; }
    void setTooLong(bool v) { tooLong_ = v; }
    void setTooShort(bool v) { tooShort_ = v; }
    void setRangeUnderflow(bool v) { rangeUnderflow_ = v; }
    void setRangeOverflow(bool v) { rangeOverflow_ = v; }
    void setStepMismatch(bool v) { stepMismatch_ = v; }
    void setBadInput(bool v) { badInput_ = v; }
    void setCustomError(bool v) { customError_ = v; }
    
    void reset() {
        valueMissing_ = typeMismatch_ = patternMismatch_ = false;
        tooLong_ = tooShort_ = rangeUnderflow_ = rangeOverflow_ = false;
        stepMismatch_ = badInput_ = customError_ = false;
    }
    
private:
    bool valueMissing_ = false;
    bool typeMismatch_ = false;
    bool patternMismatch_ = false;
    bool tooLong_ = false;
    bool tooShort_ = false;
    bool rangeUnderflow_ = false;
    bool rangeOverflow_ = false;
    bool stepMismatch_ = false;
    bool badInput_ = false;
    bool customError_ = false;
};

/**
 * @brief Constraint validation mixin
 */
class ConstraintValidation {
public:
    virtual ~ConstraintValidation() = default;
    
    // Validation state
    const ValidityState& validity() const { return validity_; }
    
    bool willValidate() const { return willValidate_; }
    
    std::string validationMessage() const { return validationMessage_; }
    
    // Methods
    virtual bool checkValidity();
    virtual bool reportValidity();
    virtual void setCustomValidity(const std::string& message);
    
protected:
    virtual void updateValidity() = 0;
    
    ValidityState validity_;
    std::string validationMessage_;
    bool willValidate_ = true;
};

/**
 * @brief Form data interface
 */
class FormData {
public:
    using Entry = std::pair<std::string, std::string>;
    
    FormData() = default;
    explicit FormData(HTMLElement* form);
    
    // Entry management
    void append(const std::string& name, const std::string& value);
    void delete_(const std::string& name);
    std::string get(const std::string& name) const;
    std::vector<std::string> getAll(const std::string& name) const;
    bool has(const std::string& name) const;
    void set(const std::string& name, const std::string& value);
    
    // Iteration
    std::vector<Entry>::const_iterator begin() const { return entries_.begin(); }
    std::vector<Entry>::const_iterator end() const { return entries_.end(); }
    
    // Keys and values
    std::vector<std::string> keys() const;
    std::vector<std::string> values() const;
    
    // URL encoding
    std::string toURLSearchParams() const;
    
private:
    std::vector<Entry> entries_;
};

/**
 * @brief URL Search Params
 */
class URLSearchParams {
public:
    URLSearchParams() = default;
    explicit URLSearchParams(const std::string& query);
    
    void append(const std::string& name, const std::string& value);
    void delete_(const std::string& name);
    std::string get(const std::string& name) const;
    std::vector<std::string> getAll(const std::string& name) const;
    bool has(const std::string& name) const;
    void set(const std::string& name, const std::string& value);
    
    void sort();
    
    std::string toString() const;
    
private:
    std::vector<std::pair<std::string, std::string>> params_;
    
    void parse(const std::string& query);
};

} // namespace Zepra::WebCore
