// Parser unit tests stub
#include <gtest/gtest.h>
#include "zeprascript/frontend/parser.hpp"

using namespace Zepra::Frontend;

TEST(ParserTests, EmptyProgram) {
    auto program = parse("", "test.js");
    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->body().size(), 0);
}
