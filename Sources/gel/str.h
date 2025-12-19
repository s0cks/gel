#ifndef GEL_STR_H
#define GEL_STR_H

#include "object.h"

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

class String : public StringObject {
 protected:
  String() = default;
  explicit String(const std::string& value) :
    StringObject(value) {}

 public:
  ~String() override = default;
  auto Eq(Object* rhs) const -> Object* override;
  auto Equals(const std::string& rhs) const -> bool;

  auto ToBuffer() const -> Buffer*;

  operator std::string() const {
    return Get();
  }

  DECLARE_TYPE(String);

 public:
  inline friend auto operator<<(std::ostream& stream, const String& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }

 public:
  static auto New() -> String*;
  static auto New(Symbol* rhs) -> String*;
  static inline auto New(const std::string& value) -> String* {
    return new String(value);
  }
  static inline auto Unbox(Object* rhs) -> const std::string& {
    ASSERT(rhs && rhs->IsString());
    return rhs->AsString()->Get();
  }

  static auto Empty() -> String*;
  static auto ValueOf(Object* rhs) -> String*;
};
}  // namespace gel

namespace fmt {
template <>
struct formatter<gel::String> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::String& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "\"{}\"", value.Get());
  }
};
}  // namespace fmt

#endif  // GEL_STR_H
