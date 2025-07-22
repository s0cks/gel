#include "gel/common.h"
#include "gel/native_procedure.h"
#include "gel/natives.h"
#include "gel/object.h"
#ifdef GEL_DEBUG

#include "gel/collector.h"
#include "gel/gel.h"
#include "gel/heap.h"
#include "gel/module.h"
#include "gel/runtime.h"

namespace gel::proc {
NATIVE_PROCEDURE_F(gel_print_heap) {
  const auto heap = GetCurrentThreadHeap();
  if (heap) {
    PrintHeap(*heap);
  }
  return Return();
}

NATIVE_PROCEDURE_F(gel_print_new_zone) {
  const auto heap = GetCurrentThreadHeap();
  if (heap) {
    PrintNewZone(heap->GetNewZone());
  }
  return Return();
}

NATIVE_PROCEDURE_F(gel_print_old_zone) {
  const auto heap = GetCurrentThreadHeap();
  if (heap) {
    PrintOldZone(heap->GetOldZone());
  }
  return Return();
}

NATIVE_PROCEDURE_F(gel_numrefs) {
  REQUIRED_NATIVE_ARG(0, Object, value);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return ReturnNumber();
}

NATIVE_PROCEDURE_F(gel_print_roots) {
  PrintRoots();
  return ReturnNull();
}

NATIVE_PROCEDURE_F(gel_minor_gc) {
  gel::MinorCollection();
  return DoNothing();
}

NATIVE_PROCEDURE_F(gel_major_gc) {
  gel::MajorCollection();
  return DoNothing();
}

NATIVE_PROCEDURE_F(gel_get_debug) {
#ifdef GEL_DEBUG
  return ReturnTrue();
#else
  return ReturnFalse();
#endif  // GEL_DEBUG
}

NATIVE_PROCEDURE_F(gel_get_locals) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  LocalScope::Iterator iter(GetRuntime()->GetScope());
  return Return(gel::ToList<LocalScope::Iterator, LocalVariable*>(iter, [](LocalVariable* local) -> Object* {
    return gel::ToList(ObjectList{
        local->HasValue() ? local->GetValue() : Nil::Get(),
        Str::New(local->GetSymbol()),
    });
  }));
}

NATIVE_PROCEDURE_F(gel_get_target_triple) {
  return ReturnNew<Str>(GEL_TARGET_TRIPLE);
}

NATIVE_PROCEDURE_F(gel_get_natives) {
  ASSERT(args.empty());
  const auto& natives = NativeFn::GetAll();
  Object* result = Nil::Get();
  for (const auto& native : natives) {
    result = Pair::New(Str::ValueOf(native->GetSymbol()), result);
  }
  return Return(result);
}

NATIVE_PROCEDURE_F(gel_get_compile_time) {
  REQUIRED_NATIVE_ARG(0, LambdaFn, target);
  const auto& code = target->GetCode();
  return ReturnNumber(code->GetCompileTime());
}
}  // namespace gel::proc

#endif  // GEL_DEBUG