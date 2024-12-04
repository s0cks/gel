#ifndef GEL_TO_STRING_HELPER_H
#define GEL_TO_STRING_HELPER_H

#include <glog/logging.h>

#include <set>
#include <string>
#include <type_traits>

#include "gel/common.h"
#include "gel/expression.h"
#include "gel/instruction.h"
#include "gel/object.h"

#if defined(__clang__)
#include <cxxabi.h>
#endif

namespace std {
template <typename T>
struct has_to_string {
  static constexpr const auto value = false;
};

#define DECLARE_HAS_TO_STRING(Name)           \
  template <>                                 \
  struct has_to_string<gel::Name> {           \
    static constexpr const auto value = true; \
  };
FOR_EACH_TYPE(DECLARE_HAS_TO_STRING)
DECLARE_HAS_TO_STRING(Expression);
DECLARE_HAS_TO_STRING(Datum);
DECLARE_HAS_TO_STRING(Object);
DECLARE_HAS_TO_STRING(LocalScope);
DECLARE_HAS_TO_STRING(instr::Definition);
DECLARE_HAS_TO_STRING(instr::EntryInstr);
DECLARE_HAS_TO_STRING(instr::Instruction);
FOR_EACH_INSTRUCTION(DECLARE_HAS_TO_STRING);
#undef DECLARE_HAS_TO_STRING

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
    explicit Field(const std::string& name, const std::string& value = "") :
      name_(name),
      value_(value) {}
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
  virtual auto GetEncosingStyle() const -> EnclosingStyle = 0;
  virtual auto GetValueSeparatorStyle() const -> ValueSeparatorStyle = 0;
  virtual auto GetFieldSeparatorStyle() const -> FieldSeparatorStyle = 0;

 public:
  virtual ~ToStringHelperBase() = default;

  auto GetFields() const -> const FieldList& {
    return fields_;
  }

  virtual void AddField(const std::string& name, const std::string& value) {
    ASSERT(!name.empty());
    fields_.emplace_back(name, value);
  }

  void AddField(const std::string& name, const gel::Object* value);
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

  auto GetEncosingStyle() const -> EnclosingStyle override {
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

  void AddField(const std::string& name, const std::string& value) override {
    return ToStringHelperBase::AddField(name, value);
  }

  template <typename V>
  void AddField(const std::string& name, const V& value, std::enable_if_t<std::is_container<V>::value, V>) {
    ASSERT(!name.empty());
    std::stringstream ss;
    ss << value;
    return ToStringHelperBase::AddField(name, ss.str());
  }

  template <typename V>
  void AddField(const std::string& name, const V& value,
                std::enable_if_t<std::is_pointer_v<V> && std::has_to_string<std::remove_pointer_t<V>>::value>* = nullptr) {
    ASSERT(!name.empty());
    if (!value)
      return ToStringHelperBase::AddField(name, "");
    std::stringstream ss;
    ss << value->ToString();
    return ToStringHelperBase::AddField(name, ss.str());
  }

  template <typename V>
  void AddField(const std::string& name, const V& value, std::enable_if_t<!std::is_pointer_v<V>>* = nullptr) {
    ASSERT(!name.empty());
    std::stringstream ss;
    ss << value;
    return ToStringHelperBase::AddField(name, ss.str());
  }

  void AddField(const std::string& name, const bool value) {
    ASSERT(!name.empty());
    return value ? AddField(name, "true") : AddField(name, "false");
  }

  operator std::string() const {
    return ToStringHelperBase::ToString();
  }
};
}  // namespace tostring

using tostring::ToStringHelper;
}  // namespace gel

#endif  // GEL_TO_STRING_HELPER_H
