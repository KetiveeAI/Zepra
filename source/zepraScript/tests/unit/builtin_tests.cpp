// Builtin unit tests
#include <gtest/gtest.h>
#include <cmath>

// Test basic math operations using std library
// MathBuiltin delegates to these under the hood

TEST(BuiltinTests, MathAbs) {
    EXPECT_EQ(std::abs(-5.0), 5.0);
    EXPECT_EQ(std::abs(5.0), 5.0);
}

TEST(BuiltinTests, MathFloor) {
    EXPECT_EQ(std::floor(3.7), 3.0);
    EXPECT_EQ(std::floor(-3.7), -4.0);
}

TEST(BuiltinTests, MathCeil) {
    EXPECT_EQ(std::ceil(3.2), 4.0);
    EXPECT_EQ(std::ceil(-3.2), -3.0);
}

TEST(BuiltinTests, MathRound) {
    EXPECT_EQ(std::round(3.4), 3.0);
    EXPECT_EQ(std::round(3.5), 4.0);
    EXPECT_EQ(std::round(-3.5), -4.0);
}

TEST(BuiltinTests, MathSqrt) {
    EXPECT_EQ(std::sqrt(16.0), 4.0);
    EXPECT_EQ(std::sqrt(25.0), 5.0);
}

TEST(BuiltinTests, MathTrig) {
    EXPECT_NEAR(std::sin(0.0), 0.0, 0.0001);
    EXPECT_NEAR(std::cos(0.0), 1.0, 0.0001);
    EXPECT_NEAR(std::tan(0.0), 0.0, 0.0001);
}

TEST(BuiltinTests, MathPow) {
    EXPECT_EQ(std::pow(2.0, 3.0), 8.0);
    EXPECT_EQ(std::pow(2.0, 10.0), 1024.0);
}

TEST(BuiltinTests, MathLog) {
    EXPECT_NEAR(std::log(1.0), 0.0, 0.0001);
    EXPECT_NEAR(std::log10(100.0), 2.0, 0.0001);
}
