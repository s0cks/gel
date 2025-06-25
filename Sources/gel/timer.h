#ifndef GEL_TIMER_H
#define GEL_TIMER_H

#include <ostream>
#include <uv.h>

#include "gel/common.h"
#include "gel/natives.h"
#include "gel/object.h"
#include "gel/platform.h"
#include "gel/type.h"

namespace gel {
class Timer : public Object {
  friend class EventLoop;
  friend class proc::timer_set_repeat;

 private:
  static void OnTick(uv_timer_t* handle);

 private:
  uword id_;
  uv_timer_t handle_{};
  Procedure* on_tick_;

  Timer(uword id, Procedure* on_tick);

  void SetData(void* data) {
    ASSERT(data);
    uv_handle_set_data((uv_handle_t*)handle(), this);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  void SetRepeat(const uword rhs) {
    uv_timer_set_repeat(handle(), rhs);
  }

 public:
  ~Timer() override = default;

  auto GetId() const -> uword {
    return id_;
  }

  auto handle() const -> const uv_timer_t& {
    return handle_;
  }

  auto handle() -> uv_timer_t* {
    return &handle_;
  }

  auto GetRepeat() const -> uword {
    return uv_timer_get_repeat(&handle());
  }

  auto GetDueIn() const -> uword {
    return uv_timer_get_due_in(&handle());
  }

  auto GetData() const -> void* {
    return uv_handle_get_data((uv_handle_t*)this);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto GetCallback() const -> Procedure* {
    return on_tick_;
  }

  void Again() {
    const auto status = uv_timer_again(handle());
    LOG_IF(FATAL, status != 0) << "failed to run " << (*this) << " again: " << uv_strerror(status);
  }

  void Start(const uword timeout, const uword repeat) {
    const auto status = uv_timer_start(handle(), &OnTick, timeout, repeat);
    LOG_IF(FATAL, status != 0) << "failed to initialize uv_timer_t: " << uv_strerror(status);
  }

  void Stop() {
    const auto status = uv_timer_stop(handle());
    LOG_IF(FATAL, status != 0) << "failed to stop Timer: " << uv_strerror(status);
  }

  friend auto operator<<(std::ostream& stream, const Timer& rhs) -> std::ostream& {
    stream << "Timer(";
    stream << "id=" << rhs.GetId();
    stream << ")";
    return stream;
  }

  DECLARE_TYPE(Timer);

 public:
  static inline auto New(uword id, Procedure* on_tick) -> Timer* {
    ASSERT(on_tick);
    return new Timer(id, on_tick);
  }
};
}  // namespace gel

#endif  // GEL_TIMER_H
