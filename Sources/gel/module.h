#ifndef GEL_MODULE_H
#define GEL_MODULE_H

#include "gel/namespace.h"
#include "gel/object.h"

namespace gel {
class Module;
using ModuleList = std::vector<Module*>;
class Module : public Object {
 private:
  String* name_;
  LocalScope* scope_;
  NamespaceList namespaces_{};

  void Append(Namespace* ns) {
    ASSERT(ns);
    namespaces_.push_back(ns);
  }

 protected:
  explicit Module(String* name, LocalScope* scope) :
    Object(),
    name_(name),
    scope_(scope) {
    ASSERT(name_);
    ASSERT(scope_);
  }

 public:
  ~Module() override = default;

  auto GetName() const -> String* {
    return name_;
  }

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  auto GetNamespaces() const -> const NamespaceList& {
    return namespaces_;
  }

  auto GetNumberOfNamespaces() const -> uword {
    return namespaces_.size();
  }

  auto GetNamespaceAt(const uword idx) const -> Namespace* {
    ASSERT(idx >= 0 && idx <= GetNumberOfNamespaces());
    return namespaces_[idx];
  }

  DECLARE_TYPE(Module);

 private:
  static ModuleList modules_;

 public:
  static inline auto New(String* name, LocalScope* scope) -> Module* {
    ASSERT(scope);
    const auto m = new Module(name, scope);
    ASSERT(m);
    modules_.push_back(m);
    return m;
  }
};
}  // namespace gel

#endif  // GEL_MODULE_H
