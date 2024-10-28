#include "scheme/module.h"

#include "scheme/expression.h"
#include "scheme/local.h"
#include "scheme/local_scope.h"

namespace scm {
Module::~Module() {
  ASSERT(symbol_);
  delete symbol_;
  ASSERT(scope_);
  delete scope_;
}

auto Module::ToString() const -> std::string {
  std::stringstream ss;
  ss << "Module(";
  ss << "symbol=" << GetSymbol();
  if (!IsEmpty())
    ss << ", scope=" << GetScope();
  ss << ")";
  return ss.str();
}

#define __ ((google::LogMessage(GetFile(), GetLine(), GetSeverity())).stream() << GetIndent())
auto ModulePrinter::VisitLocal(LocalVariable* local) -> bool {
  if (local->HasValue()) {
    __ << "- #" << local->GetIndex() << " " << local->GetName() << ": " << local->GetValue()->ToString();
  } else {
    __ << "- #" << local->GetIndex() << " " << local->GetName();
  }
  return true;
}

void ModulePrinter::Print(Module* module) {
  ASSERT(module);
  __ << "Module Name: " << module->GetSymbol();
  const auto scope = module->GetScope();
  ASSERT(scope);
  if (scope->IsEmpty()) {
    __ << "Scope: Empty";
  } else {
    __ << "Scope: ";
    {
      Indent();
      LocalScopeIterator iter(module->GetScope());
      while (iter.HasNext()) {
        const auto scope = iter.Next();
        ASSERT(scope);
        if (!scope->VisitAllLocals(this)) {
          LOG(ERROR) << "failed to visit all locals in scope.";
          return;
        }
      }
      Deindent();
    }
  }
}
#undef __
}  // namespace scm