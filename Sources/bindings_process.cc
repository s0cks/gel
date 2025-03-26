#include <gel/plugin.h>
#include <glog/logging.h>

#include <cstdlib>

#include "gel/native_procedure.h"
#include "gel/uv.h"

#if defined(OS_IS_OSX) || defined(OS_IS_LINUX)
#include <sys/types.h>
#include <unistd.h>
#elif defined(OS_IS_WINDOWS)
#include <windows.h>
#else
#error "Unsupported Operating System"
#endif

using namespace gel;

_DECLARE_NATIVE_PROCEDURE(process_get_pid, "process/get-pid");
_DECLARE_NATIVE_PROCEDURE(process_get_cwd, "process/get-cwd");
_DECLARE_NATIVE_PROCEDURE(process_get_uid, "process/get-uid");
_DECLARE_NATIVE_PROCEDURE(process_get_gid, "process/get-gid");

NATIVE_PROCEDURE_F(process_get_cwd) {
#ifdef OS_IS_WINDOWS
  std::array<char, MAX_PATH> cwd{};
#elif defined(OS_IS_OSX) || defined(OS_IS_LINUX)
  std::array<char, PATH_MAX> cwd{};
#else
#error "Unsupported Operating System"
#endif
  size_t size = cwd.size();
  LOG_IF(FATAL, uv_cwd(cwd.data(), &size) != 0)
      << "failed to get cwd buffer of size: " << units::data::byte_t(static_cast<double>(size));
  return ReturnNew<String>(std::string(cwd.data(), size));
}

NATIVE_PROCEDURE_F(process_get_pid) {
#if defined(OS_IS_OSX) || defined(OS_IS_LINUX)
  const auto pid = getpid();
  return ReturnLong(static_cast<RawLong>(pid));
#elif defined(OS_IS_WINDOWS)
  const auto pid = static_cast<word>(GetCurrentProcessId());
  return ReturnLong(pid);
#else
  return ReturnNull();
#endif
}

NATIVE_PROCEDURE_F(process_get_gid) {
#if defined(OS_IS_OSX) || defined(OS_IS_LINUX)
  const auto gid = getgid();
  return ReturnLong(static_cast<RawLong>(gid));
#else
  return ReturnNull();
#endif
}

NATIVE_PROCEDURE_F(process_get_uid) {
#if defined(OS_IS_OSX) || defined(OS_IS_LINUX)
  const auto uid = getuid();
  return ReturnLong(static_cast<RawLong>(uid));
#else
  return ReturnNull();
#endif
}

DEFINE_PLUGIN(rx) {
  InitNative<process_get_pid>();
  InitNative<process_get_cwd>();
  InitNative<process_get_gid>();
  InitNative<process_get_uid>();
  return EXIT_SUCCESS;
}