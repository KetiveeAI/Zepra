// Copyright (c) 2025 KetiveeAI. All rights reserved.
// Licensed under KPL-2.0. See LICENSE file for details.
// webgl_stubs.cpp - Stub implementations for missing WebCore source files
// These files (webgl_bindings.cpp, webgl_context.cpp) are missing from the source
// but the pre-built libwebcore.a references their symbols

#include "graphics/webgl_bindings.hpp"
#include "graphics/webgl_context.hpp"
#include "browser/dom.hpp"

namespace Zepra::WebCore {

// ============================================================================
// WebGLRenderingContext destructor (from webgl_context.cpp - missing)
// ============================================================================

WebGLRenderingContext::~WebGLRenderingContext() {
    // Cleanup resources - stub implementation
}

// ============================================================================
// WebGLBindings stubs (from webgl_bindings.cpp - missing)
// ============================================================================

std::unordered_map<uint32_t, std::unique_ptr<WebGLRenderingContext>> WebGLBindings::contexts_;
uint32_t WebGLBindings::nextHandle_ = 1;

uint32_t WebGLBindings::createContext(int width, int height) {
    (void)width;
    (void)height;
    return 0; // WebGL not supported
}

WebGLRenderingContext* WebGLBindings::getContext(uint32_t handle) {
    auto it = contexts_.find(handle);
    return it != contexts_.end() ? it->second.get() : nullptr;
}

void WebGLBindings::destroyContext(uint32_t handle) {
    contexts_.erase(handle);
}

void WebGLBindings::registerNativeFunctions(Runtime::VM* vm) {
    (void)vm;
}

// NOTE: DOMElement::innerHTML, setInnerHTML, outerHTML are now in webCore/src/dom.cpp

} // namespace Zepra::WebCore

// ============================================================================
// MicrotaskQueue::drain stub (from ZepraScript runtime)
// ============================================================================

namespace Zepra::Runtime {

class MicrotaskQueue {
public:
    void drain();
};

void MicrotaskQueue::drain() {
    // No-op stub - microtasks disabled
}

} // namespace Zepra::Runtime
