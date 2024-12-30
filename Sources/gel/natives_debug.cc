#include "gel/native_procedure.h"
#include "gel/natives.h"
#ifdef GEL_DEBUG

#include "gel/collector.h"
#include "gel/gel.h"
#include "gel/heap.h"
#include "gel/module.h"
#include "gel/runtime.h"

namespace gel::proc {
NATIVE_PROCEDURE_F(gel_get_fields) {
  NativeArgument<0, Class> cls(args);
  if (!cls)
    return Throw(cls);
  Object* result = Null();
  for (const auto& field : cls->GetFields()) {
    result = Cons(Cons(field->GetName(), Long::New(field->GetOffset())), result);
  }
  return Return(result);
}

NATIVE_PROCEDURE_F(gel_print_args) {
  NativeArgument<0, Procedure> func(args);
  if (!func)
    return Throw(func);
  if (func->IsLambda()) {
    const auto& arguments = func->AsLambda()->GetArgs();
    DLOG(INFO) << func->GetSymbol() << " arguments:";
    for (const auto& arg : arguments) {
      DLOG(INFO) << " - " << arg;
    }
  } else if (func->IsNativeProcedure()) {
    const auto& arguments = func->AsNativeProcedure()->GetArgs();
    DLOG(INFO) << func->GetSymbol() << " arguments:";
    for (const auto& arg : arguments) {
      DLOG(INFO) << " - " << arg;
    }
  }
  return Return();
}

NATIVE_PROCEDURE_F(gel_print_heap) {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return Return();
}

NATIVE_PROCEDURE_F(gel_get_modules) {
  std::vector<Module*> modules{};
  Module::GetAllLoadedModules(modules);
  return Return(ToList((const ObjectList&)modules));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

NATIVE_PROCEDURE_F(gel_print_new_zone) {
  const auto heap = Heap::GetHeap();
  if (!heap)
    return Return();
  PrintNewZone(heap->GetNewZone());
  return Return();
}

NATIVE_PROCEDURE_F(gel_print_old_zone) {
  const auto heap = Heap::GetHeap();
  if (!heap)
    return Return();
  PrintOldZone(heap->GetOldZone());
  return Return();
}

NATIVE_PROCEDURE_F(gel_get_roots) {
  Object* result = Null();
  LOG_IF(FATAL, !VisitRoots([&result](Pointer** ptr) {
           ASSERT((*ptr));
           result = Pair::New((*ptr)->GetObjectPointer(), result);
           return true;
         }))
      << "failed to visit roots.";
  return Return(result);
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

NATIVE_PROCEDURE_F(gel_get_frame) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  DLOG(INFO) << "stack frames:";
  StackFrameIterator iter(runtime->stack_);
  while (iter.HasNext()) {
    DLOG(INFO) << "- " << iter.Next();
  }
  return DoNothing();
}

NATIVE_PROCEDURE_F(gel_print_st) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  LOG(INFO) << "Stack Trace:";
  StackFrameIterator iter(runtime->stack_);
  while (iter.HasNext()) {
    const auto& next = iter.Next();
    LOG(INFO) << "  " << next.GetId() << ": " << next.GetTargetName();
  }
  return DoNothing();
}

NATIVE_PROCEDURE_F(gel_get_locals) {
  ASSERT(HasRuntime());
  ASSERT(args.empty());
  LocalScope::Iterator iter(GetRuntime()->GetScope());
  return Return(gel::ToList<LocalScope::Iterator, LocalVariable*>(iter, [](LocalVariable* local) -> Object* {
    return gel::ToList(ObjectList{

        local->HasValue() ? local->GetValue() : Null(),
        String::New(local->GetName()),
    });
  }));
}

NATIVE_PROCEDURE_F(gel_get_target_triple) {
  return ReturnNew<String>(GEL_TARGET_TRIPLE);
}

NATIVE_PROCEDURE_F(gel_get_natives) {
  ASSERT(args.empty());
  const auto& natives = NativeProcedure::GetAll();
  Object* result = Null();
  for (const auto& native : natives) {
    result = Pair::New(String::ValueOf(native->GetSymbol()), result);
  }
  return Return(result);
}

NATIVE_PROCEDURE_F(gel_get_compile_time) {
  NativeArgument<0, Lambda> lambda(args);
  if (!lambda)
    return Throw(lambda.GetError());
  return ReturnNew<Long>(lambda->GetCompileTime());
}
}  // namespace gel::proc

#endif  // GEL_DEBUG