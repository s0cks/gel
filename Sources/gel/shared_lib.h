#ifndef GEL_SHARED_LIB_H
#define GEL_SHARED_LIB_H

#include <uv.h>

#include "gel/common.h"

namespace gel {
class SharedLibrary {
  DEFINE_NON_COPYABLE_TYPE(SharedLibrary);

 private:
  uv_lib_t handle_{};

  inline auto handle() -> uv_lib_t* {
    return &handle_;
  }

  static inline auto GetError(const uv_lib_t& handle) -> const char* {
    return uv_dlerror(&handle);
  }

  static inline void Open(uv_lib_t& handle, const char* filename) {
    ASSERT(filename && strlen(filename) > 0);
    const auto status = uv_dlopen(filename, &handle);
    LOG_IF(FATAL, status != 0) << "failed to open shared library from " << filename << ": " << GetError(handle);
    DVLOG(1) << "shared library from " << filename << " opened!";
  }

 public:
  explicit SharedLibrary(const char* filename) {
    Open(handle_, filename);
  }
  explicit SharedLibrary(const std::string& filename) :
    SharedLibrary(filename.c_str()) {}
  ~SharedLibrary() {
    Close();
  }

  template <typename Func>
  auto DlSym(const char* name) -> Func {
    ASSERT(name && strlen(name) > 0);
    Func func;
    const auto status = uv_dlsym(&handle_, name, (void**)&func);
    LOG_IF(FATAL, status != 0) << "failed to dlsym " << name << " in " << (*this);
    DVLOG(10) << "found dlsym " << name << " in " << (*this);
    return func;
  }

  void Close() {
    uv_dlclose(&handle_);
  }

  friend auto operator<<(std::ostream& stream, const SharedLibrary& rhs) -> std::ostream& {
    stream << "SharedLibrary(";
    stream << "handle=" << &rhs.handle_;
    stream << ")";
    return stream;
  }
};
}  // namespace gel

#endif  // GEL_SHARED_LIB_H
