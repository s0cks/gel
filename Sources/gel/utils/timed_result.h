
#include <chrono>
#include "gel/common.h"

namespace gel {
struct TimedResult {
  DEFINE_DEFAULT_COPYABLE_TYPE(TimedResult);

 public:
  gel::Object* result;
  Clock::duration duration;

  TimedResult() = default;
  TimedResult(gel::Object* r, const Clock::duration& d) :
    result(r),
    duration(d) {}
  TimedResult(const std::pair<gel::Object*, Clock::duration>& value) :
    result(value.first),
    duration(value.second) {}
  ~TimedResult() = default;

  auto IsError() const -> bool {
    return gel::IsError(result);
  }

  auto IsNull() const -> bool {
    return gel::IsNil(result);
  }

  operator bool() const {
    return !IsError();
  }

  friend auto operator<<(std::ostream& stream, const TimedResult& rhs) -> std::ostream& {
    const auto& result = rhs.result;
    const auto& duration = rhs.duration;
    DVLOG(1) << "finished in " << units::time::nanosecond_t(static_cast<double>(duration.count()));
    if (result->IsNil())
      return stream;
    if (gel::IsError(result))
      return stream << "error: " << ToError(result)->GetMessage()->Get();
    ASSERT(!result->IsNil());
    stream << "result: ";
    PrintValue(stream, result) << std::endl;
    return stream;
  }
};

}
