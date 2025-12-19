#ifndef GEL_TO_STRING_HELPER_H
#define GEL_TO_STRING_HELPER_H

#include <concepts>
#include <cstdlib>
#include <glog/logging.h>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "common.h"
#include "compiled_code.h"
#include "expr/expression.h"
#include "instruction.h"
#include "local_scope.h"
#include "object.h"
#include "platform.h"
#include "type.h"

#if defined(__clang__)
#include <cxxabi.h>
#endif

namespace std {
template <typename T>
struct is_container {
  static constexpr const auto value = false;
};

template <typename T, typename Alloc>
struct is_container<std::vector<T, Alloc>> {
  static constexpr const auto value = true;
};

template <typename T, typename Compare, typename Alloc>
struct is_container<std::set<T, Compare, Alloc>> {
  static constexpr const auto value = true;
};
}  // namespace std

namespace gel {

template <class T>
concept HasToString = requires(T value) {
  { value.ToString() } -> std::convertible_to<std::string>;
};

class Object;
namespace tostring {
enum EnclosingStyle {
  kParenthesis,
  kBraces,
  kBrackets,
};

static inline constexpr auto GetChar(const EnclosingStyle rhs, const bool open = false) -> char {
  switch (rhs) {
    case EnclosingStyle::kParenthesis:
      return open ? '(' : ')';
    case EnclosingStyle::kBraces:
      return open ? '{' : '}';
    case EnclosingStyle::kBrackets:
      return open ? '[' : ']';
    default:
      LOG(FATAL) << "invalid Style: " << rhs;
  }
}

enum ValueSeparatorStyle {
  kValueSepColon,
  kValueSepEquals,
  kValueSepSpace,
};

static inline constexpr auto GetChar(const ValueSeparatorStyle rhs) -> char {
  switch (rhs) {
    case ValueSeparatorStyle::kValueSepColon:
      return ':';
    case ValueSeparatorStyle::kValueSepEquals:
      return '=';
    case ValueSeparatorStyle::kValueSepSpace:
      return ' ';
    default:
      LOG(FATAL) << "invalid ValueSeparatorStyle: " << rhs;
  }
}

static inline auto operator<<(std::ostream& stream, const ValueSeparatorStyle& rhs) -> std::ostream& {
  return stream << GetChar(rhs);
}

enum FieldSeparatorStyle {
  kSpace,
  kComma,
  kColon,
  kSemicolon,
};

static inline constexpr auto GetChar(const FieldSeparatorStyle rhs) -> char {
  switch (rhs) {
    case FieldSeparatorStyle::kSpace:
      return ' ';
    case FieldSeparatorStyle::kComma:
      return ',';
    case FieldSeparatorStyle::kColon:
      return ':';
    case FieldSeparatorStyle::kSemicolon:
      return ';';
    default:
      LOG(FATAL) << "invalid Style: " << rhs;
  }
}

class ToStringHelperBase {
  DEFINE_NON_COPYABLE_TYPE(ToStringHelperBase);

 private:
  struct Field {
    DEFINE_DEFAULT_COPYABLE_TYPE(Field);

   private:
    std::string name_;
    std::string value_;

   public:
    explicit Field(const std::string_view name, const std::string value = "") :
      name_(std::move(name)),
      value_(std::move(value)) {}
    ~Field() = default;

    auto name() const -> const std::string& {
      return name_;
    }

    auto value() const -> const std::string& {
      return value_;
    }
  };

  using FieldList = std::vector<Field>;

 private:
  FieldList fields_{};

 protected:
  ToStringHelperBase() = default;
  virtual auto GetTypename() const -> std::string = 0;
  virtual auto GetEnclosingStyle() const -> EnclosingStyle = 0;
  virtual auto GetValueSeparatorStyle() const -> ValueSeparatorStyle = 0;
  virtual auto GetFieldSeparatorStyle() const -> FieldSeparatorStyle = 0;

 public:
  virtual ~ToStringHelperBase() = default;

  auto GetFields() const -> const FieldList& {
    return fields_;
  }

  template <typename V>
  inline void AddField(const std::string_view name, V value) {
    std::stringstream ss;
    ss << value;
    return AddField(std::move(name), ss.str());
  }

  template <HasToString V>
  inline void AddField(const std::string_view name, V* value) {
    return value ? AddField(std::move(name), value->ToString()) : AddField(std::move(name), "null");
  }

  template <HasToString V>
  inline void AddField(const std::string_view name, const V& value) {
    return AddField(std::move(name), value.ToString());
  }

