#include "gel/event_loop.h"

#include <uv.h>

#include "gel/common.h"
#include "gel/error.h"
#include "gel/runtime.h"
#include "gel/thread_local.h"
#include "gel/to_string_helper.h"

namespace gel {
auto EventLoop::Run(const uv_run_mode mode) -> int {
  return uv_run(Get(), mode);
}

auto EventLoop::CreateTimer(Procedure* on_tick) -> Timer* {
  const auto timer = Timer::New(timers_.size() + 1, on_tick);
  ASSERT(timer);
  timers_.push_back(timer);
  return timer;
}

auto EventLoop::GetTimer(const uword idx) const -> Timer* {
  const auto pos = std::find_if(std::begin(timers()), std::begin(timers()), [idx](Timer* timer) {
    return timer->GetId() == idx;
  });
  return pos != std::end(timers()) ? (*pos) : nullptr;
}

static inline auto WrapOnError(Procedure* on_error) -> OnErrorCallback {
  return [on_error](Error* error) {
    ASSERT(error);
    if (on_error)
      return GetRuntime()->Call(on_error, {error});
  };
}

static inline auto WrapOnSuccess(Procedure* on_success) -> OnSuccessCallback {
  return [on_success]() {
    if (on_success)
      return GetRuntime()->Call(on_success);
  };
}

static inline auto WrapOnFinished(Procedure* on_finished) -> OnFinishedCallback {
  return [on_finished]() {
    ASSERT(on_finished);
    if (on_finished)
      return GetRuntime()->Call(on_finished);
  };
}

auto EventLoop::Stat(const std::string& path, Procedure* on_next, Procedure* on_error, Procedure* on_finished) -> bool {
  ASSERT(!path.empty());
  ASSERT(on_next);
  return Stat(
      path,
      [on_next](int stat) {
        return GetRuntime()->Call(on_next, {Long::New(stat)});
      },
      WrapOnError(on_error), WrapOnFinished(on_finished));
}

auto EventLoop::Stat(const std::string& path, const std::function<void(uword)>& on_next, const OnErrorCallback& on_error,
                     const OnFinishedCallback& on_finished) -> bool {
  ASSERT(!path.empty());
  const auto request = new fs::StatRequest(path, on_next, on_error, on_finished);
  ASSERT(request);
  return request->Execute(this) == 0;
}

auto EventLoop::Rename(const std::string& old_path, const std::string& new_path, const OnErrorCallback& on_error,
                       const OnFinishedCallback& on_finished) -> bool {
  ASSERT(!old_path.empty());
  ASSERT(!new_path.empty());
  const auto request = new fs::RenameRequest(old_path, new_path, on_error, on_finished);
  ASSERT(request);
  return request->Execute(this) == 0;
}

auto EventLoop::Rename(const std::string& old_path, const std::string& new_path, Procedure* on_error, Procedure* on_finished)
    -> bool {
  ASSERT(!old_path.empty());
  ASSERT(!new_path.empty());
  return Rename(old_path, new_path, WrapOnError(on_error), WrapOnFinished(on_finished));
}

auto EventLoop::Mkdir(const std::string& path, const int mode, const OnSuccessCallback& on_success,
                      const OnErrorCallback& on_error, const OnFinishedCallback& on_finished) -> bool {
  ASSERT(!path.empty());
  const auto request = new fs::MkdirRequest(path, mode, on_success, on_error, on_finished);
  ASSERT(request);
  return request->Execute(this) == 0;
}

auto EventLoop::Mkdir(const std::string& path, const int mode, Procedure* on_success, Procedure* on_error, Procedure* on_finished)
    -> bool {
  ASSERT(!path.empty());
  return Mkdir(path, mode, WrapOnSuccess(on_success), WrapOnError(on_error), WrapOnFinished(on_finished));
}

auto EventLoop::ToString() const -> std::string {
  ToStringHelper<EventLoop> helper;
  helper.AddField("data", (const void*)Get());
  return helper;
}

auto EventLoop::HashCode() const -> uword {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
}

auto EventLoop::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsEventLoop())
    return false;
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto EventLoop::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Object::GetClass(), "EventLoop");
}

auto EventLoop::New(const ObjectList& args) -> EventLoop* {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return nullptr;
}

static ThreadLocal<EventLoop> kEventLoop;

void EventLoop::Init() {
  InitClass();
  Timer::InitClass();
}

auto GetThreadEventLoop() -> EventLoop* {
  if (kEventLoop)
    return kEventLoop;
  const auto loop = EventLoop::New();
  ASSERT(loop);
  kEventLoop = loop;
  return loop;
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

namespace fs {
FS_REQUEST_CALLBACK_F(RenameRequest) {
  ASSERT(handle);
  const auto request = From<RenameRequest>(handle);
  ASSERT(request);
  const auto result = request->handle()->result;
  if (result == -1) {
    const auto message =
        fmt::format("error reading stats of file {}: {}", request->GetPath(), uv_strerror(static_cast<int>(result)));
    return request->OnError(Error::New(message));
  }
  uv_fs_req_cleanup(handle);
  request->OnFinished();
}

FS_REQUEST_EXECUTE_F(RenameRequest) {
  ASSERT(loop);
  const auto& path = GetPath();
  const auto& new_path = GetNewPath();
  return uv_fs_rename(loop->Get(), handle(), path.c_str(), new_path.c_str(), &OnRenameRequest);
}

FS_REQUEST_CALLBACK_F(MkdirRequest) {
  ASSERT(handle);
  const auto request = From<MkdirRequest>(handle);
  ASSERT(request);
  const auto result = request->GetResult();
  if (result == -1) {
    const auto message =
        fmt::format("error reading stats of file {}: {}", request->GetPath(), uv_strerror(static_cast<int>(result)));
    request->OnError(Error::New(message));
  } else {
    request->OnSuccess();
  }
  uv_fs_req_cleanup(handle);
  request->OnFinished();
}

FS_REQUEST_EXECUTE_F(MkdirRequest) {
  ASSERT(loop);
  const auto& path = GetPath();
  return uv_fs_mkdir(loop->Get(), handle(), path.c_str(), GetMode(), &OnMkdirRequest);
}

FS_REQUEST_CALLBACK_F(StatRequest) {
  ASSERT(handle);
  const auto request = From<StatRequest>(handle);
  ASSERT(request);
  const auto result = request->handle()->result;
  if (result == -1) {
    const auto message =
        fmt::format("error reading stats of file {}: {}", request->GetPath(), uv_strerror(static_cast<int>(result)));
    return request->OnError(Error::New(message));
  }
  request->OnNext(request->handle()->statbuf.st_size);
  uv_fs_req_cleanup(handle);
  request->OnFinished();
}

FS_REQUEST_EXECUTE_F(StatRequest) {
  ASSERT(loop);
  const auto& path = GetPath();
  return uv_fs_stat(loop->Get(), handle(), path.c_str(), &OnStatRequest);
}
}  // namespace fs
}  // namespace gel