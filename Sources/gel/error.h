#ifndef GEL_ERROR_H
#define GEL_ERROR_H

#include <exception>
#include <fmt/base.h>
#include <fmt/format.h>
#include <sstream>
#include <string>

#include "gel/common.h"
#include "gel/object.h"
#include "gel/platform.h"
#include "gel/pointer.h"

namespace gel {
class Error : public Object {
 private:
  uword message_ = UNALLOCATED;

  auto raw_ptr() const -> Pointer** {
    return ((Pointer**)&message_);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

 protected:
  auto VisitPointers(PointerVisitor* vis) -> bool override;
  auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override;

 public:
  explicit Error(String* message) :
    Object() {
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

  static inline auto New(const std::stringstream& ss) -> Error* {
    return New(ss.str());
  }

  static inline auto New(Object* rhs) -> Error* {
    ASSERT(rhs);
    return New(String::ValueOf(rhs));
  }

  static inline auto New(const std::exception& exc) -> Error* {
    return Error::New(exc.what());
  }

  static inline auto New(const std::exception_ptr exc) -> Error* {
    try {
      std::rethrow_exception(exc);
    } catch (const std::exception& e) {
      return New(e);
    }
  }
};
}  // namespace gel

namespace fmt {
template <>
struct formatter<gel::Error> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::Error& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", value.GetMessage()->Get());
  }
};
}  // namespace fmt

#endif  // GEL_ERROR_H
