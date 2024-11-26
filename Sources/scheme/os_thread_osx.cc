#include "scheme/os_thread.h"
#ifdef OS_IS_OSX

#include <fmt/format.h>
#include <glog/logging.h>
#include <pthread.h>
#include <pthread_spis.h>

#include <utility>

namespace scm {
class ThreadStartData {
  DEFINE_NON_COPYABLE_TYPE(ThreadStartData);

 private:
  std::string name_;
  ThreadHandler handler_;
  void* parameter_;

 public:
  ThreadStartData(std::string name, const ThreadHandler& function, void* parameter) :
    name_(std::move(name)),
    handler_(function),
    parameter_(parameter) {}
  ~ThreadStartData() = default;

  auto GetName() const -> std::string {
    return name_;
  }

  auto GetFunction() -> ThreadHandler& {
    return handler_;
  }

  auto GetFunction() const -> ThreadHandler {
    return handler_;
  }

  auto GetParameter() const -> void* {
    return parameter_;
  }
};

auto SetThreadName(const ThreadId& thread, const char* name) -> bool {
  char truncated_name[kThreadNameMaxLength];
  snprintf(truncated_name, kThreadNameMaxLength - 1, "%s", name);  // NOLINT(cppcoreguidelines-pro-type-vararg)
  int result = -1;
  if ((result = pthread_setname_np(truncated_name)) != 0) {
    LOG(WARNING) << "couldn't set thread name: " << strerror(result);
    return false;
  }
  return true;
}

static auto HandleThread(void* pdata) -> void* {
  auto data = (ThreadStartData*)pdata;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  auto& func = data->GetFunction();
  void* parameter = data->GetParameter();

  if (!SetThreadName(pthread_self(), data->GetName()) != 0)
    goto exit;  // NOLINT(cppcoreguidelines-avoid-goto)
  func(parameter);
exit:
  delete data;
  pthread_exit(nullptr);
}

auto GetCurrentThreadId() -> ThreadId {
  return pthread_self();
}

auto Start(ThreadId* thread, const std::string& name, const ThreadHandler& func, void* parameter) -> bool {
  int result = -1;
  pthread_attr_t attrs;
  if ((result = pthread_attr_init(&attrs)) != 0) {
    LOG(ERROR) << "couldn't initialize the thread attributes: " << strerror(result);
    return false;
  }

  DVLOG(1) << "starting " << name << " thread w/ parameter: " << std::hex << parameter;
  auto data = new ThreadStartData(name, func, parameter);
  if ((result = pthread_create(thread, &attrs, &HandleThread, data)) != 0) {
    LOG(ERROR) << "couldn't start the thread: " << strerror(result);
    return false;
  }

  if ((result = pthread_attr_destroy(&attrs)) != 0) {
    LOG(ERROR) << "couldn't destroy the thread attributes: " << strerror(result);
    return false;
  }
  return true;
}

auto Join(const ThreadId& thread) -> bool {
  std::string thread_name = GetThreadName(thread);

  char return_data[kThreadMaxResultLength];

  int result = -1;
  if ((result = pthread_join(thread, (void**)&return_data)) != 0) {  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    LOG(ERROR) << "couldn't join thread: " << strerror(result);
    return false;
  }

  VLOG(3) << thread_name << " thread finished w/ result: " << std::string(return_data, kThreadMaxResultLength);
  return true;
}

auto ThreadEquals(const ThreadId& lhs, const ThreadId& rhs) -> int {
  return pthread_equal(lhs, rhs);
}

auto GetThreadName(const ThreadId& thread) -> std::string {
  char name[kThreadNameMaxLength];

  int err = -1;
  if ((err = pthread_getname_np(thread, name, kThreadNameMaxLength)) != 0) {
    LOG(ERROR) << "cannot get name for " << thread << " thread: " << strerror(err);
    return "unknown";
  }
  return {name};
}

auto SetThreadName(const ThreadId& thread, const std::string& name) -> bool {
  char truncated_name[kThreadNameMaxLength];
  snprintf(truncated_name, kThreadNameMaxLength - 1, "%s", name.data());  // NOLINT(cppcoreguidelines-pro-type-vararg)
  int result = -1;
  if ((result = pthread_setname_np(truncated_name)) != 0) {
    LOG(WARNING) << "couldn't set thread name: " << strerror(result);
    return false;
  }
  DLOG(INFO) << "set thread #" << thread << " name to: " << name;
  return true;
}
}  // namespace scm

#endif  // OS_IS_OSX