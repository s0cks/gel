#ifndef SCM_COMMON_H
#define SCM_COMMON_H

#include <glog/logging.h>

#include <chrono>
#include <cstdio>

#include "scheme/platform.h"
#ifdef SCM_DEBUG

#include <cassert>
#define ASSERT(x) assert(x);

#else

#define ASSERT(x)

#endif  // SCM_DEBUG

#define DEFINE_NON_COPYABLE_TYPE(Name)               \
 public:                                             \
  Name(Name&& rhs) = delete;                         \
  Name(const Name& rhs) = delete;                    \
  auto operator=(const Name& rhs) -> Name& = delete; \
  auto operator=(Name&& rhs) -> Name& = delete;

#define DEFINE_DEFAULT_COPYABLE_TYPE(Name)            \
 public:                                              \
  Name(Name&& rhs) = default;                         \
  Name(const Name& rhs) = default;                    \
  auto operator=(const Name& rhs) -> Name& = default; \
  auto operator=(Name&& rhs) -> Name& = default;

#define NOT_IMPLEMENTED(Level) LOG(Level) << __FUNCTION__ << " is not implemented.";

#define SCM_VLEVEL_1 1
#define SCM_VLEVEL_2 2
#define SCM_VLEVEL_3 3

namespace scm {
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

template <typename T>
static inline auto GetPercentageOf(const T part, const T whole) -> double {
  return (static_cast<double>(part) * 100.0) / static_cast<double>(whole);
}

static inline auto RoundUpPow2(word x) -> uword {
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
}

template <typename T>
static inline auto IsPow2(T x) -> bool {
  return ((x & (x - 1)) == 0) && (x != 0);
}

struct Percent {
  double value;

  constexpr explicit Percent(const double val = 0.0) :
    value(val) {}
  ~Percent() = default;

  friend auto operator<<(std::ostream& stream, const Percent& rhs) -> std::ostream& {
    static constexpr const auto kFormattedLength = 8;
    const auto kFormatBuffer = std::string(kFormattedLength, '\0');
    memset((void*)&kFormatBuffer[0], '\0', kFormattedLength);
    snprintf((char*)&kFormatBuffer[0], kFormattedLength, "%.2f%%", rhs.value);
    return stream << kFormatBuffer;
  }

  DEFINE_DEFAULT_COPYABLE_TYPE(Percent);
};

static inline auto PrettyPrintPercent(const double rhs) -> std::string {
  static constexpr const auto kFormattedLength = 8;
  std::string data{};
  data.reserve(kFormattedLength);
  snprintf(&data[0], kFormattedLength, "%.2f%%", rhs);
  return data;
}

template <typename T>
static inline auto PrettyPrintPercent(const T part, const T whole) -> std::string {
  return PrettyPrintPercent(GetPercentageOf(part, whole));
}

using Clock = std::chrono::high_resolution_clock;
}  // namespace scm

#endif  // SCM_COMMON_H