#ifndef SCM_LOCAL_SCOPE_H
#define SCM_LOCAL_SCOPE_H

#include "scheme/common.h"
#include "scheme/local.h"

namespace scm {
class Symbol;
class LocalScope {
  DEFINE_NON_COPYABLE_TYPE(LocalScope);

 private:
  LocalScope* parent_;
  std::vector<LocalVariable*> locals_{};

  explicit LocalScope(LocalScope* parent) :
    parent_(parent) {}

 public:
  ~LocalScope() = default;

  auto GetParent() const -> LocalScope* {
    return parent_;
  }

  auto HasParent() const -> bool {
    return GetParent() != nullptr;
  }

  auto Has(const std::string& name, const bool recursive = false) -> bool;
  auto Has(const Symbol* symbol, const bool recursive = false) -> bool;
  auto Add(LocalVariable* local) -> bool;
  auto Add(Symbol* symbol, Type* value = nullptr) -> bool;
  auto Add(LocalScope* scope) -> bool;
  auto Lookup(const std::string& name, LocalVariable** result, const bool recursive = true) -> bool;
  auto Lookup(const Symbol* symbol, LocalVariable** result, const bool recursive = true) -> bool;

  auto IsEmpty() const -> bool {
    return locals_.empty();
  }

  auto GetNumberOfLocals() const -> uint64_t {
    return locals_.size();
  }

  inline auto Add(const std::string& name, Type* value = nullptr) -> bool {
    ASSERT(!name.empty());

    LocalVariable* local = nullptr;
    if (!Lookup(name, &local, false))
      return Add(local = new LocalVariable(this, GetNumberOfLocals(), name, value));
    if (local->HasValue())
      return false;
    local->SetConstantValue(value);
    return true;
  }

  auto VisitAllLocals(LocalVariableVisitor* vis) -> bool;

 public:
  static inline auto New(LocalScope* parent = nullptr) -> LocalScope* {
    return new LocalScope(parent);
  }
};

class LocalScopeIterator {
  DEFINE_NON_COPYABLE_TYPE(LocalScopeIterator);

 private:
  LocalScope* scope_;

 public:
  explicit LocalScopeIterator(LocalScope* scope) :
    scope_(scope) {}
  ~LocalScopeIterator() = default;

  auto HasNext() const -> bool {
    return scope_ != nullptr;
  }

  auto Next() -> LocalScope* {
    const auto next = scope_;
    ASSERT(next);
    scope_ = next->GetParent();
    return next;
  }
};
}  // namespace scm

#endif  // SCM_LOCAL_SCOPE_H
