#ifndef SCM_ERROR_H
#define SCM_ERROR_H

#include <string>

#include "scheme/common.h"
#include "scheme/object.h"

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

  auto GetType() const -> Class* override {
    return GetClass();
  }

  DECLARE_TYPE(Error);

 private:
  static Class* kClass;

 public:
  static void Init();
  static inline auto New(String* message) -> Error* {
    return new Error(message);
  }

  static inline auto New(const std::string& message) -> Error* {
    return new Error(String::New(message));
  }

  static inline auto New(Object* rhs) -> Error* {
    ASSERT(rhs && rhs->IsString());
    return New(rhs->AsString());
  }

  static inline auto GetClass() -> Class* {
    ASSERT(kClass);
    return kClass;
  }
};
}  // namespace scm

#endif  // SCM_ERROR_H
