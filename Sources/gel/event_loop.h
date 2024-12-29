#ifndef GEL_EVENT_LOOP_H
#define GEL_EVENT_LOOP_H

#include <uv.h>

#include <string>

#include "gel/common.h"
#include "gel/natives.h"
#include "gel/object.h"

namespace gel {
// TODO: move to async namespace
using OnSuccessCallback = std::function<void()>;
using OnErrorCallback = std::function<void(Error*)>;
using OnFinishedCallback = std::function<void()>;

namespace fs {
class Request;
using RequestCallback = std::function<void(Request*)>;
}  // namespace fs

class Timer;
class EventLoop : public Object {
 private:
  uv_loop_t* loop_;
  std::vector<Timer*> timers_{};

  explicit EventLoop(uv_loop_t* loop) :
    Object(),
    loop_(loop) {
    ASSERT(loop_);
    SetData(this);
  }

  void SetData(void* data) {
    ASSERT(loop_);
    ASSERT(data);
    uv_loop_set_data(Get(), data);
  }

 public:
  ~EventLoop() override = default;

  auto Get() const -> uv_loop_t* {
    return loop_;
  }

  auto GetData() const -> void* {
    ASSERT(loop_);
    return uv_loop_get_data(Get());
  }

  auto timers() const -> const std::vector<Timer*>& {
    return timers_;
  }

  auto Stat(const std::string& path, const std::function<void(uword)>& on_next, const OnErrorCallback& on_error = {},
            const OnFinishedCallback& on_finished = {}) -> bool;
  auto Stat(const std::string& path, Procedure* on_next, Procedure* on_error, Procedure* on_finished) -> bool;

  auto Rename(const std::string& old_path, const std::string& new_path, const OnSuccessCallback& on_success = {},
              const OnErrorCallback& on_error = {}, const OnFinishedCallback& on_finished = {}) -> bool;
  auto Rename(const std::string& old_path, const std::string& new_path, Procedure* on_success, Procedure* on_error,
              Procedure* on_finished) -> bool;

  auto Mkdir(const std::string& path, const int mode, const OnSuccessCallback& on_success = {},
             const OnErrorCallback& on_error = {}, const OnFinishedCallback& on_finished = {}) -> bool;
  auto Mkdir(const std::string& path, const int mode, Procedure* on_success, Procedure* on_error, Procedure* on_finished) -> bool;

  auto Rmdir(const std::string& path, const OnSuccessCallback& on_success = {}, const OnErrorCallback& on_error = {},
             const OnFinishedCallback& on_finished = {}) -> bool;
  auto Rmdir(const std::string& path, Procedure* on_success, Procedure* on_error, Procedure* on_finished) -> bool;

  auto Open(const std::string& path, const int flags, const int mode, const OnSuccessCallback& on_success = {},
            const OnErrorCallback& on_error = {}, const OnFinishedCallback& on_finished = {}) -> bool;
  auto Open(const std::string& path, const int flags, const int mode, Procedure* on_success, Procedure* on_error,
            Procedure* on_finished) -> bool;

  auto GetTimer(const uword idx) const -> Timer*;
  auto Run(const uv_run_mode mode) -> int;
  auto CreateTimer(Procedure* on_tick) -> Timer*;

  friend auto operator<<(std::ostream& stream, const EventLoop& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }

  DECLARE_TYPE(EventLoop);

 public:
  static void Init();
  static inline auto New(uv_loop_t* loop = uv_loop_new()) -> EventLoop* {
    ASSERT(loop);
    return new EventLoop(loop);
  }
};

auto GetThreadEventLoop() -> EventLoop*;

namespace fs {
class RequestBase {
  friend class EventLoop;
  DEFINE_NON_COPYABLE_TYPE(RequestBase);

 private:
  std::string path_;
  uv_fs_t handle_;
  OnErrorCallback on_error_;
  OnFinishedCallback on_finished_;

 protected:
  explicit RequestBase(const std::string& path, const OnErrorCallback& on_error, const OnFinishedCallback& on_finished) :
    path_(path),
    handle_(),
    on_error_(on_error),
    on_finished_(on_finished) {
    SetData(this);
  }

  auto handle() -> uv_fs_t* {
    return &handle_;
  }

