#ifndef GEL_EXCEPTION_H
#define GEL_EXCEPTION_H

#include <string>
#include <utility>

#include "gel/common.h"

namespace gel {
class IllegalArgumentException : public Exception {
  DEFINE_DEFAULT_COPYABLE_TYPE(IllegalArgumentException);

 public:
  explicit IllegalArgumentException(std::string message) :
    Exception(std::move(message)) {}
  ~IllegalArgumentException() override = default;
};
}  // namespace gel

#endif  // GEL_EXCEPTION_H
