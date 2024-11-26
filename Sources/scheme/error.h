#ifndef SCM_ERROR_H
#define SCM_ERROR_H

#include <string>

#include "scheme/common.h"
#include "scheme/object.h"
#include "scheme/platform.h"
#include "scheme/pointer.h"

namespace scm {
class Error : public Datum {
 private:
  uword message_ = UNALLOCATED;

  auto raw_ptr() const -> Pointer** {
    return ((Pointer**)&message_);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

 protected:
  auto VisitPointers(PointerVisitor* vis) -> bool override;
  auto VisitPointers(PointerPointerVisitor* vis) -> bool override;

 public:
  explicit Error(String* message) :
    Datum() {
    SetMessage(message);
  }

  void SetMessage(String* rhs) {
    ASSERT(rhs);
    (*raw_ptr()) = rhs->raw_ptr();
  }

 public:
  ~Error() override = default;

  auto GetMessage() const -> String* {
    return (*raw_ptr())->As<String>();
  }

  DECLARE_TYPE(Error);

 public:
  static inline auto New(String* message) -> Error* {
    ASSERT(message);
    return new Error(message);
  }

  static inline auto New(const std::string& message) -> Error* {
    ASSERT(!message.empty());
    return New(String::New(message));
  }

  static inline auto New(Object* rhs) -> Error* {
    ASSERT(rhs && rhs->IsString());
    return New(rhs->AsString());
  }
};
}  // namespace scm

#endif  // SCM_ERROR_H
