
#ifndef GEL_ENABLE_PROFILING
#define GEL_ENABLE_PROFILING 1
#endif  // GEL_ENABLE_PROFILING

#ifdef GEL_ENABLE_PROFILING

#ifndef GEL_PROFILING_H
#define GEL_PROFILING_H

#include <chrono>
#include <string>

#include "common.h"

namespace gel {
class TimeSlice {
  DEFINE_DEFAULT_COPYABLE_TYPE(TimeSlice);

 public:
  using Clock = std::chrono::high_resolution_clock;

 private:
  std::string name_;
  Clock::time_point start_;
  Clock::time_point finished_{};

 public:
  explicit TimeSlice(const std::string name, Clock::time_point start = Clock::now()) :
    name_(std::move(name)),
    start_(start) {}
  ~TimeSlice() {
    finished_ = Clock::now();
    DLOG(INFO) << name() << " finished in " << std::chrono::duration_cast<std::chrono::milliseconds>(duration());
  }

  inline auto name() const -> const std::string& {
    return name_;
  }

  inline auto start() const -> const Clock::time_point& {
    return start_;
  }

  inline auto finished() const -> const Clock::time_point& {
    return finished_;
  }

  inline auto duration() const -> Clock::duration {
    return (finished() - start());
  }
};
}  // namespace gel

#define _GEL_PROFILE(Name) TimeSlice slice(Name)

#define GEL_PROFILE        _GEL_PROFILE(GEL_PRETTY_FUNC_NAME)

#endif  // GEL_PROFILING_H

#else

#define GEL_PROFILE

#endif  // GEL_ENABLE_PROFILING
