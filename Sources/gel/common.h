#ifndef GEL_COMMON_H
#define GEL_COMMON_H

#include <glog/logging.h>

#include <chrono>
#include <cstdio>
#include <functional>

#include "gel/platform.h"
#ifdef GEL_DEBUG

#include <cassert>
#define ASSERT(x) assert(x);

#else

#define ASSERT(x)

#endif  // GEL_DEBUG

#define DEFINE_NON_COPYABLE_TYPE(Name)             \
 public:                                           \
  Name(Name&& rhs) = delete;                       \
  Name(const Name& rhs) = delete;                  \
  auto operator=(const Name& rhs)->Name& = delete; \
  auto operator=(Name&& rhs)->Name& = delete;

#define DEFINE_DEFAULT_COPYABLE_TYPE(Name)          \
 public:                                            \
  Name(Name&& rhs) = default;                       \
  Name(const Name& rhs) = default;                  \
  auto operator=(const Name& rhs)->Name& = default; \
  auto operator=(Name&& rhs)->Name& = default;

#define NOT_IMPLEMENTED(Level) LOG(Level) << __FUNCTION__ << " is not implemented.";

#define GEL_VLEVEL_1 1
#define GEL_VLEVEL_2 2
#define GEL_VLEVEL_3 3

namespace gel {
class Exception : public std::exception {
  DEFINE_DEFAULT_COPYABLE_TYPE(Exception);

 private:
  std::string message_;

 public:
  explicit Exception(std::string message = "") :
    std::exception(),
    message_(std::move(message)) {}
  ~Exception() override = default;

  auto GetMessage() const -> const std::string& {
    return message_;
  }

  auto what() const noexcept -> const char* override {
    return message_.c_str();
  }

  auto operator==(const Exception& rhs) const -> bool {
    return GetMessage() == rhs.GetMessage();
  }

  auto operator!=(const Exception& rhs) const -> bool {
    return GetMessage() != rhs.GetMessage();
  }

  friend auto operator<<(std::ostream& stream, const Exception& rhs) -> std::ostream& {
    stream << "RuntimeException(";
    stream << "message=" << rhs.GetMessage();
    stream << ")";
    return stream;
  }
};

static inline auto RoundUpPow2(word x) -> uword {
  // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
  x = x - 1;
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
#if defined(ARCH_IS_ARM64) || defined(ARCH_IS_X64)
  x = x | (x >> 32);
#endif
  return x + 1;
  // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}

template <typename T>
static inline auto IsPow2(T x) -> bool {
  return ((x & (x - 1)) == 0) && (x != 0);
}

struct Percent {
 private:
  template <typename T>
  static inline constexpr auto CalculatePercentage(const T part, const T whole) -> double {
    return (static_cast<double>(part) * 100.0) / static_cast<double>(whole);
  }

 public:
  double value;

  constexpr explicit Percent(const double val = 0.0) :
    value(val) {}
  template <typename T>
  constexpr explicit Percent(const T part, const T whole) :
    value(CalculatePercentage(part, whole)) {}
  ~Percent() = default;

  friend auto operator<<(std::ostream& stream, const Percent& rhs) -> std::ostream& {
    static constexpr const auto kFormattedLength = 8;
    const auto kFormatBuffer = std::string(kFormattedLength, '\0');
    memset((void*)&kFormatBuffer[0], '\0', kFormattedLength);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,cppcoreguidelines-pro-type-cstyle-cast)
    snprintf((char*)&kFormatBuffer[0], kFormattedLength, "%.2f%%", rhs.value);
    return stream << kFormatBuffer;
  }

  DEFINE_DEFAULT_COPYABLE_TYPE(Percent);
};

using Clock = std::chrono::high_resolution_clock;

template <typename R>
static inline auto TimedExecution(const std::function<R()>& func) -> std::pair<R, Clock::duration> {
  const auto start_ts = Clock::now();
  const auto result = func();
  const auto stop_ts = Clock::now();
  const auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(stop_ts - start_ts);
  return std::make_pair(result, total_ns);
}
}  // namespace gel

#endif  // GEL_COMMON_H