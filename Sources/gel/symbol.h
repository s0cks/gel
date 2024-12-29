#ifndef GEL_SYMBOL_H
#define GEL_SYMBOL_H

#include "gel/object.h"

namespace gel {
class Symbol : public Object {
  friend class Parser;

 public:
  struct Comparator {
    auto operator()(Symbol* lhs, Symbol* rhs) const -> bool {
      ASSERT(lhs && rhs);
      if (lhs->GetNamespace() != rhs->GetNamespace())
        return false;
      return lhs->GetSymbolName() == rhs->GetSymbolName();
    }
  };

 private:
  std::string ns_;
  std::string type_;
  std::string name_;

  explicit Symbol(const std::string& ns, const std::string& type, const std::string& name) :
    Object(),
    ns_(ns),
    type_(type),
    name_(name) {
    ASSERT(!name_.empty());
  }

  void SetNamespace(const std::string& rhs) {
    ASSERT(!rhs.empty());
    ns_ = rhs;
  }

  void SetNamespace(Namespace* ns);

 public:
  ~Symbol() override = default;

  auto GetNamespace() const -> std::string {
    return ns_;
  }

  inline auto HasNamespace() const -> bool {
    return !ns_.empty();
  }

  auto GetSymbolName() const -> const std::string& {
    return name_;
  }

  auto GetSymbolType() const -> const std::string& {
    return type_;
  }

  inline auto HasSymbolType() const -> bool {
    return !type_.empty();
  }

  auto GetFullyQualifiedName() const -> std::string {
    if (HasNamespace()) {
      if (HasSymbolType())
        return fmt::format("{}/{}:{}", GetNamespace(), GetSymbolType(), GetSymbolName());
      return fmt::format("{}/{}", GetNamespace(), GetSymbolName());
    }
    if (HasSymbolType())
      return fmt::format("{}:{}", GetSymbolType(), GetSymbolName());
    return GetSymbolName();
  }

  auto Equals(const std::string& rhs) const -> bool;
  DECLARE_TYPE(Symbol);

 public:
  static auto New(const std::string& ns, const std::string& type, const std::string& name) -> Symbol*;

  static inline auto New(const std::string& ns, const std::string& name) -> Symbol* {
    return New(ns, "", name);
  }

  static inline auto New(const std::string& rhs) -> Symbol* {
    const auto slashpos = rhs.find_last_of('/');
    const auto ns = slashpos != std::string::npos ? rhs.substr(0, slashpos) : "";
    const auto colonpos = rhs.find_last_of(':');
    const auto type = colonpos != std::string::npos ? rhs.substr(ns.empty() ? 0 : ns.length() + 1, colonpos) : "";
    const auto name = rhs.substr((ns.empty() ? 0 : ns.length() + 1) + (type.empty() ? 0 : type.length() + 1));
    return New(ns, type, name);
  }
};

using SymbolList = std::vector<Symbol*>;
using SymbolSet = std::unordered_set<Symbol*, Symbol::Comparator>;
}  // namespace gel

namespace fmt {
using gel::Symbol;

template <>
struct formatter<Symbol> : public formatter<std::string> {
  template <typename FormatContext>
  constexpr auto format(const Symbol& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", value.GetFullyQualifiedName());
  }
};
}  // namespace fmt

#endif  // GEL_SYMBOL_H
