// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "custom_properties.h"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cstdlib>
#include <cmath>

namespace NXRender {
namespace Web {

// ==================================================================
// CSSVariableScope
// ==================================================================

void CSSVariableScope::set(const std::string& name, const std::string& value) {
    props_[name] = value;
}

const std::string* CSSVariableScope::get(const std::string& name) const {
    auto it = props_.find(name);
    return (it != props_.end()) ? &it->second : nullptr;
}

bool CSSVariableScope::has(const std::string& name) const {
    return props_.find(name) != props_.end();
}

void CSSVariableScope::remove(const std::string& name) {
    props_.erase(name);
}

void CSSVariableScope::clear() {
    props_.clear();
}

void CSSVariableScope::inheritFrom(const CSSVariableScope& parent) {
    for (const auto& [name, value] : parent.props_) {
        if (props_.find(name) == props_.end()) {
            props_[name] = value;
        }
    }
}

// ==================================================================
// CSSVariableResolver
// ==================================================================

CSSVariableResolver::CSSVariableResolver() {}
CSSVariableResolver::~CSSVariableResolver() {}

void CSSVariableResolver::registerProperty(const RegisteredProperty& prop) {
    registered_[prop.name] = prop;
}

const RegisteredProperty* CSSVariableResolver::findRegistered(const std::string& name) const {
    auto it = registered_.find(name);
    return (it != registered_.end()) ? &it->second : nullptr;
}

bool CSSVariableResolver::containsVar(const std::string& value) {
    return value.find("var(") != std::string::npos;
}

std::vector<CSSVariableResolver::VarReference>
CSSVariableResolver::extractVarReferences(const std::string& value) {
    std::vector<VarReference> refs;
    size_t pos = 0;

    while ((pos = value.find("var(", pos)) != std::string::npos) {
        pos += 4; // skip "var("
        int depth = 1;
        size_t start = pos;

        while (pos < value.size() && depth > 0) {
            if (value[pos] == '(') depth++;
            else if (value[pos] == ')') depth--;
            if (depth > 0) pos++;
        }

        std::string inner = value.substr(start, pos - start);

        VarReference ref;
        size_t comma = std::string::npos;
        int parenDepth = 0;
        for (size_t i = 0; i < inner.size(); i++) {
            if (inner[i] == '(') parenDepth++;
            else if (inner[i] == ')') parenDepth--;
            else if (inner[i] == ',' && parenDepth == 0) {
                comma = i;
                break;
            }
        }

        if (comma != std::string::npos) {
            ref.name = inner.substr(0, comma);
            ref.fallback = inner.substr(comma + 1);
            // Trim
            while (!ref.name.empty() && std::isspace(ref.name.back())) ref.name.pop_back();
            while (!ref.fallback.empty() && std::isspace(ref.fallback.front()))
                ref.fallback.erase(ref.fallback.begin());
        } else {
            ref.name = inner;
            while (!ref.name.empty() && std::isspace(ref.name.back())) ref.name.pop_back();
        }
        while (!ref.name.empty() && std::isspace(ref.name.front()))
            ref.name.erase(ref.name.begin());

        refs.push_back(ref);
        pos++;
    }

    return refs;
}

std::string CSSVariableResolver::resolve(const std::string& value,
                                           const CSSVariableScope& scope) const {
    if (!containsVar(value)) return value;

    std::string result = value;
    int iterations = 0;

    while (containsVar(result) && iterations < MAX_VAR_DEPTH) {
        auto refs = extractVarReferences(result);
        if (refs.empty()) break;

        for (const auto& ref : refs) {
            std::string resolved = resolveVar(ref.name, ref.fallback, scope, 0);

            // Build the var(...) string to replace
            std::string varExpr = "var(" + ref.name;
            if (!ref.fallback.empty()) varExpr += ", " + ref.fallback;
            varExpr += ")";

            size_t pos = result.find(varExpr);
            if (pos != std::string::npos) {
                result.replace(pos, varExpr.size(), resolved);
            }
        }
        iterations++;
    }

    return result;
}

std::string CSSVariableResolver::resolveVar(const std::string& name,
                                               const std::string& fallback,
                                               const CSSVariableScope& scope,
                                               int depth) const {
    if (depth >= MAX_VAR_DEPTH) return fallback;

    const std::string* value = scope.get(name);
    if (value) {
        if (containsVar(*value)) {
            return resolve(*value, scope);
        }
        return *value;
    }

    // Check registered property initial value
    auto* reg = findRegistered(name);
    if (reg && !reg->initialValue.empty()) {
        return reg->initialValue;
    }

    // Use fallback
    if (!fallback.empty()) {
        if (containsVar(fallback)) {
            return resolve(fallback, scope);
        }
        return fallback;
    }

    return "";
}

bool CSSVariableResolver::hasCircularReference(const std::string& name,
                                                  const CSSVariableScope& scope) const {
    std::vector<std::string> visited;
    std::string current = name;

    for (int i = 0; i < MAX_VAR_DEPTH; i++) {
        if (std::find(visited.begin(), visited.end(), current) != visited.end()) {
            return true;
        }
        visited.push_back(current);

        const std::string* value = scope.get(current);
        if (!value) return false;

        auto refs = extractVarReferences(*value);
        if (refs.empty()) return false;

        current = refs[0].name;
    }

    return true;
}

bool CSSVariableResolver::validateValue(const std::string& name,
                                          const std::string& value) const {
    auto* reg = findRegistered(name);
    if (!reg) return true; // Unregistered = any value valid

    const std::string& syntax = reg->syntax;
    if (syntax == "*") return true;

    if (syntax == "<color>") {
        // Basic validation: hex, rgb(), named colors
        if (value[0] == '#') return value.size() == 4 || value.size() == 7 || value.size() == 9;
        if (value.find("rgb") == 0 || value.find("hsl") == 0) return true;
        return true; // Accept named colors
    }

    if (syntax == "<length>") {
        char* end;
        std::strtof(value.c_str(), &end);
        if (end == value.c_str()) return false;
        std::string unit(end);
        return unit == "px" || unit == "em" || unit == "rem" ||
               unit == "%" || unit == "vw" || unit == "vh" || unit.empty();
    }

    if (syntax == "<number>") {
        char* end;
        std::strtof(value.c_str(), &end);
        return end != value.c_str();
    }

    if (syntax == "<integer>") {
        char* end;
        std::strtol(value.c_str(), &end, 10);
        return end != value.c_str();
    }

    if (syntax == "<percentage>") {
        char* end;
        std::strtof(value.c_str(), &end);
        return end != value.c_str() && std::string(end) == "%";
    }

    return true;
}

std::string CSSVariableResolver::interpolate(const std::string& name,
                                                const std::string& from,
                                                const std::string& to,
                                                float progress) const {
    auto* reg = findRegistered(name);
    if (!reg) {
        // Unregistered: discrete at 50%
        return (progress < 0.5f) ? from : to;
    }

    const std::string& syntax = reg->syntax;

    if (syntax == "<number>" || syntax == "<integer>" || syntax == "<length>" || syntax == "<percentage>") {
        float fromVal = std::strtof(from.c_str(), nullptr);
        float toVal = std::strtof(to.c_str(), nullptr);
        float result = fromVal + (toVal - fromVal) * progress;

        // Preserve unit
        std::string unit;
        for (size_t i = 0; i < to.size(); i++) {
            if (!std::isdigit(to[i]) && to[i] != '.' && to[i] != '-') {
                unit = to.substr(i);
                break;
            }
        }

        if (syntax == "<integer>") {
            return std::to_string(static_cast<int>(std::round(result))) + unit;
        }
        return std::to_string(result) + unit;
    }

    if (syntax == "<color>") {
        // Would need color interpolation — for now, discrete at 50%
        return (progress < 0.5f) ? from : to;
    }

    return (progress < 0.5f) ? from : to;
}

// ==================================================================
// CSSEnvironment
// ==================================================================

CSSEnvironment::CSSEnvironment() {
    env_["safe-area-inset-top"] = "0px";
    env_["safe-area-inset-right"] = "0px";
    env_["safe-area-inset-bottom"] = "0px";
    env_["safe-area-inset-left"] = "0px";
}

CSSEnvironment& CSSEnvironment::instance() {
    static CSSEnvironment inst;
    return inst;
}

void CSSEnvironment::set(const std::string& name, const std::string& value) {
    env_[name] = value;
}

const std::string* CSSEnvironment::get(const std::string& name) const {
    auto it = env_.find(name);
    return (it != env_.end()) ? &it->second : nullptr;
}

void CSSEnvironment::setSafeAreaInsets(float top, float right, float bottom, float left) {
    auto px = [](float v) { return std::to_string(v) + "px"; };
    env_["safe-area-inset-top"] = px(top);
    env_["safe-area-inset-right"] = px(right);
    env_["safe-area-inset-bottom"] = px(bottom);
    env_["safe-area-inset-left"] = px(left);
}

void CSSEnvironment::setViewportSegments(int count, const float* widths, const float* heights) {
    env_["viewport-segment-count"] = std::to_string(count);
    for (int i = 0; i < count; i++) {
        std::string prefix = "viewport-segment-" + std::to_string(i);
        env_[prefix + "-width"] = std::to_string(widths[i]) + "px";
        env_[prefix + "-height"] = std::to_string(heights[i]) + "px";
    }
}

bool CSSEnvironment::containsEnv(const std::string& value) {
    return value.find("env(") != std::string::npos;
}

std::string CSSEnvironment::resolve(const std::string& value) const {
    if (!containsEnv(value)) return value;

    std::string result = value;
    size_t pos = 0;

    while ((pos = result.find("env(", pos)) != std::string::npos) {
        size_t start = pos + 4;
        int depth = 1;
        size_t end = start;

        while (end < result.size() && depth > 0) {
            if (result[end] == '(') depth++;
            else if (result[end] == ')') depth--;
            if (depth > 0) end++;
        }

        std::string inner = result.substr(start, end - start);
        // Trim
        while (!inner.empty() && std::isspace(inner.front())) inner.erase(inner.begin());
        while (!inner.empty() && std::isspace(inner.back())) inner.pop_back();

        // Check for fallback
        std::string envName = inner, fallback;
        size_t comma = inner.find(',');
        if (comma != std::string::npos) {
            envName = inner.substr(0, comma);
            fallback = inner.substr(comma + 1);
            while (!envName.empty() && std::isspace(envName.back())) envName.pop_back();
            while (!fallback.empty() && std::isspace(fallback.front()))
                fallback.erase(fallback.begin());
        }

        const std::string* envVal = get(envName);
        std::string resolved = envVal ? *envVal : fallback;

        result.replace(pos, end - pos + 1, resolved);
    }

    return result;
}

// ==================================================================
// CSSSupports
// ==================================================================

std::vector<std::string>& CSSSupports::supportedProperties() {
    static std::vector<std::string> props = {
        "display", "position", "float", "clear",
        "width", "height", "min-width", "max-width", "min-height", "max-height",
        "margin", "margin-top", "margin-right", "margin-bottom", "margin-left",
        "padding", "padding-top", "padding-right", "padding-bottom", "padding-left",
        "border", "border-radius", "border-width", "border-color", "border-style",
        "color", "background", "background-color", "background-image",
        "font-family", "font-size", "font-weight", "font-style", "line-height",
        "text-align", "text-decoration", "text-transform", "white-space",
        "overflow", "overflow-x", "overflow-y",
        "opacity", "visibility", "z-index",
        "flex", "flex-direction", "flex-wrap", "justify-content", "align-items", "align-self",
        "grid", "grid-template-columns", "grid-template-rows", "gap",
        "transform", "transition", "animation",
        "box-shadow", "text-shadow", "outline",
        "cursor", "pointer-events", "user-select",
        "container-type", "container-name",
        "aspect-ratio", "object-fit", "object-position",
        "scroll-snap-type", "scroll-snap-align",
        "accent-color", "color-scheme",
    };
    return props;
}

std::vector<std::string>& CSSSupports::supportedFunctions() {
    static std::vector<std::string> fns = {
        "var", "env", "calc", "min", "max", "clamp",
        "rgb", "rgba", "hsl", "hsla", "hwb", "oklch", "oklab",
        "linear-gradient", "radial-gradient", "conic-gradient",
        "url", "counter", "counters", "attr",
    };
    return fns;
}

bool CSSSupports::supports(const std::string& property, const std::string& /*value*/) {
    auto& props = supportedProperties();
    return std::find(props.begin(), props.end(), property) != props.end();
}

bool CSSSupports::supportsSelector(const std::string& selector) {
    // Basic selector validation
    return !selector.empty() && selector.find("???") == std::string::npos;
}

bool CSSSupports::supportsCondition(const std::string& condition) {
    // Parse "(property: value)" format
    size_t paren = condition.find('(');
    size_t colon = condition.find(':');
    if (paren == std::string::npos || colon == std::string::npos) return false;

    std::string prop = condition.substr(paren + 1, colon - paren - 1);
    while (!prop.empty() && std::isspace(prop.front())) prop.erase(prop.begin());
    while (!prop.empty() && std::isspace(prop.back())) prop.pop_back();

    return supports(prop, "");
}

void CSSSupports::registerProperty(const std::string& property) {
    auto& props = supportedProperties();
    if (std::find(props.begin(), props.end(), property) == props.end()) {
        props.push_back(property);
    }
}

void CSSSupports::registerFunction(const std::string& function) {
    auto& fns = supportedFunctions();
    if (std::find(fns.begin(), fns.end(), function) == fns.end()) {
        fns.push_back(function);
    }
}

// ==================================================================
// CSSLayerOrder
// ==================================================================

void CSSLayerOrder::declareLayer(const std::string& name) {
    if (!exists(name)) {
        layers_.push_back(name);
    }
}

void CSSLayerOrder::declareLayerOrder(const std::vector<std::string>& names) {
    for (const auto& name : names) {
        declareLayer(name);
    }
}

int CSSLayerOrder::priority(const std::string& layerName) const {
    for (size_t i = 0; i < layers_.size(); i++) {
        if (layers_[i] == layerName) return static_cast<int>(i);
    }
    return -1;
}

bool CSSLayerOrder::exists(const std::string& layerName) const {
    return std::find(layers_.begin(), layers_.end(), layerName) != layers_.end();
}

std::string CSSLayerOrder::createAnonymousLayer() {
    std::string name = "__anon_layer_" + std::to_string(nextAnon_++);
    layers_.push_back(name);
    return name;
}

// ==================================================================
// CSSNestingContext
// ==================================================================

std::vector<CSSNestingContext::FlatRule>
CSSNestingContext::flatten(const NestingRule& rule, const std::string& parentSelector) {
    std::vector<FlatRule> result;

    std::string selector = rule.selector;
    if (!parentSelector.empty()) {
        selector = combineSelectors(parentSelector, rule.selector);
    }

    if (!rule.declarations.empty()) {
        result.push_back({selector, rule.declarations});
    }

    for (const auto& nested : rule.nestedRules) {
        auto children = flatten(nested, selector);
        result.insert(result.end(), children.begin(), children.end());
    }

    return result;
}

std::string CSSNestingContext::combineSelectors(const std::string& parent,
                                                  const std::string& nested) {
    // If nested contains '&', replace it with parent
    size_t ampPos = nested.find('&');
    if (ampPos != std::string::npos) {
        std::string result = nested;
        while ((ampPos = result.find('&')) != std::string::npos) {
            result.replace(ampPos, 1, parent);
        }
        return result;
    }

    // Otherwise: parent followed by space followed by nested
    // Unless nested starts with a combinator
    if (!nested.empty() && (nested[0] == '>' || nested[0] == '+' || nested[0] == '~')) {
        return parent + " " + nested;
    }

    return parent + " " + nested;
}

} // namespace Web
} // namespace NXRender
