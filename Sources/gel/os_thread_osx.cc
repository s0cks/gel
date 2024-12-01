#include "gel/os_thread.h"
#ifdef OS_IS_OSX

#include <fmt/format.h>
#include <glog/logging.h>
#include <pthread.h>
#include <pthread_spis.h>

#include <utility>

namespace gel {
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
  static std::array<char, kThreadNameMaxLength> kThreadNameBuffer{};
  snprintf(&kThreadNameBuffer[0], kThreadNameMaxLength - 1, "%s", name);  // NOLINT(cppcoreguidelines-pro-type-vararg)
  const pthread_status status = pthread_setname_np((const char*)&kThreadNameBuffer[0]);
  LOG_IF(ERROR, !status) << "couldn't set thread name: " << status;
  return status;
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
  static std::array<char, kThreadMaxResultLength> kThreadResultBuffer{};
  const pthread_status status =
      pthread_join(thread, (void**)&kThreadResultBuffer[0]);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  if (!status) {
    LOG(ERROR) << "couldn't join thread: " << status;
    return false;
  }
  VLOG(3) << thread_name << " thread finished w/ result: " << std::string(&kThreadResultBuffer[0], kThreadMaxResultLength);
  return true;
}

auto ThreadEquals(const ThreadId& lhs, const ThreadId& rhs) -> int {
  return pthread_equal(lhs, rhs);
}

auto GetThreadName(const ThreadId& thread) -> std::string {
  static std::array<char, kThreadNameMaxLength> kThreadNameBuffer{};
  const pthread_status status = pthread_getname_np(thread, &kThreadNameBuffer[0], kThreadNameMaxLength);
  if (!status) {
    LOG(ERROR) << "failed to get name for thread: " << thread;
    return {};
  }
  return {&kThreadNameBuffer[0], kThreadNameMaxLength};
}

auto SetThreadName(const ThreadId& thread, const std::string& name) -> bool {
  static std::array<char, kThreadNameMaxLength> kThreadNameBuffer{};
  snprintf(&kThreadNameBuffer[0], kThreadNameMaxLength - 1, "%s", name.data());  // NOLINT(cppcoreguidelines-pro-type-vararg)
  const pthread_status status = pthread_setname_np(&kThreadNameBuffer[0]);
  LOG_IF(ERROR, !status) << "couldn't set thread name: " << status;
  DLOG_IF(INFO, status) << "set thread name to `" << std::string(&kThreadNameBuffer[0], kThreadNameMaxLength) << "`";
  return status;
}
}  // namespace gel

#endif  // OS_IS_OSX