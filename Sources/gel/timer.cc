#include "gel/timer.h"

#include "gel/event_loop.h"
#include "gel/runtime.h"
#include "gel/to_string_helper.h"

namespace gel {
Timer::Timer(uword id, Procedure* on_tick) :
  Object(),
  id_(id),
  on_tick_(on_tick) {
  ASSERT(on_tick_);
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  {
    const auto status = uv_timer_init(loop->Get(), handle());
    LOG_IF(FATAL, status != 0) << "failed to initialize uv_timer_t: " << uv_strerror(status);
  }
  SetData(this);
}

auto Timer::ToString() const -> std::string {
  ToStringHelper<Timer> helper;
  helper.AddField("handle", (const void*)&handle());
  return helper;
}

auto Timer::HashCode() const -> uword {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
}

auto Timer::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsTimer())
    return false;
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto Timer::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Object::GetClass(), "Timer");
}

auto Timer::New(const ObjectList& args) -> Timer* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return nullptr;
}

void Timer::OnTick(uv_timer_t* handle) {
  const auto timer = ((Timer*)uv_handle_get_data((uv_handle_t*)handle));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  ASSERT(timer);
  const auto on_tick = timer->GetCallback();
  ASSERT(on_tick);
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  return runtime->Call(on_tick);
}

auto Timer::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}
}  // namespace gel