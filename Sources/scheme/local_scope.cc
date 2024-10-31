#include "scheme/local_scope.h"

#include <glog/logging.h>

#include "scheme/common.h"
#include "scheme/local.h"
#include "scheme/type.h"

namespace scm {
auto LocalScope::Has(const std::string& name, const bool recursive) -> bool {
  for (const auto& local : locals_) {
    if (local->GetName() == name)
      return true;
  }
  return recursive && HasParent() ? GetParent()->Has(name, recursive) : false;
}

auto LocalScope::Has(const Symbol* symbol, const bool recursive) -> bool {
  ASSERT(symbol);
  return Has(symbol->Get(), recursive);
}

auto LocalScope::Add(LocalVariable* local) -> bool {
  ASSERT(local);
  if (Has(local->GetName())) {
    DLOG(ERROR) << "cannot add duplicate local: " << (*local);
    return false;
  }
  locals_.push_back(local);
  if (!local->HasOwner())
    local->SetOwner(this);
  return true;
}

auto LocalScope::Add(Symbol* symbol, Type* value) -> bool {
  ASSERT(symbol);
  return Add(symbol->Get(), value);
}

auto LocalScope::Add(LocalScope* scope) -> bool {
  ASSERT(scope);
  for (const auto& local : scope->locals_) {
    if (!Add(local->GetName(), local->GetValue())) {
      LOG(ERROR) << "failed to add local " << local->GetName() << " to scope.";
      return false;
    }
  }
  DVLOG(10) << "added " << scope->GetNumberOfLocals() << " locals to scope.";
  return true;
}

auto LocalScope::Lookup(const std::string& name, LocalVariable** result, const bool recursive) -> bool {
  ASSERT(!name.empty());
  for (const auto& local : locals_) {
    if (local->GetName() == name) {
      (*result) = local;
      return true;
    }
  }
  return recursive && HasParent() ? GetParent()->Lookup(name, result, recursive) : false;
}

auto LocalScope::Lookup(const Symbol* symbol, LocalVariable** result, const bool recursive) -> bool {
  ASSERT(symbol);
  return Lookup(symbol->Get(), result, recursive);
}

static inline auto operator<<(std::ostream& stream, const std::vector<LocalVariable*>& rhs) -> std::ostream& {
  stream << "[";
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  stream << "]";
  return stream;
}

auto LocalScope::ToString() const -> std::string {
  std::stringstream ss;
  ss << "LocalScope(";
  ss << "locals=" << locals_;
  if (HasParent()) {
    ss << ", parent=" << GetParent();
  }
  ss << ")";
  return ss.str();
}

auto LocalScope::VisitAllLocals(LocalVariableVisitor* vis) -> bool {
  ASSERT(vis);
  for (const auto& local : locals_) {
    if (!vis->VisitLocal(local))
      return false;
  }
  return true;
}

#define __ (google::LogMessage(GetFile(), GetLine(), GetSeverity()).stream()) << GetIndentString()

auto LocalScopePrinter::VisitLocal(LocalVariable* local) -> bool {
  __ << "- " << (*local);
  return true;
}

auto LocalScopePrinter::PrintLocalScope(LocalScope* scope) -> bool {
  ASSERT(scope);
  __ << "Local Scope (" << scope->GetNumberOfLocals() << " locals):";
  Indent();
  if (!scope->VisitAllLocals(this)) {
    LOG(FATAL) << "failed to visit local scope: " << scope->ToString();
    return false;
  }
  Deindent();
  return true;
}

#undef __
}  // namespace scm