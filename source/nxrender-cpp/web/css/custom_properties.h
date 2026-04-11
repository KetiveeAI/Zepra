// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace NXRender {
namespace Web {

// ==================================================================
// CSS Custom Properties (CSS Variables) — Level 1
// ==================================================================

struct CSSCustomProperty {
    std::string name;        // "--my-color"
    std::string value;       // "blue" or "var(--other, fallback)"
    bool inherited = true;
    bool initial = false;    // @property initial-value
    std::string syntax;      // @property syntax: "<color>" etc.
};

// ==================================================================
// Registered custom property (@property rule)
// ==================================================================

struct RegisteredProperty {
    std::string name;
    std::string syntax = "*";
    bool inherits = true;
    std::string initialValue;
};

// ==================================================================
// CSS Variable scope — stores custom properties for a single element
// ==================================================================

class CSSVariableScope {
public:
    void set(const std::string& name, const std::string& value);
    const std::string* get(const std::string& name) const;
    bool has(const std::string& name) const;
    void remove(const std::string& name);
    void clear();

    // Merge with parent scope (inheritance)
    void inheritFrom(const CSSVariableScope& parent);

    // Get all defined properties
    const std::unordered_map<std::string, std::string>& properties() const { return props_; }

    // Count
    size_t size() const { return props_.size(); }

private:
    std::unordered_map<std::string, std::string> props_;
};

// ==================================================================
// CSS Variable resolver — handles var() function
// ==================================================================

class CSSVariableResolver {
public:
    CSSVariableResolver();
    ~CSSVariableResolver();

    // Register @property rule
    void registerProperty(const RegisteredProperty& prop);
    const RegisteredProperty* findRegistered(const std::string& name) const;

    // Resolve var() references in a value string
    // Returns resolved value, or fallback/initial if undefined
    std::string resolve(const std::string& value, const CSSVariableScope& scope) const;

    // Check if a value contains var() references
    static bool containsVar(const std::string& value);

    // Extract variable name and fallback from "var(--name, fallback)"
    struct VarReference {
        std::string name;
        std::string fallback;
    };
    static std::vector<VarReference> extractVarReferences(const std::string& value);

    // Check for circular references
    bool hasCircularReference(const std::string& name, const CSSVariableScope& scope) const;

    // Validate value against @property syntax
    bool validateValue(const std::string& name, const std::string& value) const;

    // Animation interpolation for registered properties
    std::string interpolate(const std::string& name,
                             const std::string& from,
                             const std::string& to,
                             float progress) const;

    // Get all registered properties
    const std::unordered_map<std::string, RegisteredProperty>& registeredProperties() const {
        return registered_;
    }

private:
    std::unordered_map<std::string, RegisteredProperty> registered_;

    std::string resolveVar(const std::string& name,
                            const std::string& fallback,
                            const CSSVariableScope& scope,
                            int depth) const;

    static constexpr int MAX_VAR_DEPTH = 16;
};

// ==================================================================
// CSS Environment variables (env())
// ==================================================================

class CSSEnvironment {
public:
    static CSSEnvironment& instance();

    void set(const std::string& name, const std::string& value);
    const std::string* get(const std::string& name) const;

    // Standard env() values
    void setSafeAreaInsets(float top, float right, float bottom, float left);
    void setViewportSegments(int count, const float* widths, const float* heights);

    // Resolve env() in value string
    std::string resolve(const std::string& value) const;

    static bool containsEnv(const std::string& value);

private:
    CSSEnvironment();
    std::unordered_map<std::string, std::string> env_;
};

// ==================================================================
// CSS @supports query
// ==================================================================

class CSSSupports {
public:
    static bool supports(const std::string& property, const std::string& value);
    static bool supportsSelector(const std::string& selector);
    static bool supportsCondition(const std::string& condition);

    // Register support for a property
    static void registerProperty(const std::string& property);
    static void registerFunction(const std::string& function);

private:
    static std::vector<std::string>& supportedProperties();
    static std::vector<std::string>& supportedFunctions();
};

// ==================================================================
// CSS @layer
// ==================================================================

class CSSLayerOrder {
public:
    void declareLayer(const std::string& name);
    void declareLayerOrder(const std::vector<std::string>& names);

    int priority(const std::string& layerName) const;
    bool exists(const std::string& layerName) const;

    // Anonymous layers
    std::string createAnonymousLayer();

    const std::vector<std::string>& layers() const { return layers_; }

private:
    std::vector<std::string> layers_;
    int nextAnon_ = 0;
};

// ==================================================================
// CSS Nesting context
// ==================================================================

class CSSNestingContext {
public:
    struct NestingRule {
        std::string selector;
        std::vector<std::pair<std::string, std::string>> declarations;
        std::vector<NestingRule> nestedRules;
        bool isAtRule = false;
        std::string atRulePrefix; // "@media", "@container", etc.
    };

    // Flatten nested rules into a list of selector + declarations
    struct FlatRule {
        std::string selector;
        std::vector<std::pair<std::string, std::string>> declarations;
    };

    static std::vector<FlatRule> flatten(const NestingRule& rule,
                                          const std::string& parentSelector = "");

    // Combine parent & nested selectors per CSS Nesting spec
    static std::string combineSelectors(const std::string& parent,
                                          const std::string& nested);
};

} // namespace Web
} // namespace NXRender
