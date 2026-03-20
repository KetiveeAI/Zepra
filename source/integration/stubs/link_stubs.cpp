// link_stubs.cpp — Minimal stubs for unresolved symbols during linking
// These functions are referenced but not yet implemented

#include "nxrender_cpp.h"
#include "rendering/page_renderer.hpp"

namespace NXRender {

void dispatchEvent(const Event&) {
    // Stub — event dispatch not wired yet
}

} // namespace NXRender

namespace Zepra::WebCore {

void PageRenderer::invalidateAll() {
    // Stub — full invalidation not wired yet
}

} // namespace Zepra::WebCore
