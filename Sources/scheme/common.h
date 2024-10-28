#ifndef SCM_COMMON_H
#define SCM_COMMON_H

#include <glog/logging.h>
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

namespace scm {}  // namespace scm

#endif  // SCM_COMMON_H