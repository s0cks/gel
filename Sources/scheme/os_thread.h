#ifndef SCM_OS_THREAD_H
#define SCM_OS_THREAD_H

#include <string>

#include "scheme/common.h"
#ifdef OS_IS_LINUX
#include "scheme/os_thread_linux.h"
#elif OS_IS_OSX
#include "scheme/os_thread_osx.h"
#elif OS_IS_WINDOWS
#include "scheme/os_thread_windows.h"
#endif

namespace scm {
auto GetCurrentThreadId() -> ThreadId;
auto GetThreadName(const ThreadId& thread) -> std::string;
auto SetThreadName(const ThreadId& thread, const std::string& name) -> bool;
auto Start(ThreadId* thread, const std::string& name, const ThreadHandler& func, void* data) -> bool;
auto Join(const ThreadId& thread) -> bool;
auto Compare(const ThreadId& lhs, const ThreadId& rhs) -> bool;
auto GetCurrentThreadCount() -> int;

static inline auto GetCurrentThreadName() -> std::string {
  return GetThreadName(GetCurrentThreadId());
}

static inline auto SetCurrentThreadName(const std::string& name) -> bool {
  return SetThreadName(GetCurrentThreadId(), name);
}
}  // namespace scm

#endif  // SCM_OS_THREAD_H
