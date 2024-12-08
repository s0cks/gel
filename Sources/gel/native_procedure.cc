#include "gel/native_procedure.h"

#include "gel/common.h"
#include "gel/natives.h"
#include "gel/runtime.h"
#include "gel/to_string_helper.h"
#include "gel/type.h"

namespace gel {
NativeProcedureList NativeProcedure::all_{};

void NativeProcedure::Init() {
  using namespace proc;
  InitClass();
  //----------------------------------------------------------------------
  // kernel lib
  //----------------------------------------------------------------------
  InitNative<print>();
  InitNative<type>();
  InitNative<import>();
  InitNative<exit>();
  InitNative<format>();
  InitNative<list>();
  InitNative<set_car>();
  InitNative<set_cdr>();
  InitNative<random>();
  InitNative<gel_docs>();
  InitNative<rand_range>();
  //----------------------------------------------------------------------
  // array lib
  //----------------------------------------------------------------------
  InitNative<array_new>();
  InitNative<array_get>();
  InitNative<array_set>();
  InitNative<array_length>();
  //----------------------------------------------------------------------
  // uv lib
  //----------------------------------------------------------------------
  InitNative<uv_close>();
  InitNative<uv_run>();
  InitNative<uv_alive>();
  InitNative<uv_stop>();
  //----------------------------------------------------------------------

#ifdef GEL_ENABLE_RX
#define REGISTER_RX(Name) InitNative<rx_##Name>();
  //----------------------------------------------------------------------
  // rx lib
  //----------------------------------------------------------------------
  REGISTER_RX(observer);
  REGISTER_RX(observable);
  REGISTER_RX(subscribe);
  REGISTER_RX(first);
  REGISTER_RX(last);
  REGISTER_RX(map);
  REGISTER_RX(take);
  REGISTER_RX(take_last);
  REGISTER_RX(skip);
  REGISTER_RX(buffer);
  REGISTER_RX(filter);
  REGISTER_RX(take_while);
  REGISTER_RX(replay_subject);
  REGISTER_RX(publish_subject);
  REGISTER_RX(publish);
  REGISTER_RX(complete);
  REGISTER_RX(publish_error);
#undef REGISTER_RX
#endif  // GEL_ENABLE_RX

#ifdef GEL_DEBUG
  //----------------------------------------------------------------------
  // debug lib
  //----------------------------------------------------------------------
  InitNative<gel_print_heap>();
  InitNative<gel_print_new_zone>();
  InitNative<gel_print_old_zone>();
  InitNative<gel_get_roots>();
  InitNative<gel_minor_gc>();
  InitNative<gel_major_gc>();
  InitNative<gel_get_frame>();
  InitNative<gel_get_debug>();
  InitNative<gel_get_target_triple>();
  InitNative<gel_get_locals>();
  InitNative<gel_get_classes>();
  InitNative<gel_get_natives>();
  InitNative<gel_get_compile_time>();
#endif  // GEL_DEBUG
}

void NativeProcedure::Register(NativeProcedure* native) {
  ASSERT(native);
  all_.push_back(native);
}

auto NativeProcedure::Find(const std::string& name) -> NativeProcedure* {
  ASSERT(!name.empty());
  for (const auto& native : all_) {
    ASSERT(native);
    const auto symbol = native->GetSymbol()->Get();
    if (name == symbol)
      return native;
  }
  return nullptr;
}

auto NativeProcedure::Find(Symbol* symbol) -> NativeProcedure* {
  ASSERT(symbol);
  for (const auto& native : all_) {
    ASSERT(native);
    if (native->GetSymbol()->Equals(symbol))
      return native;
  }
  return nullptr;
}

auto NativeProcedure::New(const ObjectList& args) -> NativeProcedure* {
  NOT_IMPLEMENTED(FATAL);
}

auto NativeProcedure::CreateClass() -> Class* {
  return Class::New(Procedure::GetClass(), kClassName);
}

auto NativeProcedure::Return(Object* rhs) const -> bool {
  ASSERT(rhs);
  GetRuntime()->Push(rhs);
  return DoNothing();
}

auto NativeProcedure::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsNativeProcedure())
    return false;
  return GetSymbol()->Equals(rhs->AsNativeProcedure()->GetSymbol());
}

auto NativeProcedure::ToString() const -> std::string {
  ToStringHelper<NativeProcedure> helper;
  helper.AddField("symbol", GetSymbol());
  helper.AddField("args", GetArgs());
  if (HasDocs())
    helper.AddField("docs", GetDocs());
  return helper;
}

}  // namespace gel