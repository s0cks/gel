#ifndef GEL_COMPILED_CODE_H
#define GEL_COMPILED_CODE_H

#include <ostream>
#include <string>

#include "gel/allocator.h"
#include "gel/platform.h"
#include "gel/region.h"

namespace gel {
class CompiledCode : public HeapObject {
  friend class FlowGraphCompiler;
  DECLARE_HEAP_ALLOC_TYPE(CompiledCode);

 private:
  uword start_ = 0;
  uword size_ = 0;
#ifdef GEL_DEBUG
  uword compile_time_ns_ = 0;
#endif  // GEL_DEBUG

  CompiledCode(const uword start, const uword size) :
    start_(start),
    size_(size) {}
  explicit CompiledCode(const Region& region) :
    CompiledCode(region.GetStartingAddress(), region.GetSize()) {}

#ifdef GEL_DEBUG
  void SetCompileTime(const uword ns) {
    compile_time_ns_ = ns;
  }
#endif  // GEL_DEBUG

 public:
  ~CompiledCode() override = default;

  auto GetCodeStartingAddress() const -> uword {
    return start_;
  }

  auto GetCodeSize() const -> uword {
    return size_;
  }

  auto IsCompiled() const -> bool {
    return GetCodeStartingAddress() != UNALLOCATED && GetCodeSize() > 0;
  }

  auto GetRegion() const -> Region {
    return {GetCodeStartingAddress(), GetCodeSize()};
  }

#ifdef GEL_DEBUG
  auto GetCompileTime() const -> uword {
    return compile_time_ns_;
  }
#endif  // GEL_DEBUG

  auto ToString() const -> std::string override;

  inline friend auto operator<<(std::ostream& stream, const CompiledCode& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }

 public:
  static inline auto New(const Region& region) -> CompiledCode* {
    return new CompiledCode(region);
  }
};
}  // namespace gel

#endif  // GEL_COMPILED_CODE_H
