#ifndef SCM_COMMON_H
#define SCM_COMMON_H

#include <glog/logging.h>

#include <chrono>

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

using Clock = std::chrono::high_resolution_clock;
}  // namespace scm

#endif  // SCM_COMMON_H