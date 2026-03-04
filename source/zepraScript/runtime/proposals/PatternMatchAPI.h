/**
 * @file PatternMatchAPI.h
 * @brief Pattern Matching Simulation (Stage 1 Proposal)
 */

#pragma once

#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <optional>
#include <any>
#include <typeinfo>
#include <stdexcept>

namespace Zepra::Runtime {

// =============================================================================
// Pattern Types
// =============================================================================

class Pattern {
public:
    virtual ~Pattern() = default;
    virtual bool matches(const std::any& value) const = 0;
};

class LiteralPattern : public Pattern {
public:
    template<typename T>
    explicit LiteralPattern(T value) : expected_(std::move(value)) {}
    
    bool matches(const std::any& value) const override {
        return value.type() == expected_.type();
    }

private:
    std::any expected_;
};

class TypePattern : public Pattern {
public:
    explicit TypePattern(const std::type_info& type) : type_(&type) {}
    
    bool matches(const std::any& value) const override {
        return value.type() == *type_;
    }

private:
    const std::type_info* type_;
};

class WildcardPattern : public Pattern {
public:
    bool matches(const std::any&) const override { return true; }
};

class GuardPattern : public Pattern {
public:
    using Guard = std::function<bool(const std::any&)>;
    
    GuardPattern(std::shared_ptr<Pattern> pattern, Guard guard)
        : pattern_(std::move(pattern)), guard_(std::move(guard)) {}
    
    bool matches(const std::any& value) const override {
        return pattern_->matches(value) && guard_(value);
    }

private:
    std::shared_ptr<Pattern> pattern_;
    Guard guard_;
};

class RangePattern : public Pattern {
public:
    RangePattern(double min, double max) : min_(min), max_(max) {}
    
    bool matches(const std::any& value) const override {
        try {
            double v = std::any_cast<double>(value);
            return v >= min_ && v <= max_;
        } catch (...) {
            return false;
        }
    }

private:
    double min_, max_;
};

// =============================================================================
// Match Case
// =============================================================================

template<typename R>
class MatchCase {
public:
    using Handler = std::function<R(const std::any&)>;
    
    MatchCase(std::shared_ptr<Pattern> pattern, Handler handler)
        : pattern_(std::move(pattern)), handler_(std::move(handler)) {}
    
    bool tryMatch(const std::any& value, R& result) const {
        if (pattern_->matches(value)) {
            result = handler_(value);
            return true;
        }
        return false;
    }

private:
    std::shared_ptr<Pattern> pattern_;
    Handler handler_;
};

// =============================================================================
// Match Expression
// =============================================================================

template<typename R>
class Match {
public:
    explicit Match(std::any value) : value_(std::move(value)) {}
    
    Match& when(std::shared_ptr<Pattern> pattern, std::function<R(const std::any&)> handler) {
        cases_.emplace_back(std::move(pattern), std::move(handler));
        return *this;
    }
    
    template<typename T>
    Match& whenType(std::function<R(const T&)> handler) {
        auto pattern = std::make_shared<TypePattern>(typeid(T));
        cases_.emplace_back(pattern, [handler](const std::any& v) {
            return handler(std::any_cast<const T&>(v));
        });
        return *this;
    }
    
    template<typename T>
    Match& whenValue(T expected, std::function<R()> handler) {
        auto pattern = std::make_shared<LiteralPattern>(expected);
        cases_.emplace_back(pattern, [handler](const std::any&) { return handler(); });
        return *this;
    }
    
    Match& otherwise(std::function<R()> handler) {
        defaultHandler_ = handler;
        return *this;
    }
    
    R execute() const {
        for (const auto& c : cases_) {
            R result;
            if (c.tryMatch(value_, result)) {
                return result;
            }
        }
        if (defaultHandler_) {
            return defaultHandler_();
        }
        throw std::runtime_error("No pattern matched");
    }
    
    operator R() const { return execute(); }

private:
    std::any value_;
    std::vector<MatchCase<R>> cases_;
    std::function<R()> defaultHandler_;
};

// =============================================================================
// Factory Functions
// =============================================================================

template<typename R>
Match<R> match(std::any value) {
    return Match<R>(std::move(value));
}

inline std::shared_ptr<Pattern> literal(std::any value) {
    return std::make_shared<LiteralPattern>(std::move(value));
}

inline std::shared_ptr<Pattern> wildcard() {
    return std::make_shared<WildcardPattern>();
}

inline std::shared_ptr<Pattern> range(double min, double max) {
    return std::make_shared<RangePattern>(min, max);
}

template<typename T>
std::shared_ptr<Pattern> type() {
    return std::make_shared<TypePattern>(typeid(T));
}

inline std::shared_ptr<Pattern> when(std::shared_ptr<Pattern> p, std::function<bool(const std::any&)> guard) {
    return std::make_shared<GuardPattern>(std::move(p), std::move(guard));
}

} // namespace Zepra::Runtime
