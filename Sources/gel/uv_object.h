#ifndef GEL_OBJECT_H
#error "Please #include <gel/object.h> instead."
#endif  // GEL_OBJECT_H

#ifndef GEL_UV_OBJECT_H
#define GEL_UV_OBJECT_H

#include <uv.h>

#include "gel/object.h"

namespace gel {
class Loop : public Instance {
 public:
  using RunMode = uv_run_mode;

 private:
  uv_loop_t* loop_;

 protected:
  explicit Loop(uv_loop_t* loop) :
    Instance(GetClass()),
    loop_(loop) {
    ASSERT(loop_);
  }

 public:
  ~Loop() override = default;

  auto Get() const -> uv_loop_t* {
    return loop_;
  }

  void Run(const RunMode mode);
  DECLARE_TYPE(Loop);

 public:
  static inline auto New(uv_loop_t* loop = uv_loop_new()) -> Loop* {
    ASSERT(loop);
    return new Loop(loop);
  }
};
}  // namespace gel

#endif  // GEL_UV_OBJECT_H