  inline void SetData(void* data) {
    ASSERT(data);
    uv_handle_set_data((uv_handle_t*)handle(), this);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline void OnError(Error* error) {
    ASSERT(error);
    if (on_error_)
      return on_error_(error);
  }

  inline void OnFinished() {
    if (on_finished_)
      return on_finished_();
  }

  virtual auto Execute(EventLoop* loop) -> int = 0;

 public:
  virtual ~RequestBase() = default;
  virtual auto GetName() const -> const char* = 0;
  virtual auto GetRequestName() const -> const char* = 0;

  auto GetPath() const -> const std::string& {
    return path_;
  }

  auto handle() const -> const uv_fs_t& {
    return handle_;
  }

  auto GetData() const -> void* {
    return uv_handle_get_data((uv_handle_t*)&handle());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto GetResult() const -> word {
    return uv_fs_get_result(&handle());
  }

 protected:
  template <class R>
  static inline auto From(const uv_fs_t* handle) -> R* {
    return (R*)uv_handle_get_data((uv_handle_t*)handle);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }
};

template <typename Next>
class TemplateRequest : public RequestBase {
  DEFINE_NON_COPYABLE_TYPE(TemplateRequest<Next>);

 public:
  using OnNextCallback = std::function<void(const Next&)>;

 private:
  OnNextCallback on_next_;

 protected:
  inline void OnNext(const Next& value) {
    return on_next_(value);
  }

 public:
  TemplateRequest(const std::string& path, const OnNextCallback& on_next, const OnErrorCallback& on_error,
                  const OnFinishedCallback& on_finished) :
    RequestBase(path, on_error, on_finished),
    on_next_(on_next) {}
  ~TemplateRequest() override = default;
};

#define DECLARE_FS_REQUEST_TYPE(Name)                  \
  friend class gel::EventLoop;                         \
  DEFINE_NON_COPYABLE_TYPE(Name);                      \
                                                       \
 private:                                              \
  static void On##Name(uv_fs_t* handle);               \
                                                       \
 protected:                                            \
  auto Execute(EventLoop* loop) -> int override;       \
                                                       \
 public:                                               \
  auto GetRequestName() const -> const char* override; \
  auto GetName() const -> const char* override {       \
    return #Name;                                      \
  }

#define FS_REQUEST_CALLBACK_F(Name) void Name::On##Name(uv_fs_t* handle)
#define FS_REQUEST_EXECUTE_F(Name)  auto Name::Execute(EventLoop* loop) -> int

class SimpleRequest : public RequestBase {
  DEFINE_NON_COPYABLE_TYPE(SimpleRequest);

 private:
  OnSuccessCallback on_success_;

 protected:
  SimpleRequest(const std::string& path, const OnSuccessCallback& on_success, const OnErrorCallback& on_error,
                const OnFinishedCallback& on_finished) :
    RequestBase(path, on_error, on_finished),
    on_success_(on_success) {}

  inline void OnSuccess() {
    if (on_success_)
      return on_success_();
  }

 public:
  ~SimpleRequest() override = default;
};

class MkdirRequest : public SimpleRequest {
 private:
  int mode_;

  MkdirRequest(const std::string& path, const int mode, const OnSuccessCallback& on_success, const OnErrorCallback& on_error,
               const OnFinishedCallback& on_finished) :
    SimpleRequest(path, on_success, on_error, on_finished),
    mode_(mode) {}

 public:
  ~MkdirRequest() override = default;

  auto GetMode() const -> int {
    return mode_;
  }

  DECLARE_FS_REQUEST_TYPE(MkdirRequest);
};

class RmdirRequest : public SimpleRequest {
 private:
  RmdirRequest(const std::string& path, const OnSuccessCallback& on_success, const OnErrorCallback& on_error,
               const OnFinishedCallback& on_finished) :
    SimpleRequest(path, on_success, on_error, on_finished) {}

 public:
  ~RmdirRequest() override = default;
  DECLARE_FS_REQUEST_TYPE(RmdirRequest);
};

class RenameRequest : public SimpleRequest {
 private:
  std::string new_path_;

  RenameRequest(const std::string& old_path, const std::string& new_path, const OnSuccessCallback on_success,
                const OnErrorCallback& on_error, const OnFinishedCallback& on_finished) :
    SimpleRequest(old_path, on_success, on_error, on_finished),
    new_path_(new_path) {}

 public:
  ~RenameRequest() override = default;

  auto GetNewPath() const -> const std::string& {
    return new_path_;
  }

  DECLARE_FS_REQUEST_TYPE(RenameRequest);
};

class StatRequest : public TemplateRequest<uword> {
 private:
  StatRequest(const std::string& path, const OnNextCallback& on_next, const OnErrorCallback& on_error = {},
              const OnFinishedCallback& on_finished = {}) :
    TemplateRequest<uword>(path, on_next, on_error, on_finished) {}

 public:
  ~StatRequest() override = default;
  DECLARE_FS_REQUEST_TYPE(StatRequest);
};

class OpenRequest : public SimpleRequest {
 private:
  int flags_;
  int mode_;

  OpenRequest(const std::string& path, const int flags, const int mode, const OnSuccessCallback& on_success,
              const OnErrorCallback& on_error, const OnSuccessCallback& on_finished) :
    SimpleRequest(path, on_success, on_error, on_finished),
    flags_(flags),
    mode_(mode) {}

 public:
  ~OpenRequest() override = default;

  auto GetFlags() const -> int {
    return flags_;
  }

  auto GetMode() const -> int {
    return mode_;
  }

  DECLARE_FS_REQUEST_TYPE(OpenRequest);
};
}  // namespace fs

class Timer : public Object {
  friend class EventLoop;
  friend class proc::timer_set_repeat;

 private:
  static void OnTick(uv_timer_t* handle);

 private:
  uword id_;
  uv_timer_t handle_{};
  Procedure* on_tick_;

  Timer(uword id, Procedure* on_tick) :
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

#endif  // GEL_EVENT_LOOP_H
