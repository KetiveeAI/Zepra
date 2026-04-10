// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#pragma once

#include "nxgfx/primitives.h"
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace NXRender {

// ==================================================================
// Container types
// ==================================================================

enum class ContainerType : uint8_t {
    None,       // No containment
    Size,       // Both inline and block size containment
    InlineSize, // Inline-size containment only
    Normal,     // Style containment (default for container-name)
};

// ==================================================================
// Container query condition types
// ==================================================================

enum class QueryFeature : uint8_t {
    Width,
    Height,
    InlineSize,
    BlockSize,
    AspectRatio,
    Orientation,
    // Style queries
    Style,
};

enum class QueryComparator : uint8_t {
    Eq,     // = or :
    Lt,     // <
    Gt,     // >
    LtEq,   // <=
    GtEq,   // >=
};

enum class QueryLogical : uint8_t {
    None,
    And,
    Or,
    Not,
};

// ==================================================================
// Container query condition
// ==================================================================

struct QueryCondition {
    QueryFeature feature = QueryFeature::Width;
    QueryComparator comparator = QueryComparator::GtEq;
    float value = 0;
    std::string unit;
    std::string styleProperty;
    std::string styleValue;

    bool evaluate(float containerWidth, float containerHeight,
                  float containerInline, float containerBlock) const;
};

struct QueryExpression {
    QueryLogical logical = QueryLogical::None;
    std::vector<QueryCondition> conditions;
    std::vector<QueryExpression> subExpressions;

    bool evaluate(float containerWidth, float containerHeight,
                  float containerInline, float containerBlock) const;
};

// ==================================================================
// Container definition
// ==================================================================

struct ContainerDefinition {
    std::string name;
    ContainerType type = ContainerType::None;
    float width = 0;
    float height = 0;
    float inlineSize = 0;
    float blockSize = 0;
    bool writingModeVertical = false;
};

// ==================================================================
// Container query rule
// ==================================================================

struct ContainerQueryRule {
    std::string containerName;   // Empty = unnamed (closest container)
    QueryExpression expression;
    std::vector<int> ruleIndices; // CSS rules to apply when query matches
};

// ==================================================================
// Container query evaluator
// ==================================================================

class ContainerQueryEvaluator {
public:
    void registerContainer(const std::string& name, const ContainerDefinition& def);
    void unregisterContainer(const std::string& name);
    void updateContainerSize(const std::string& name, float width, float height);

    bool evaluate(const ContainerQueryRule& query) const;

    // Find the closest container with the given name (or type)
    const ContainerDefinition* findContainer(const std::string& name) const;
    const ContainerDefinition* findClosestSizeContainer() const;

    // Evaluate all registered queries and return which rules to apply
    std::vector<int> activeRuleIndices(const std::vector<ContainerQueryRule>& queries) const;

    // Invalidation
    using InvalidationCallback = std::function<void()>;
    void onContainerResize(InvalidationCallback cb) { onResize_ = cb; }

    // Parse container query string
    static ContainerQueryRule parse(const std::string& queryStr);
    static QueryExpression parseExpression(const std::string& exprStr);

private:
    std::vector<ContainerDefinition> containers_;
    InvalidationCallback onResize_;

    bool evaluateCondition(const QueryCondition& cond,
                           const ContainerDefinition& container) const;
};

// ==================================================================
// Container size observer
// ==================================================================

class ContainerSizeObserver {
public:
    struct Entry {
        std::string containerName;
        float prevWidth = 0, prevHeight = 0;
        float width = 0, height = 0;
        bool changed = false;
    };

    void observe(const std::string& name, float width, float height);
    void update(const std::string& name, float width, float height);
    bool hasChanges() const;
    std::vector<Entry> changedEntries() const;
    void clearChanges();

private:
    std::vector<Entry> entries_;
};

} // namespace NXRender
