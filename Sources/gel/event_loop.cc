#include "event_loop.h"

#include <algorithm>
#include <fmt/format.h>
#include <functional>
#include <iterator>
#include <string>
#include <utility>
#include <uv.h>

#include "common.h"
#include "error.h"
#include "hashcode.h"
#include "object.h"
#include "platform.h"
#include "pointer.h"
#include "procedure.h"
#include "runtime.h"
#include "thread_local.h"
#include "timer.h"
#include "to_string_helper.h"
#include "type.h"

namespace gel {
auto WrapOnError(Procedure* on_error) -> OnErrorCallback {
  return on_error->IsError() ? OnErrorCallback{} : [on_error](Error* error) {
    return GetRuntime()->Call(*on_error, ObjectList{error});
  };
}

auto WrapOnSuccess(Procedure* on_success) -> OnSuccessCallback {
  return on_success->IsNil() ? OnSuccessCallback{} : [on_success]() {
    return GetRuntime()->Call(*on_success);
  };
}

auto WrapOnFinished(Procedure* on_finished) -> OnFinishedCallback {
  return on_finished->IsNil() ? OnFinishedCallback{} : [on_finished]() {
    return GetRuntime()->Call(*on_finished);
  };
}

auto EventLoop::Run(const uv_run_mode mode) -> int {
  return uv_run(Get(), mode);
}

auto EventLoop::Submit(fs::RequestBase* request) -> int {
  ASSERT(request);
  return request->Execute(this);
}

auto EventLoop::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
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

auto EventLoop::Stat(const std::string& path, Procedure* on_next, Procedure* on_error, Procedure* on_finished) -> bool {
  ASSERT(!path.empty());
  ASSERT(on_next);
  return Stat(
      path,
      [on_next](int stat) {
        return GetRuntime()->Call(*on_next, {Long::New(stat)});
      },
      WrapOnError(on_error), WrapOnFinished(on_finished));
}

auto EventLoop::Stat(const std::string& path, const std::function<void(uword)>& on_next,
                     const OnErrorCallback& on_error, const OnFinishedCallback& on_finished) -> bool {
  ASSERT(!path.empty());
  const auto request = new fs::StatRequest(path, on_next, on_error, on_finished);
  ASSERT(request);
  return request->Execute(this) == 0;
}

auto EventLoop::Rename(const std::string& old_path, const std::string& new_path, const OnSuccessCallback& on_success,
                       const OnErrorCallback& on_error, const OnFinishedCallback& on_finished) -> bool {
  ASSERT(!old_path.empty());
  ASSERT(!new_path.empty());
  const auto request = new fs::RenameRequest(old_path, new_path, on_success, on_error, on_finished);
  ASSERT(request);
  return request->Execute(this) == 0;
}

auto EventLoop::Rename(const std::string& old_path, const std::string& new_path, Procedure* on_success,
                       Procedure* on_error, Procedure* on_finished) -> bool {
  ASSERT(!old_path.empty());
  ASSERT(!new_path.empty());
  return Rename(old_path, new_path, WrapOnSuccess(on_success), WrapOnError(on_error), WrapOnFinished(on_finished));
}

auto EventLoop::Mkdir(const std::string& path, const int mode, const OnSuccessCallback& on_success,
                      const OnErrorCallback& on_error, const OnFinishedCallback& on_finished) -> bool {
  ASSERT(!path.empty());
  const auto request = new fs::MkdirRequest(path, mode, on_success, on_error, on_finished);
  ASSERT(request);
  return request->Execute(this) == 0;
}

auto EventLoop::Mkdir(const std::string& path, const int mode, Procedure* on_success, Procedure* on_error,
                      Procedure* on_finished) -> bool {
  ASSERT(!path.empty());
  return Mkdir(path, mode, WrapOnSuccess(on_success), WrapOnError(on_error), WrapOnFinished(on_finished));
}

auto EventLoop::Rmdir(const std::string& path, const OnSuccessCallback& on_success, const OnErrorCallback& on_error,
                      const OnFinishedCallback& on_finished) -> bool {
  ASSERT(!path.empty());
  const auto request = new fs::RmdirRequest(path, on_success, on_error, on_finished);
  ASSERT(request);
  return request->Execute(this) == 0;
}

auto EventLoop::Rmdir(const std::string& path, Procedure* on_success, Procedure* on_error, Procedure* on_finished)
    -> bool {
  ASSERT(!path.empty());
  return Rmdir(path, WrapOnSuccess(on_success), WrapOnError(on_error), WrapOnFinished(on_finished));
}

auto EventLoop::ToString() const -> std::string {
  ToStringHelper<EventLoop> helper;
  helper.AddField("data", (const void*)Get());
  return helper;
}

auto EventLoop::GetHashCode() const -> HashCode {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return kInvalidHashCode;
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

auto VisitThreadEventLoopPointer(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  const auto event_loop = GetThreadEventLoop();
  ASSERT(event_loop);
  return vis->Visit(event_loop->raw_ptr());
}

auto VisitThreadEventLoopPointerPointer(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  const auto event_loop = GetThreadEventLoop();
  ASSERT(event_loop);
  auto raw_ptr = event_loop->raw_ptr();
  ASSERT(raw_ptr);
  if (!vis->Visit(&raw_ptr))
    return false;
  if (!event_loop->raw_ptr()->Equals(raw_ptr))
    kEventLoop.Set(raw_ptr->As<EventLoop>());
  return true;
}

auto VisitThreadEventLoopPointerPointer(const std::function<bool(Pointer**)>& vis) -> bool {
  ASSERT(vis);
  const auto event_loop = GetThreadEventLoop();
  ASSERT(event_loop);
  auto raw_ptr = event_loop->raw_ptr();
  ASSERT(raw_ptr);
  if (!vis(&raw_ptr))
    return false;
  if (!event_loop->raw_ptr()->Equals(raw_ptr))
    kEventLoop.Set(raw_ptr->As<EventLoop>());
  return true;
}

auto GetThreadEventLoop() -> EventLoop* {
  if (kEventLoop)
    return kEventLoop;
  const auto loop = EventLoop::New();
  ASSERT(loop);
  kEventLoop = loop;
  return loop;
}

void RunCurrentThreadEventLoop(const uv_run_mode mode) {
  const auto event_loop = GetThreadEventLoop();
  ASSERT(event_loop);
  while (event_loop->Run(UV_RUN_NOWAIT) != 0)
    ;  // do nothing
}

auto OpenFileAsync(std::string path, const int flags, const int mode, FileOpenedCallback on_success,
                   OnErrorCallback on_error, OnFinishedCallback on_finished) -> bool {
  ASSERT(!path.empty());
  auto request = new fs::OpenFileRequest(std::move(path), flags, mode, std::move(on_success), std::move(on_error),
                                         std::move(on_finished));
  ASSERT(request);
  return GetThreadEventLoop()->Submit(request) == 0;
}

namespace fs {
#define FS_REQUEST_CALL_UV(Name, Func, ...) \
  Func(loop->Get(), handle(), GetPath().c_str() __VA_OPT__(, ) __VA_ARGS__, &On##Name)

#define FS_REQUEST_CALL_F(Name, Func, ...)              \
  auto Name::GetRequestName() const -> const char* {    \
    return #Func;                                       \
  }                                                     \
  FS_REQUEST_EXECUTE_F(Name) {                          \
    ASSERT(loop);                                       \
    return FS_REQUEST_CALL_UV(Name, Func, __VA_ARGS__); \
  }

#define FS_REQUEST_SIMPLE_CALLBACK_F(Name)                                                                           \
  FS_REQUEST_CALLBACK_F(Name) {                                                                                      \
    ASSERT(handle);                                                                                                  \
    const auto request = From<Name>(handle);                                                                         \
    ASSERT(request);                                                                                                 \
    const auto result = request->GetResult();                                                                        \
    if (result < 0) {                                                                                                \
      const auto request_name = request->GetRequestName();                                                           \
      const auto error_message = std::string(uv_strerror(static_cast<int>(result)));                                 \
      const auto message = fmt::format("{} error for file {}: {}", request_name, request->GetPath(), error_message); \
      request->OnError(Error::New(message));                                                                         \
    } else {                                                                                                         \
      request->OnSuccess();                                                                                          \
    }                                                                                                                \
    uv_fs_req_cleanup(handle);                                                                                       \
    request->OnFinished();                                                                                           \
  }

FS_REQUEST_CALL_F(RenameRequest, uv_fs_rename, GetNewPath().c_str());
FS_REQUEST_SIMPLE_CALLBACK_F(RenameRequest);

FS_REQUEST_CALL_F(RmdirRequest, uv_fs_rmdir);
FS_REQUEST_SIMPLE_CALLBACK_F(RmdirRequest);

FS_REQUEST_CALL_F(MkdirRequest, uv_fs_mkdir, GetMode());
FS_REQUEST_SIMPLE_CALLBACK_F(MkdirRequest);

FS_REQUEST_CALL_F(StatRequest, uv_fs_stat);
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

FS_REQUEST_CALL_F(OpenFileRequest, uv_fs_open, GetFlags(), GetMode());
FS_REQUEST_CALLBACK_F(OpenFileRequest) {
  ASSERT(handle);
  const auto request =
      From<OpenFileRequest>(handle);  // TODO: *urgent* *memory leak* request is allocated but never freed
  ASSERT(request);
  const auto result = request->handle()->result;
  if (result < 0) {
    const auto message =
        fmt::format("error reading stats of file {}: {}", request->GetPath(), uv_strerror(static_cast<int>(result)));
    return request->OnError(Error::New(message));
  }
  request->OnNext(Long::New(static_cast<RawLong>(result)));
  uv_fs_req_cleanup(handle);
  request->OnFinished();
}
}  // namespace fs
}  // namespace gel
