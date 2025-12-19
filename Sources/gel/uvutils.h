#ifndef GEL_UV_H
#define GEL_UV_H

#include <string>
#include <uv.h>

#include "common.h"

namespace gel::uv {
#ifndef UV_OK
#define UV_OK 0
#endif  // UV_OK

class Status {
  using RawType = int;
  DEFINE_DEFAULT_COPYABLE_TYPE(Status);

 private:
  RawType value_;

 public:
  constexpr Status(const RawType value) :
    value_(value) {}
  ~Status() = default;

  auto GetMessage() const -> std::string {
    return uv_strerror(value());
  }

  constexpr auto value() const -> RawType {
    return value_;
  }

  constexpr auto IsOk() const -> bool {
    return value() == UV_OK;
  }

  constexpr operator RawType() const {
    return value();
  }

  constexpr auto operator==(const Status& rhs) const -> bool {
    return operator==(rhs.value());
  }

  constexpr auto operator==(const RawType rhs) const -> bool {
    return value() == rhs;
  }

  constexpr auto operator!=(const Status& rhs) const -> bool {
    return operator!=(rhs.value());
  }

  constexpr auto operator!=(const RawType rhs) const -> bool {
    return value() != rhs;
  }
};
}  // namespace gel::uv

#endif  // GEL_UV_H
