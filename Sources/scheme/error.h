#ifndef SCM_ERROR_H
#define SCM_ERROR_H

#include <string>

#include "scheme/type.h"

namespace scm {
class Error : public Datum {
 private:
  String* message_;

 public:
  explicit Error(String* message) :
    Datum(),
    message_(message) {}

 public:
  auto GetMessage() const -> String* {
    return message_;
  }

  DECLARE_TYPE(Error);

 public:
  static inline auto New(String* message) -> Error* {
    return new Error(message);
  }

  static inline auto New(const std::string& message) -> Error* {
    return new Error(String::New(message));
  }

  static inline auto New(Type* rhs) -> Error* {
    ASSERT(rhs && rhs->IsString());
    return New(rhs->AsString());
  }
};
}  // namespace scm

#endif  // SCM_ERROR_H
