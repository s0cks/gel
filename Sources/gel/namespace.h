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

 private:
  Object* owner_ = nullptr;
  String* name_;
  LocalScope* scope_;
  String* docs_ = nullptr;

 protected:
  explicit Namespace(String* name, LocalScope* scope) :
    Object(),
    name_(name),
    scope_(scope) {
    ASSERT(name_);
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

  // TODO: remove this function
  inline auto IsKernelNamespace() const -> bool {
    return name_->Get() == "_kernel";
  }

 public:
  ~Namespace() override = default;

  auto GetName() const -> String* {
    return name_;
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

  auto Prefix(Symbol* rhs) const -> Symbol*;
  auto HasPrefix(Symbol* rhs) const -> bool;
  DECLARE_TYPE(Namespace);

 private:
  static NamespaceList namespaces_;

 public:
  static inline auto New(String* name, LocalScope* scope) -> Namespace* {
    ASSERT(name);
    ASSERT(scope);
    const auto ns = new Namespace(name, scope);
    ASSERT(ns);
    namespaces_.push_back(ns);
    return ns;
  }

  static auto Get(const std::string& name) -> Namespace*;

  template <typename T>
  static inline auto Get(T* name, std::enable_if_t<is_string_like<T>::value>* = nullptr) -> Namespace* {
    ASSERT(name && !name->IsEmpty());
    return Get(name->Get());
  }
};

}  // namespace gel

#endif  // GEL_NAMESPACE_H
