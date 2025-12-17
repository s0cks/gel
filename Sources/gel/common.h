#ifndef GEL_COMMON_H
#define GEL_COMMON_H

#include <chrono>
#include <concepts>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <glog/logging.h>
#include <optional>
#include <ostream>
#include <ranges>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>
#include <units.h>

#include "gel/platform.h" // IWYU pragma: expor
#include "gel/assert.h"

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

#define DEFINE_NON_INSTANTIABLE_TYPE(Name) \
  DEFINE_NON_COPYABLE_TYPE(Name);          \
                                           \
 public:                                   \
  Name() = delete;                         \
  ~Name() = delete;

#if defined(_MSC_VER)
#define GEL_PRETTY_FUNC_NAME __FUNCSIG__
#elif defined(__clang__) || defined(__GNUC__)
#define GEL_PRETTY_FUNC_NAME __PRETTY_FUNCTION__
#else
#define GEL_PRETTY_FUNC_NAME __FUNCTION__
#endif  // NOT_IMPLEMENTED

#define NOT_IMPLEMENTED(Level) LOG(Level) << GEL_PRETTY_FUNC_NAME << " is not implemented!"
#define GEL_VLEVEL_1           1
#define GEL_VLEVEL_2           2
#define GEL_VLEVEL_3           3

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

#define DECLARE_VISITOR_WRAPPER(Name, Type)               \
  class Name##VisitorWrapper : public Type##Visitor {     \
    using Callback = std::function<bool(Type*)>;          \
    DEFINE_NON_COPYABLE_TYPE(Name##VisitorWrapper);       \
                                                          \
   private:                                               \
    Callback delegate_;                                   \
                                                          \
   public:                                                \
    Name##VisitorWrapper(Callback delegate) :             \
      Name##Visitor(),                                    \
      delegate_(std::move(delegate)) {}                   \
    ~Name##VisitorWrapper() override = default;           \
    inline auto Visit##Type(Type* ptr) -> bool override { \
      return delegate_(ptr);                              \
    }                                                     \
    inline auto operator()(Type* ptr) -> bool {           \
      return Visit##Type(ptr);                            \
    }                                                     \
  };
#define DECLARE_VISITOR(Type)                          \
  class Type##Visitor {                                \
    DEFINE_NON_COPYABLE_TYPE(Type##Visitor);           \
                                                       \
   protected:                                          \
    Type##Visitor() = default;                         \
                                                       \
   public:                                             \
    virtual ~Type##Visitor() = default;                \
    virtual auto Visit##Type(Type* value) -> bool = 0; \
  };                                                   \
  DECLARE_VISITOR_WRAPPER(Type, Type);

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
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    snprintf((char*)&kFormatBuffer[0], kFormattedLength, "%.2f%%", rhs.value);
    return stream << kFormatBuffer;
  }

  DEFINE_DEFAULT_COPYABLE_TYPE(Percent);
};

using Clock = std::chrono::high_resolution_clock;

template <typename R>
static inline auto TimedExecution(std::function<R()> func) -> std::pair<R, Clock::duration> {
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

  auto path() const -> std::optional<std::filesystem::path> {
    const auto value = getenv(name_.data());
    return value ? std::optional<std::filesystem::path>{value} : std::nullopt;
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

static inline auto bytes(const uword nbytes) -> units::data::bytes<double> {
  return units::data::bytes<double>(static_cast<double>(nbytes));
}

static inline auto PrettyPrintBytes(const uword num_bytes) -> std::string {
  static constexpr const auto kScale = 1024;

  std::stringstream ss;
  int scale = 0;
  uword remaining = num_bytes;
  while (remaining >= kScale) {
    remaining /= kScale;
    scale += 1;
  }
  switch (scale) {
    case 1:
      ss << units::data::kilobytes<double>(static_cast<double>(remaining));
      break;
    case 2:
      ss << units::data::megabytes<double>(static_cast<double>(remaining));
      break;
    case 3:
      ss << units::data::gigabytes<double>(static_cast<double>(remaining));
      break;
    case 4:
      ss << units::data::terabytes<double>(static_cast<double>(remaining));
      break;
    case 5:  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
      ss << units::data::petabytes<double>(static_cast<double>(remaining));
      break;
    case 0:
    default:
      ss << units::data::bytes(static_cast<double>(remaining));
      break;
  }
  return ss.str();
}

class ostream_guard {
  DEFINE_NON_COPYABLE_TYPE(ostream_guard);

 private:
  std::ostream& stream_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  std::ostream::fmtflags flags_;
  std::streamsize precision_;
  char fill_;

 public:
  explicit ostream_guard(std::ostream& stream) :
    stream_(stream),
    flags_(stream.flags()),
    precision_(stream.precision()),
    fill_(stream.fill()) {}
  ~ostream_guard() {
    stream().flags(flags_);
    stream().fill(fill_);
    stream().precision(precision_);
  }

  inline auto stream() const -> std::ostream& {
    return stream_;
  }

  inline constexpr auto flags() const -> std::ostream::fmtflags {
    return flags_;
  }

  inline constexpr auto fill() const -> char {
    return fill_;
  }

  inline constexpr auto precsion() const -> std::streamsize {
    return precision_;
  }
};

template <class V, typename T>
concept Visitor = requires(V vis, T value) {
  { vis.Visit(value) } -> std::convertible_to<bool>;
};

template <typename V, Visitor<V> Visitor>
static inline auto Visit(V value, Visitor& vis) -> bool {
  return value && vis.Visit(value);
}

template <typename V, std::predicate<V> Visitor>
static inline auto Visit(V value, Visitor& vis) -> bool {
  return value && vis(value);
}

template <std::ranges::range R, std::predicate<std::ranges::range_value_t<R>> Visitor>
static inline auto VisitAll(const R& range, Visitor& vis) -> bool {
  for (const auto& v : range) {
    if (!vis(v))
      return false;
  }
  return true;
}

template <std::ranges::range R, Visitor<std::ranges::range_value_t<R>> Visitor>
static inline auto VisitAll(const R& range, Visitor& vis) -> bool {
  for (const auto& v : range) {
    if (!vis.Visit(v))
      return false;
  }
  return true;
}

template <typename T, typename V>
concept VisitorLike = Visitor<T, V> || std::predicate<T, V>;
}  // namespace gel

#endif  // GEL_COMMON_H
