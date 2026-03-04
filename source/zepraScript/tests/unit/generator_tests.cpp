/**
 * @file generator_tests.cpp
 * @brief Unit tests for Generator Functions and Iterators
 */

#include <gtest/gtest.h>
#include "runtime/async/GeneratorAPI.h"
#include "runtime/async/IteratorAPI.h"
#include <vector>
#include <string>
#include <stdexcept>

using namespace Zepra::Runtime;

// =============================================================================
// Generator State Tests
// =============================================================================

class GeneratorTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(GeneratorTests, GeneratorCreation) {
    auto gen = std::make_unique<Generator>([](GeneratorContext& ctx) {
        ctx.yield(1.0);
        ctx.yield(2.0);
        ctx.yield(3.0);
    });
    
    EXPECT_EQ(gen->state(), GeneratorState::SuspendedStart);
}

TEST_F(GeneratorTests, GeneratorFirstNext) {
    auto gen = std::make_unique<Generator>([](GeneratorContext& ctx) {
        ctx.yield(42.0);
    });
    
    auto result = gen->next();
    
    EXPECT_FALSE(result.done);
}

TEST_F(GeneratorTests, GeneratorCompletion) {
    auto gen = std::make_unique<Generator>([](GeneratorContext& ctx) {
        // Empty generator - completes immediately
    });
    
    auto result = gen->next();
    
    EXPECT_TRUE(result.done);
}

TEST_F(GeneratorTests, GeneratorReturn) {
    auto gen = std::make_unique<Generator>([](GeneratorContext& ctx) {
        ctx.yield(1.0);
        ctx.yield(2.0);
    });
    
    gen->next();  // yield 1
    auto result = gen->return_(100.0);
    
    EXPECT_TRUE(result.done);
}

TEST_F(GeneratorTests, GeneratorThrow) {
    auto gen = std::make_unique<Generator>([](GeneratorContext& ctx) {
        ctx.yield(1.0);
    });
    
    gen->next();  // Start generator
    
    EXPECT_THROW(gen->throw_(std::string("Test error")), std::runtime_error);
}

// =============================================================================
// Iterator Protocol Tests - use static factory methods correctly
// =============================================================================

class IteratorTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(IteratorTests, IteratorResultDone) {
    auto result = IteratorResult::done();
    EXPECT_TRUE(result.done);
}

TEST_F(IteratorTests, IteratorResultValueDouble) {
    IteratorValue val = 42.0;
    auto result = IteratorResult{val, false};
    EXPECT_FALSE(result.done);
}

TEST_F(IteratorTests, IteratorResultValueString) {
    IteratorValue val = std::string("hello");
    auto result = IteratorResult{val, false};
    EXPECT_FALSE(result.done);
}

// =============================================================================
// YieldBuilder Tests
// =============================================================================

TEST(YieldBuilderTests, BuildSimpleGenerator) {
    YieldBuilder builder;
    builder << 1.0;
    builder << 2.0;
    builder << 3.0;
    
    auto gen = builder.build();
    
    auto r1 = gen->next();
    EXPECT_FALSE(r1.done);
    
    auto r2 = gen->next();
    EXPECT_FALSE(r2.done);
    
    auto r3 = gen->next();
    EXPECT_FALSE(r3.done);
    
    auto r4 = gen->next();
    EXPECT_TRUE(r4.done);
}

// =============================================================================
// Generator Context Tests
// =============================================================================

TEST(GeneratorContextTests, YieldCallback) {
    GeneratorContext ctx;
    
    std::vector<IteratorValue> yielded;
    ctx.setYieldCallback([&yielded](IteratorValue v) {
        yielded.push_back(std::move(v));
    });
    
    ctx.yield(1.0);
    ctx.yield(2.0);
    
    EXPECT_EQ(yielded.size(), 2);
}

TEST(GeneratorContextTests, ResumeValue) {
    GeneratorContext ctx;
    
    ctx.setResumeValue(42.0);
    
    auto resume = ctx.resumeValue();
    ASSERT_TRUE(resume.has_value());
}

TEST(GeneratorContextTests, ReturnValue) {
    GeneratorContext ctx;
    
    ctx.setReturnValue(std::string("done"));
    
    auto ret = ctx.returnValue();
    ASSERT_TRUE(ret.has_value());
}

// =============================================================================
// ArrayIterator Tests
// =============================================================================

TEST(ArrayIteratorTests, IterateDoubles) {
    std::vector<double> arr = {1.0, 2.0, 3.0};
    ArrayIterator<double> iter(arr);
    
    auto r1 = iter.next();
    EXPECT_FALSE(r1.done);
    
    auto r2 = iter.next();
    EXPECT_FALSE(r2.done);
    
    auto r3 = iter.next();
    EXPECT_FALSE(r3.done);
    
    auto r4 = iter.next();
    EXPECT_TRUE(r4.done);
}

TEST(ArrayIteratorTests, IterateStrings) {
    std::vector<std::string> arr = {"a", "b", "c"};
    ArrayIterator<std::string> iter(arr);
    
    auto r1 = iter.next();
    EXPECT_FALSE(r1.done);
}

// =============================================================================
// StringIterator Tests
// =============================================================================

TEST(StringIteratorTests, IterateASCII) {
    StringIterator iter("abc");
    
    auto r1 = iter.next();
    EXPECT_FALSE(r1.done);
    
    auto r2 = iter.next();
    EXPECT_FALSE(r2.done);
    
    auto r3 = iter.next();
    EXPECT_FALSE(r3.done);
    
    auto r4 = iter.next();
    EXPECT_TRUE(r4.done);
}

// =============================================================================
// FunctionIterator Tests
// =============================================================================

TEST(FunctionIteratorTests, CustomNext) {
    int count = 0;
    FunctionIterator iter([&count]() -> IteratorResult {
        if (count >= 3) {
            return IteratorResult::done();
        }
        count++;
        IteratorValue val = static_cast<double>(count);
        return IteratorResult{val, false};
    });
    
    EXPECT_FALSE(iter.next().done);
    EXPECT_FALSE(iter.next().done);
    EXPECT_FALSE(iter.next().done);
    EXPECT_TRUE(iter.next().done);
}