  template <HasToString V>
  inline void AddField(const std::string_view name, const std::vector<V>& value) {
    std::stringstream ss;
    ss << "[";
    for (auto idx = 0; idx < value.size(); idx++) {
      ss << value[idx]->ToString();
      if (idx < (value.size() - 1))
        ss << ", ";
    }
    ss << "]";
    return AddField(std::move(name), value.ToString());
  }

  template <std::ranges::range R>
  inline void AddArrayField(const std::string_view name, const R& range,
                            const ValueSeparatorStyle separator = ValueSeparatorStyle::kValueSepColon) {
    uword num_remaining = range.size();
    std::stringstream ss{};
    ss << "[";
    for (const auto& v : range) {
      ss << v;
      if (--num_remaining > 0)
        ss << separator << " ";
    }
    ss << "]";
  }

  template <typename V>
  inline void AddField(const std::string_view name, const V value)
    requires(std::is_integral_v<V>)
  {
    std::stringstream ss;
    ss << value;
    return AddField(std::move(name), ss.str());
  }

  void AddBytesField(const std::string_view name, const uword num_bytes);
  auto ToString() const -> std::string;
};

template <typename T, const EnclosingStyle ES = kParenthesis, const ValueSeparatorStyle VS = kValueSepEquals,
          const FieldSeparatorStyle FS = kComma>
class ToStringHelper : public ToStringHelperBase {
  DEFINE_NON_COPYABLE_TYPE(ToStringHelper);

 private:
  auto GetTypename() const -> std::string override {
    const auto name = typeid(T).name();
#if defined(__clang__)
    int status = 0;
    const auto demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    const auto result = std::string(demangled);
    free(demangled);  // NOLINT(cppcoreguidelines-no-malloc)
    return status == 0 ? result : name;
#else
    return name;
#endif
  }

  auto GetEnclosingStyle() const -> EnclosingStyle override {
    return ES;
  }

  auto GetFieldSeparatorStyle() const -> FieldSeparatorStyle override {
    return FS;
  }

  auto GetValueSeparatorStyle() const -> ValueSeparatorStyle override {
    return VS;
  }

 public:
  ToStringHelper() = default;
  ~ToStringHelper() override = default;

  operator std::string() const {
    return ToStringHelperBase::ToString();
  }

  friend auto operator<<(std::ostream& stream, const ToStringHelper<T, ES, VS, FS>& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }
};

template <>
inline void ToStringHelperBase::AddField<std::string>(const std::string_view name, const std::string value) {
  ASSERT(!name.empty());
  ASSERT(!value.empty());
  fields_.emplace_back(std::move(name), std::move(value));
}

template <>
inline void ToStringHelperBase::AddField<std::string_view>(const std::string_view name, const std::string_view value) {
  return AddField(name, std::string(value));
}

template <>
inline void ToStringHelperBase::AddField<const char*>(const std::string_view name, const char* value) {
  return AddField(name, std::string(value));
}

template <>
inline void ToStringHelperBase::AddField<const void*>(const std::string_view name, const void* value) {
  ASSERT(!name.empty());
  std::stringstream ss;
  ss << value;
  return AddField(name, ss.str());
}

template <>
inline void ToStringHelperBase::AddField<bool>(const std::string_view name, const bool value) {
  return AddField<std::string_view>(name, value ? "true" : "false");
}

// template <HasToString V>
// void AddField(const std::string& name, const V& value) {
//   ASSERT(!name.empty());
//   std::stringstream ss;
//   ss << value;
//   return ToStringHelperBase::AddField(name, ss.str());
// }

// void AddField(const std::string& name, const bool value) {
//   ASSERT(!name.empty());
//   return value ? AddField(name, "true") : AddField(name, "false");
// }

// template <typename V>
// void AddField(const std::string& name, const V& value) {
//   ASSERT(!name.empty());
//   std::stringstream ss;
//   ss << value;
//   return ToStringHelperBase::AddField(name, ss.str());
// }

// template <HasToString V>
// void AddField(const std::string& name, const V* value) {
//   ASSERT(!name.empty());
//   if (!value)
//     return ToStringHelperBase::AddField(name, "");
//   std::stringstream ss;
//   ss << value->ToString();
//   return ToStringHelperBase::AddField(name, ss.str());
// }
}  // namespace tostring

using tostring::ToStringHelper;
}  // namespace gel

#endif  // GEL_TO_STRING_HELPER_H
