/**
 * @file type_profiler.cpp
 * @brief Type profiling implementation for JIT optimization
 * 
 * Production-ready type feedback collection for speculative compilation.
 */

#include "jit/type_profiler.hpp"
#include <cmath>

namespace Zepra::JIT {

// =============================================================================
// TypeFeedbackVector Implementation
// =============================================================================

void TypeFeedbackVector::recordType(size_t siteIndex, const Runtime::Value& value) {
    if (siteIndex >= MAX_SITES) return;
    
    auto& profile = sites_[siteIndex];
    ObservedType type = observeType(value);
    
    profile.observedTypes |= type;
    profile.sampleCount++;
}

void TypeFeedbackVector::recordBinaryOp(size_t siteIndex,
                                         const Runtime::Value& left,
                                         const Runtime::Value& right,
                                         const Runtime::Value& result) {
    if (siteIndex >= MAX_SITES) return;
    
    auto& feedback = binaryOps_[siteIndex];
    
    ObservedType leftType = observeType(left);
    ObservedType rightType = observeType(right);
    ObservedType resultType = observeType(result);
    
    feedback.profile.leftType |= leftType;
    feedback.profile.rightType |= rightType;
    feedback.profile.resultType |= resultType;
    feedback.profile.observedTypes |= leftType | rightType | resultType;
    feedback.profile.sampleCount++;
    
    // Update optimization hint after collecting samples
    if (feedback.profile.sampleCount >= 10) {
        feedback.updateHint();
    }
}

void TypeFeedbackVector::recordOverflow(size_t siteIndex) {
    if (siteIndex >= MAX_SITES) return;
    
    binaryOps_[siteIndex].overflowCount++;
    
    // Downgrade hint if overflow detected
    if (binaryOps_[siteIndex].optimizationHint == BinaryOpFeedback::Hint::Int32Add ||
        binaryOps_[siteIndex].optimizationHint == BinaryOpFeedback::Hint::Int32Sub ||
        binaryOps_[siteIndex].optimizationHint == BinaryOpFeedback::Hint::Int32Mul) {
        binaryOps_[siteIndex].optimizationHint = BinaryOpFeedback::Hint::DoubleArithmetic;
    }
}

// =============================================================================
// TypeProfiler Implementation
// =============================================================================

TypeFeedbackVector* TypeProfiler::getVector(uintptr_t functionId) {
    if (!enabled_) return nullptr;
    
    auto it = vectors_.find(functionId);
    if (it != vectors_.end()) {
        return &it->second;
    }
    
    // Create new vector (limit total vectors to prevent memory explosion)
    if (vectors_.size() >= 4096) {
        return nullptr;  // Too many functions tracked
    }
    
    vectors_[functionId] = TypeFeedbackVector{};
    return &vectors_[functionId];
}

void TypeProfiler::recordType(uintptr_t functionId, size_t site, const Runtime::Value& value) {
    if (!enabled_) return;
    
    auto* vector = getVector(functionId);
    if (vector) {
        vector->recordType(site, value);
    }
}

void TypeProfiler::recordBinaryOp(uintptr_t functionId, size_t site,
                                   const Runtime::Value& left,
                                   const Runtime::Value& right,
                                   const Runtime::Value& result) {
    if (!enabled_) return;
    
    auto* vector = getVector(functionId);
    if (vector) {
        vector->recordBinaryOp(site, left, right, result);
    }
}

bool TypeProfiler::shouldSpecializeInt32(uintptr_t functionId, size_t site) const {
    auto it = vectors_.find(functionId);
    if (it == vectors_.end()) return false;
    
    const auto& vector = it->second;
    auto* profile = const_cast<TypeFeedbackVector&>(vector).getProfile(site);
    if (!profile) return false;
    
    // Need sufficient samples and only int32 observed
    return profile->sampleCount >= 10 && profile->isInt32Only();
}

bool TypeProfiler::shouldSpecializeDouble(uintptr_t functionId, size_t site) const {
    auto it = vectors_.find(functionId);
    if (it == vectors_.end()) return false;
    
    const auto& vector = it->second;
    auto* profile = const_cast<TypeFeedbackVector&>(vector).getProfile(site);
    if (!profile) return false;
    
    // Need sufficient samples and only numbers (including doubles)
    return profile->sampleCount >= 10 && profile->isNumberOnly() && !profile->isInt32Only();
}

bool TypeProfiler::shouldSpecializeString(uintptr_t functionId, size_t site) const {
    auto it = vectors_.find(functionId);
    if (it == vectors_.end()) return false;
    
    const auto& vector = it->second;
    auto* profile = const_cast<TypeFeedbackVector&>(vector).getProfile(site);
    if (!profile) return false;
    
    return profile->sampleCount >= 10 && profile->isStringOnly();
}

// =============================================================================
// Integration with existing JIT Profiler
// =============================================================================

// Helper for VM integration - record arithmetic operation types
void recordArithmeticTypes(uintptr_t functionId, size_t bytecodeOffset,
                           const Runtime::Value& left,
                           const Runtime::Value& right,
                           const Runtime::Value& result) {
    TypeProfiler::instance().recordBinaryOp(functionId, bytecodeOffset, left, right, result);
}

// Helper for VM integration - record general value type
void recordValueType(uintptr_t functionId, size_t bytecodeOffset,
                     const Runtime::Value& value) {
    TypeProfiler::instance().recordType(functionId, bytecodeOffset, value);
}

// Helper for VM integration - record integer overflow
void recordIntegerOverflow(uintptr_t functionId, size_t bytecodeOffset) {
    auto* vector = TypeProfiler::instance().getVector(functionId);
    if (vector) {
        vector->recordOverflow(bytecodeOffset);
    }
}

// Check if addition should use fast int32 path
bool canUseInt32Add(uintptr_t functionId, size_t bytecodeOffset) {
    auto* vector = TypeProfiler::instance().getVector(functionId);
    if (!vector) return false;
    
    auto* feedback = vector->getBinaryOpFeedback(bytecodeOffset);
    if (!feedback) return false;
    
    return feedback->optimizationHint == BinaryOpFeedback::Hint::Int32Add &&
           !feedback->hasOverflowed();
}

// Check if operation should use fast double path
bool canUseDoubleArithmetic(uintptr_t functionId, size_t bytecodeOffset) {
    auto* vector = TypeProfiler::instance().getVector(functionId);
    if (!vector) return false;
    
    auto* feedback = vector->getBinaryOpFeedback(bytecodeOffset);
    if (!feedback) return false;
    
    return feedback->optimizationHint == BinaryOpFeedback::Hint::DoubleArithmetic;
}

// Get optimization hint for JIT compilation
BinaryOpFeedback::Hint getOptimizationHint(uintptr_t functionId, size_t bytecodeOffset) {
    auto* vector = TypeProfiler::instance().getVector(functionId);
    if (!vector) return BinaryOpFeedback::Hint::Generic;
    
    auto* feedback = vector->getBinaryOpFeedback(bytecodeOffset);
    if (!feedback) return BinaryOpFeedback::Hint::Generic;
    
    return feedback->optimizationHint;
}

} // namespace Zepra::JIT
