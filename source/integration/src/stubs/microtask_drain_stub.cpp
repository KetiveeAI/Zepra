/**
 * @file microtask_drain_stub.cpp
 * @brief Provides the MicrotaskQueue::drain() symbol needed by pre-built webCore
 * 
 * The webCore library was compiled calling drain() but zepra-core only provides process().
 * This stub provides the missing symbol by forwarding to process().
 */

#include <zeprascript/runtime/promise.hpp>

namespace Zepra::Runtime {

void MicrotaskQueue::drain() {
    process();
}

} // namespace Zepra::Runtime
