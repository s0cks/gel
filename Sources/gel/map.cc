#include "gel/map.h"

#include "gel/native_procedure.h"
#include "gel/to_string_helper.h"

namespace gel {
void Map::Init() {
  using namespace proc;
#define InitMapNative(Name) InitNative<map_##Name>()
  InitMapNative(contains);
  InitMapNative(empty);
  InitMapNative(size);
  InitMapNative(get);
#undef InitMapNative
  InitClass();
}

auto Map::Get(Object* key) const -> Object* {
  ASSERT(key);
  const auto pos = Find(key);
  if (pos == std::end(data()))
    return Null();
  return pos->second;
}

auto Map::HashCode() const -> uword {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return 0;
}

auto Map::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsMap())
    return false;
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Map::ToString() const -> std::string {
  ToStringHelper<Map> helper;
  helper.AddField("size", GetSize());
  return helper;
}

auto Map::New(const ObjectList& args) -> Map* {
  ASSERT(args.empty() || (args.size() % 2 == 0));
  if (args.empty())
    return Map::New();
  StorageType data{};
  for (auto idx = 0; idx < args.size(); idx += 2) {
    const auto key = args[idx];
    ASSERT(key);
    const auto value = args[idx + 1];
    ASSERT(value);
    data.insert({key, value});  // TODO: prolly should check this insertion
  }
  return Map::New(data);
}

auto Map::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  const auto cls = Class::New(Seq::GetClass(), "Map");
  ASSERT(cls);
  cls->AddFunction(proc::map_contains::Get()->GetNative());
  return cls;
}

namespace proc {
#define MAP_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(map_##Name)

MAP_PROCEDURE_F(contains) {
  NativeArgument<0, Map> map(args);
  CHECK_NATIVE_ARG(map);
  NativeArgument<1> key(args);
  CHECK_NATIVE_ARG(key);
  return ReturnBool(map->Contains(key));
}

MAP_PROCEDURE_F(empty) {
  NativeArgument<0, Map> map(args);
  CHECK_NATIVE_ARG(map);
  return ReturnBool(map->IsEmpty());
}

MAP_PROCEDURE_F(size) {
  NativeArgument<0, Map> map(args);
  CHECK_NATIVE_ARG(map);
  return ReturnLong(map->GetSize());
}

MAP_PROCEDURE_F(get) {
  NativeArgument<0, Map> map(args);
  CHECK_NATIVE_ARG(map);
  NativeArgument<1> key(args);
  CHECK_NATIVE_ARG(key);
  return Return(map->Get(key));
}

#undef MAP_PROCEDURE_F
}  // namespace proc
}  // namespace gel