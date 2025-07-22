#ifndef GEL_STR_H
#define GEL_STR_H

#include "gel/object.h"

namespace gel {
class StringObject : public Object {
  DEFINE_NON_COPYABLE_TYPE(StringObject);

 private:
  std::string value_;

 protected:
  StringObject() = default;
  explicit StringObject(std::string value) :
    Object(),
    value_(std::move(value)) {}

  inline void Set(const std::string& value) {
    value_ = value;
  }

 public:
  ~StringObject() override = default;

  auto Get() const -> const std::string& {
    return value_;
  }

  constexpr auto GetLength() const -> uword {
    return value_.length();
  }

  inline auto IsEmpty() const -> bool {
    return value_.empty();
  }

  auto GetHashCode() const -> HashCode override;
  auto Equals(Object* rhs) const -> bool override;
  auto Equals(const std::string& rhs) const -> bool;
};

class Str : public StringObject {
 protected:
  Str() = default;
  explicit Str(const std::string& value) :
    StringObject(value) {}

 public:
  ~Str() override = default;
  auto Eq(Object* rhs) const -> Object* override;
  auto Equals(const std::string& rhs) const -> bool;

  auto ToBuffer() const -> Buffer*;

  operator std::string() const {
    return Get();
  }

  DECLARE_TYPE(Str);

 public:
  inline friend auto operator<<(std::ostream& stream, const Str& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }

 public:
  static auto New() -> Str*;
  static auto New(Symbol* rhs) -> Str*;
  static inline auto New(const std::string& value) -> Str* {
    return new Str(value);
  }
  static inline auto Unbox(Object* rhs) -> const std::string& {
    ASSERT(rhs && rhs->IsStr());
    return rhs->AsStr()->Get();
  }

  static auto Empty() -> Str*;
  static auto ValueOf(Object* rhs) -> Str*;
};
}  // namespace gel

namespace fmt {
template <>
struct formatter<gel::Str> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::Str& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "\"{}\"", value.Get());
  }
};
}  // namespace fmt

#endif  // GEL_STR_H
