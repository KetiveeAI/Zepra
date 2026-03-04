/**
 * @file tab_suspender_stubs.cpp
 * @brief TabSuspender stub implementation (source file disabled in build)
 */

#include "browser/tab_suspender.h"

namespace ZepraBrowser {

// Default constructor/destructor definitions
TabSuspender::TabSuspender() {}
TabSuspender::~TabSuspender() {}

// All other methods are stubs
TabSuspendState TabSuspender::getState(int) const { return TabSuspendState::ACTIVE; }
void TabSuspender::setState(int, TabSuspendState) {}
void TabSuspender::transitionToSleep(Tab*) {}
void TabSuspender::transitionToLightSleep(Tab*) {}
void TabSuspender::transitionToDeepSleep(Tab*) {}
void TabSuspender::restore(Tab*) {}
void TabSuspender::checkTab(Tab*) {}
void TabSuspender::checkAllTabs(const std::vector<Tab*>&, int) {}
void TabSuspender::tick() {}
void TabSuspender::setContentType(int, TabContentType) {}
TabContentType TabSuspender::getContentType(int) const { return TabContentType::NORMAL; }
void TabSuspender::notifyHostIdle(bool) {}
void TabSuspender::notifyMemoryPressure(bool) {}
TabSnapshot* TabSuspender::getSnapshot(int) { return nullptr; }
bool TabSuspender::hasSnapshot(int) const { return false; }
void TabSuspender::clearSnapshot(int) {}
SuspenderStats TabSuspender::getStats() const { return SuspenderStats{}; }

} // namespace ZepraBrowser
