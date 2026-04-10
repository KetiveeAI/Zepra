// Copyright (c) 2026 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.

#include "layout/container_query.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <cctype>

namespace NXRender {

// ==================================================================
// QueryCondition
// ==================================================================

bool QueryCondition::evaluate(float containerWidth, float containerHeight,
                               float containerInline, float containerBlock) const {
    float target = 0;

    switch (feature) {
        case QueryFeature::Width: target = containerWidth; break;
        case QueryFeature::Height: target = containerHeight; break;
        case QueryFeature::InlineSize: target = containerInline; break;
        case QueryFeature::BlockSize: target = containerBlock; break;
        case QueryFeature::AspectRatio:
            target = (containerHeight > 0) ? containerWidth / containerHeight : 0;
            break;
        case QueryFeature::Orientation:
            // value interpretation: 0 = portrait, 1 = landscape
            if (containerWidth > containerHeight) target = 1;
            else target = 0;
            break;
        case QueryFeature::Style:
            // Style queries require property lookup — evaluated externally
            return false;
    }

    switch (comparator) {
        case QueryComparator::Eq:   return std::abs(target - value) < 0.5f;
        case QueryComparator::Lt:   return target < value;
        case QueryComparator::Gt:   return target > value;
        case QueryComparator::LtEq: return target <= value;
        case QueryComparator::GtEq: return target >= value;
    }
    return false;
}

// ==================================================================
// QueryExpression
// ==================================================================

bool QueryExpression::evaluate(float containerWidth, float containerHeight,
                                float containerInline, float containerBlock) const {
    if (logical == QueryLogical::Not) {
        if (!conditions.empty()) {
            return !conditions[0].evaluate(containerWidth, containerHeight,
                                            containerInline, containerBlock);
        }
        if (!subExpressions.empty()) {
            return !subExpressions[0].evaluate(containerWidth, containerHeight,
                                                containerInline, containerBlock);
        }
        return true;
    }

    if (logical == QueryLogical::Or) {
        for (const auto& cond : conditions) {
            if (cond.evaluate(containerWidth, containerHeight, containerInline, containerBlock))
                return true;
        }
        for (const auto& sub : subExpressions) {
            if (sub.evaluate(containerWidth, containerHeight, containerInline, containerBlock))
                return true;
        }
        return false;
    }

    // And / None — all must pass
    for (const auto& cond : conditions) {
        if (!cond.evaluate(containerWidth, containerHeight, containerInline, containerBlock))
            return false;
    }
    for (const auto& sub : subExpressions) {
        if (!sub.evaluate(containerWidth, containerHeight, containerInline, containerBlock))
            return false;
    }
    return true;
}

// ==================================================================
// ContainerQueryEvaluator
// ==================================================================

void ContainerQueryEvaluator::registerContainer(const std::string& name,
                                                  const ContainerDefinition& def) {
    for (auto& c : containers_) {
        if (c.name == name) { c = def; return; }
    }
    containers_.push_back(def);
}

void ContainerQueryEvaluator::unregisterContainer(const std::string& name) {
    containers_.erase(
        std::remove_if(containers_.begin(), containers_.end(),
                        [&](const ContainerDefinition& c) { return c.name == name; }),
        containers_.end());
}

void ContainerQueryEvaluator::updateContainerSize(const std::string& name,
                                                    float width, float height) {
    for (auto& c : containers_) {
        if (c.name == name) {
            bool changed = (c.width != width || c.height != height);
            c.width = width;
            c.height = height;
            if (c.writingModeVertical) {
                c.inlineSize = height;
                c.blockSize = width;
            } else {
                c.inlineSize = width;
                c.blockSize = height;
            }
            if (changed && onResize_) onResize_();
            return;
        }
    }
}

const ContainerDefinition* ContainerQueryEvaluator::findContainer(const std::string& name) const {
    for (const auto& c : containers_) {
        if (c.name == name) return &c;
    }
    return nullptr;
}

const ContainerDefinition* ContainerQueryEvaluator::findClosestSizeContainer() const {
    // Return last registered container with size containment
    for (auto it = containers_.rbegin(); it != containers_.rend(); ++it) {
        if (it->type == ContainerType::Size || it->type == ContainerType::InlineSize) {
            return &(*it);
        }
    }
    return nullptr;
}

bool ContainerQueryEvaluator::evaluate(const ContainerQueryRule& query) const {
    const ContainerDefinition* container = nullptr;

    if (!query.containerName.empty()) {
        container = findContainer(query.containerName);
    } else {
        container = findClosestSizeContainer();
    }

    if (!container) return false;

    return query.expression.evaluate(container->width, container->height,
                                      container->inlineSize, container->blockSize);
}

bool ContainerQueryEvaluator::evaluateCondition(const QueryCondition& cond,
                                                  const ContainerDefinition& container) const {
    return cond.evaluate(container.width, container.height,
                          container.inlineSize, container.blockSize);
}

std::vector<int> ContainerQueryEvaluator::activeRuleIndices(
    const std::vector<ContainerQueryRule>& queries) const {
    std::vector<int> activeIndices;
    for (const auto& query : queries) {
        if (evaluate(query)) {
            activeIndices.insert(activeIndices.end(),
                                 query.ruleIndices.begin(), query.ruleIndices.end());
        }
    }
    return activeIndices;
}

// ==================================================================
// Parser
// ==================================================================

ContainerQueryRule ContainerQueryEvaluator::parse(const std::string& queryStr) {
    ContainerQueryRule rule;

    std::string input = queryStr;
    // Trim
    while (!input.empty() && std::isspace(input.front())) input.erase(input.begin());
    while (!input.empty() && std::isspace(input.back())) input.pop_back();

    // Check for container name: "@container name ("
    size_t parenPos = input.find('(');
    if (parenPos != std::string::npos) {
        std::string prefix = input.substr(0, parenPos);
        // Trim whitespace from prefix
        while (!prefix.empty() && std::isspace(prefix.back())) prefix.pop_back();

        // If prefix is not empty and not a keyword, it's the container name
        if (!prefix.empty() && prefix != "not") {
            rule.containerName = prefix;
        }

        std::string exprStr = input.substr(parenPos);
        rule.expression = parseExpression(exprStr);
    }

    return rule;
}

QueryExpression ContainerQueryEvaluator::parseExpression(const std::string& exprStr) {
    QueryExpression expr;

    std::string input = exprStr;
    // Remove outer parens
    while (!input.empty() && input.front() == '(') {
        input.erase(input.begin());
        if (!input.empty() && input.back() == ')') input.pop_back();
    }

    // Check for "not"
    if (input.substr(0, 3) == "not") {
        expr.logical = QueryLogical::Not;
        input = input.substr(3);
        while (!input.empty() && std::isspace(input.front())) input.erase(input.begin());
    }

    // Split by " and " or " or "
    auto splitPos = input.find(" and ");
    if (splitPos != std::string::npos) {
        expr.logical = (expr.logical == QueryLogical::Not) ? QueryLogical::Not : QueryLogical::And;
        // Split into conditions
        std::istringstream stream(input);
        std::string part;
        while (std::getline(stream, part, ' ')) {
            if (part == "and" || part == "or") continue;
            if (part.empty()) continue;

            // Parse individual condition
            QueryCondition cond;
            // Simple format: "min-width: 400px" or "width >= 400px"
            size_t colonPos = part.find(':');
            if (colonPos != std::string::npos) {
                std::string feature = part.substr(0, colonPos);
                std::string value = part.substr(colonPos + 1);
                while (!value.empty() && std::isspace(value.front())) value.erase(value.begin());

                if (feature.find("min-") == 0) {
                    cond.comparator = QueryComparator::GtEq;
                    feature = feature.substr(4);
                } else if (feature.find("max-") == 0) {
                    cond.comparator = QueryComparator::LtEq;
                    feature = feature.substr(4);
                }

                if (feature == "width") cond.feature = QueryFeature::Width;
                else if (feature == "height") cond.feature = QueryFeature::Height;
                else if (feature == "inline-size") cond.feature = QueryFeature::InlineSize;
                else if (feature == "block-size") cond.feature = QueryFeature::BlockSize;

                cond.value = std::strtof(value.c_str(), nullptr);
                expr.conditions.push_back(cond);
            }
        }
    } else {
        // Single condition
        QueryCondition cond;
        // Parse "min-width: 400px" or "width >= 400px"
        size_t colonPos = input.find(':');
        size_t geqPos = input.find(">=");
        size_t leqPos = input.find("<=");
        size_t gtPos = input.find('>');
        size_t ltPos = input.find('<');

        if (geqPos != std::string::npos) {
            std::string feature = input.substr(0, geqPos);
            std::string value = input.substr(geqPos + 2);
            while (!feature.empty() && std::isspace(feature.back())) feature.pop_back();
            while (!value.empty() && std::isspace(value.front())) value.erase(value.begin());
            if (feature == "width") cond.feature = QueryFeature::Width;
            else if (feature == "height") cond.feature = QueryFeature::Height;
            cond.comparator = QueryComparator::GtEq;
            cond.value = std::strtof(value.c_str(), nullptr);
        } else if (leqPos != std::string::npos) {
            std::string feature = input.substr(0, leqPos);
            std::string value = input.substr(leqPos + 2);
            while (!feature.empty() && std::isspace(feature.back())) feature.pop_back();
            while (!value.empty() && std::isspace(value.front())) value.erase(value.begin());
            if (feature == "width") cond.feature = QueryFeature::Width;
            else if (feature == "height") cond.feature = QueryFeature::Height;
            cond.comparator = QueryComparator::LtEq;
            cond.value = std::strtof(value.c_str(), nullptr);
        } else if (colonPos != std::string::npos) {
            std::string feature = input.substr(0, colonPos);
            std::string value = input.substr(colonPos + 1);
            while (!feature.empty() && std::isspace(feature.back())) feature.pop_back();
            while (!value.empty() && std::isspace(value.front())) value.erase(value.begin());

            if (feature.find("min-") == 0) {
                cond.comparator = QueryComparator::GtEq;
                feature = feature.substr(4);
            } else if (feature.find("max-") == 0) {
                cond.comparator = QueryComparator::LtEq;
                feature = feature.substr(4);
            }

            if (feature == "width") cond.feature = QueryFeature::Width;
            else if (feature == "height") cond.feature = QueryFeature::Height;
            else if (feature == "inline-size") cond.feature = QueryFeature::InlineSize;
            else if (feature == "block-size") cond.feature = QueryFeature::BlockSize;

            cond.value = std::strtof(value.c_str(), nullptr);
        }

        expr.conditions.push_back(cond);
        (void)gtPos; (void)ltPos;
    }

    return expr;
}

// ==================================================================
// ContainerSizeObserver
// ==================================================================

void ContainerSizeObserver::observe(const std::string& name, float width, float height) {
    for (auto& e : entries_) {
        if (e.containerName == name) {
            e.width = width;
            e.height = height;
            return;
        }
    }
    entries_.push_back({name, width, height, width, height, false});
}

void ContainerSizeObserver::update(const std::string& name, float width, float height) {
    for (auto& e : entries_) {
        if (e.containerName == name) {
            e.prevWidth = e.width;
            e.prevHeight = e.height;
            e.width = width;
            e.height = height;
            e.changed = (e.prevWidth != width || e.prevHeight != height);
            return;
        }
    }
}

bool ContainerSizeObserver::hasChanges() const {
    for (const auto& e : entries_) {
        if (e.changed) return true;
    }
    return false;
}

std::vector<ContainerSizeObserver::Entry> ContainerSizeObserver::changedEntries() const {
    std::vector<Entry> changed;
    for (const auto& e : entries_) {
        if (e.changed) changed.push_back(e);
    }
    return changed;
}

void ContainerSizeObserver::clearChanges() {
    for (auto& e : entries_) e.changed = false;
}

} // namespace NXRender
