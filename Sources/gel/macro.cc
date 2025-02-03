#include "gel/macro.h"

#include <sstream>

#include "gel/common.h"
#include "gel/expr/expression.h"
#include "gel/local.h"
#include "gel/local_scope.h"
#include "gel/namespace.h"
#include "gel/native_procedure.h"
#include "gel/symbol.h"
#include "gel/to_string_helper.h"
#include "gel/type.h"

namespace gel {
auto Macro::New(const ObjectList& args) -> Macro* {
  NOT_IMPLEMENTED(FATAL);
}

auto Macro::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), "Macro");
}

auto Macro::HashCode() const -> uword {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
}

auto Macro::VisitPointers(PointerVisitor* vis) -> bool {
  ASSERT(vis);
  if (HasOwner()) {
    if (!vis->Visit(GetOwner()))
      return false;
  }
  if (!vis->Visit(GetSymbol()))
    return false;
  if (HasDocstring()) {
    if (!vis->Visit(GetDocstring()))
      return false;
  }
  // TODO: visit body
  return true;
}

auto Macro::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsMacro())
    return false;
  const auto other = rhs->AsMacro();
  ASSERT(other);
  return GetSymbol()->Equals(other->GetSymbol());
}

auto Macro::ToString() const -> std::string {
  ToStringHelper<Macro> helper;
  helper.AddField("symbol", GetSymbol()->GetFullyQualifiedName());
  helper.AddField("args", GetArgs());
  if (HasDocstring())
    helper.AddField("docs", GetDocstring()->Get());
  if (IsEmpty())
    helper.AddField("empty", IsEmpty());
  return helper;
}

#define INIT_MACRO_NATIVE(Name) InitNative<macro_##Name>();

void Macro::Init() {
  InitClass();
  using namespace proc;
  INIT_MACRO_NATIVE(get_owner);
  INIT_MACRO_NATIVE(get_symbol);
}

#undef INIT_MACRO_NATIVE

namespace proc {
#define MACRO_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(macro_##Name)

template <const uint64_t Index = 0>
class MacroArgument : public NativeArgument<Index> {
  DEFINE_NON_COPYABLE_TYPE(MacroArgument);

 public:
  explicit MacroArgument(const ObjectList& args) :
    NativeArgument<Index>(args) {
    if (NativeArgument<Index>::HasError())
      return;
    const auto value = NativeArgument<Index>::GetValue();
    ASSERT(value);
    if (value->IsMacro()) {
      return;
    } else if (value->IsSymbol()) {
      const auto symbol = value->AsSymbol();
      ASSERT(symbol);
      if (!symbol->HasNamespace()) {
        NativeArgument<Index>::SetError(fmt::format("failed to find Macro: `{}`", (*symbol)));
        return;
      }
      const auto ns = Namespace::FindNamespace(symbol->GetNamespace());
      if (!ns) {
        NativeArgument<Index>::SetError(fmt::format("failed to find Namespace for: `{}`", (*symbol)));
        return;
      }
      const auto macro = ns->FindMacro(symbol->GetSymbolName());
      if (!macro) {
        NativeArgument<Index>::SetError(fmt::format("failed to find Macro for: `{}`", (*symbol)));
        return;
      }
      NativeArgument<Index>::SetValue(macro);
      return;
    }
    NativeArgument<Index>::SetError(fmt::format("expected `{}` to be an instanceof Module or Symbol", (*value)));
  }
  ~MacroArgument() override = default;

  inline auto GetMacro() const -> Macro* {
    ASSERT(NativeArgument<Index>::GetValue()->IsMacro());
    return NativeArgument<Index>::GetValue()->AsMacro();
  }

  auto operator->() const -> Macro* {
    return GetMacro();
  }

  operator bool() const {
    return !NativeArgument<Index>::HasError();
  }
};

MACRO_PROCEDURE_F(get_owner) {
  MacroArgument<0> macro(args);
  if (!macro)
    return Throw(macro);
  return Return(macro->GetOwner());
}

MACRO_PROCEDURE_F(get_symbol) {
  MacroArgument<0> macro(args);
  if (!macro)
    return Throw(macro);
  return Return(macro->GetSymbol());
}

#undef MACRO_PROCEDURE_F
}  // namespace proc
}  // namespace gel