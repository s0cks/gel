#ifndef GEL_NUMBER_H
#define GEL_NUMBER_H

#include "object.h"

namespace gel {
using RawLong = int64_t;

class Number : public Object {
  friend class Long;
  friend class Double;

 private:
  std::variant<RawLong, double> value_;

 protected:
  explicit Number(const RawLong value) :
    Object(),
    value_(value) {}
  explicit Number(const double value) :
    Object(),
    value_(value) {}

 public:
  ~Number() override = default;

  auto value() const -> const std::variant<RawLong, double>& {
    return value_;
  }

  auto GetLong() const -> RawLong {
    if (std::holds_alternative<double>(value()))
      return static_cast<RawLong>(GetDouble());
    return std::get<RawLong>(value());
  }

  auto GetDouble() const -> double {
    if (std::holds_alternative<RawLong>(value()))
      return static_cast<double>(GetLong());
    return std::get<double>(value());
  }

  auto BitNot() const -> Object*;

  DECLARE_TYPE(Number);

 public:
  static auto New(const RawLong rhs) -> Number*;
  static auto New(const double rhs) -> Number*;
};

class Long : public Number {
 protected:
  explicit Long(const RawLong value) :
    Number(value) {}

 public:
  ~Long() override = default;

  inline auto Get() const -> RawLong {
    return GetLong();
  }

  auto Add(Object* rhs) const -> Object* override;
  auto Subtract(Object* rhs) const -> Object* override;
  auto Multiply(Object* rhs) const -> Object* override;
  auto Divide(Object* rhs) const -> Object* override;
  auto Modulus(Object* rhs) const -> Object* override;
  auto BitAnd(Object* rhs) const -> Object* override;
  auto BitOr(Object* rhs) const -> Object* override;
  auto BitXor(Object* rhs) const -> Object* override;
  auto ShiftLeft(Object* rhs) const -> Object* override;
  auto ShiftRight(Object* rhs) const -> Object* override;

  auto Eq(Object* rhs) const -> Object* override;
  auto GreaterThan(Object* rhs) const -> Object* override;
  auto LessThan(Object* rhs) const -> Object* override;
  DECLARE_TYPE(Long);

 public:
  static inline auto New(const RawLong value) -> Long* {
    return new Long(value);
  }

  static auto Unbox(Object* rhs) -> RawLong;
};

class Double : public Number {
 protected:
  Double(const double value) :
    Number(value) {}

 public:
  ~Double() override = default;

  inline auto Get() const -> double {
    return GetDouble();
  }

  auto Add(Object* rhs) const -> Object* override;
  auto Subtract(Object* rhs) const -> Object* override;
  auto Multiply(Object* rhs) const -> Object* override;
  auto Divide(Object* rhs) const -> Object* override;
  DECLARE_TYPE(Double);

 public:
  static inline auto New(const double value) -> Double* {
    return new Double(value);
  }
};
}  // namespace gel

namespace fmt {
template <>
struct formatter<gel::Long> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::Long& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", value.Get());
  }
};

template <>
struct formatter<gel::Double> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const gel::Double& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", value.Get());
  }
};
}  // namespace fmt

#endif  // GEL_NUMBER_H
