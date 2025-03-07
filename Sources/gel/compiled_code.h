#ifndef GEL_COMPILED_CODE_H
#define GEL_COMPILED_CODE_H

#include "gel/common.h"
#include "gel/platform.h"
#include "gel/region.h"

namespace gel {
class CompiledCode {
  friend class FlowGraphCompiler;
  DEFINE_DEFAULT_COPYABLE_TYPE(CompiledCode);

 private:
  uword start_{};
  uword size_{};
#ifdef GEL_DEBUG
  uword compile_time_ns_{};

  void SetCompileTime(const uword ns) {
    compile_time_ns_ = ns;
  }
#endif  // GEL_DEBUG

 public:
  CompiledCode() = default;
  CompiledCode(const uword start, const uword size) :
    start_(start),
    size_(size) {}
  explicit CompiledCode(const Region& region) :
    CompiledCode(region.GetStartingAddress(), region.GetSize()) {}
  ~CompiledCode() = default;

  auto GetStartingAddress() const -> uword {
    return start_;
  }

  auto GetSize() const -> uword {
    return size_;
  }

  inline auto IsCompiled() const -> bool {
    return GetStartingAddress() != UNALLOCATED && GetSize() > 0;
  }

#ifdef GEL_DEBUG
  auto GetCompileTime() const -> uword {
    return compile_time_ns_;
  }
#endif  // GEL_DEBUG

  operator Region() const {
    return {GetStartingAddress(), GetSize()};
  }
};
}  // namespace gel

#endif  // GEL_COMPILED_CODE_H
