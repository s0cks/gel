#ifndef GEL_NAMESPACE_H
#define GEL_NAMESPACE_H

#include <type_traits>
#include <vector>

#include "gel/common.h"
#include "gel/object.h"
#include "gel/type_traits.h"

namespace gel {
class Namespace;
using NamespaceList = std::vector<Namespace*>;

class Namespace : public Object {
  friend class Script;
  friend class Parser;

 public:
  static constexpr const auto kPrefixChar = '/';

 private:
  Object* owner_ = nullptr;
  Symbol* symbol_;
  LocalScope* scope_;
  String* docs_ = nullptr;

 protected:
  explicit Namespace(Symbol* symbol, LocalScope* scope) :
    Object(),
    symbol_(symbol),
    scope_(scope) {
    ASSERT(symbol_);
    ASSERT(scope_);
  }

  void SetDocs(String* rhs) {
    ASSERT(rhs);
    docs_ = rhs;
  }

  void SetOwner(Object* rhs) {
    ASSERT(rhs);
    owner_ = rhs;
  }

  auto IsKernelNamespace() const -> bool;

 public:
  ~Namespace() override = default;

  auto GetSymbol() const -> Symbol* {
    return symbol_;
  }

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto GetDocs() const -> String* {
    return docs_;
  }

  auto GetOwner() const -> Object* {
    return owner_;
  }

  auto Get(Symbol* rhs) const -> Object*;
  auto Get(const std::string& rhs) const -> Object*;
  auto HasSymbol(Symbol* rhs) const -> bool;
  auto HasSymbol(const std::string& rhs) const -> bool;
  auto GetName() const -> const std::string&;
  auto CreateSymbol(const std::string& value) -> Symbol*;
  DECLARE_TYPE(Namespace);

 private:
  static NamespaceList namespaces_;

 public:
  static inline auto New(Symbol* symbol, LocalScope* scope) -> Namespace* {
    ASSERT(symbol);
    ASSERT(scope);
    const auto ns = new Namespace(symbol, scope);
    ASSERT(ns);
    namespaces_.push_back(ns);
    return ns;
  }

  static auto FindNamespace(const std::string& name) -> Namespace*;
  static auto FindNamespace(Symbol* rhs) -> Namespace*;
};

}  // namespace gel

#endif  // GEL_NAMESPACE_H
