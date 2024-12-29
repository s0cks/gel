#ifndef GEL_COMMON_H
#define GEL_COMMON_H

#include <glog/logging.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <ranges>
#include <unordered_set>

#include "gel/platform.h"
#ifdef GEL_DEBUG

#include <cassert>
#define ASSERT(x) assert(x);

#else

#define ASSERT(x)

#endif  // GEL_DEBUG

#ifdef __cplusplus

#define GEL_EXTERN extern "C"

#else

#define GEL_EXTERN

#endif  // GEL_EXTERN

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

#ifdef _MSC_VER
#define NOT_IMPLEMENTED(Level) LOG(Level) << __FUNCSIG__ << " is not implemented!"
#elif defined(__clang__) || defined(__GNUC__)
#define NOT_IMPLEMENTED(Level) LOG(Level) << __PRETTY_FUNCTION__ << " is not implemented!"
#else
#define NOT_IMPLEMENTED(Level) LOG(Level) << __FUNCTION__ << " is not implemented!"
#endif  // NOT_IMPLEMENTED

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

static inline void Split(const std::string& str, const char delimiter, std::vector<std::string>& results) {
  std::string current;
  current.reserve(str.size());
  for (const auto& c : str) {
    if (c == delimiter) {
      if (current.empty())
        continue;
      results.push_back(current);
      current.clear();
      continue;
    }
    current += c;
  }
  if (!current.empty())
    results.push_back(current);
}

static inline void Split(const std::string& str, const char delimiter, std::unordered_set<std::string>& results) {
  std::string current;
  current.reserve(str.size());
  for (const auto& c : str) {
    if (c == delimiter) {
      if (current.empty())
        continue;
      results.insert(current);
      current.clear();
      continue;
    }
    current += c;
  }
  if (!current.empty())
    results.insert(current);
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

class EnvironmentVariable {
  DEFINE_DEFAULT_COPYABLE_TYPE(EnvironmentVariable);

 private:
  std::string name_;

 public:
  explicit EnvironmentVariable(const std::string& name) :
    name_(name) {}
  ~EnvironmentVariable() = default;

  auto name() const -> const std::string& {
    return name_;
  }

  auto exists() const -> bool {
    return getenv(name_.data()) != nullptr;
  }

  auto value() const -> std::optional<std::string> {
    const auto value = getenv(name_.data());
    return value ? std::optional<std::string>{{value}} : std::nullopt;
  }

  operator bool() const {
    return exists();
  }

  explicit operator std::string() const {
    return value().value_or(std::string{});
  }

  friend auto operator<<(std::ostream& stream, const EnvironmentVariable& rhs) -> std::ostream& {
    // TODO: use ToStringHelper
    stream << "EnvironmentVariable(";
    stream << "name=" << rhs.name();
    const auto value = rhs.value();
    if (value)
      stream << "value=" << (*value);
    stream << ")";
    return stream;
  }
};

auto GetHomeEnvVar() -> const EnvironmentVariable&;

static inline auto GetFilename(const std::filesystem::path& p) -> std::string {
  const auto& filename = p.filename().string();
  const auto dotpos = filename.find_last_of('.');
  if (dotpos == std::string::npos)
    return filename;
  return filename.substr(0, filename.length() - (filename.length() - dotpos));
}

static inline auto Contains(const std::string& value, const char c) -> bool {
  const auto pos = value.find(c);
  return pos != std::string::npos;
}

#ifdef GEL_DEBUG

#define TIMER_START                   \
  const auto start_ns = Clock::now(); \
  {
#define TIMER_STOP(Result)           \
  }                                  \
  const auto stop_ns = Clock::now(); \
  const auto Result = std::chrono::duration_cast<std::chrono::nanoseconds>(stop_ns - start_ns).count();

#else

#define TIMER_START
#define TIMER_STOP(Result)

#endif  // GEL_DEBUG
}  // namespace gel

#endif  // GEL_COMMON_H