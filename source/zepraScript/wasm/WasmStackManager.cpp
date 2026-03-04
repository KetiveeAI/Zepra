/**
 * @file WasmStackManager.cpp
 * @brief WebAssembly Stack Overflow Protection Implementation
 *
 * Manages per-thread stack limits and guard regions for safe WASM execution.
 * Prevents stack overflow in WASM code from crashing the process.
 */

#include "wasm/WasmStackManager.h"

#ifdef __linux__
#include <pthread.h>
#include <sys/resource.h>
#elif defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <pthread.h>
#endif

#include <cstdint>

namespace Zepra::Wasm {

thread_local uintptr_t StackManager::threadLimit_ = 0;


} // namespace Zepra::Wasm
