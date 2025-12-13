// VM unit tests stub
#include <gtest/gtest.h>
#include "zeprascript/runtime/vm.hpp"

using namespace Zepra::Runtime;

TEST(VMTests, Stack) {
    VM vm(nullptr);
    vm.push(Value::number(42));
    EXPECT_EQ(vm.stackSize(), 1);
    EXPECT_EQ(vm.peek().asNumber(), 42.0);
    Value v = vm.pop();
    EXPECT_EQ(v.asNumber(), 42.0);
    EXPECT_EQ(vm.stackSize(), 0);
}
