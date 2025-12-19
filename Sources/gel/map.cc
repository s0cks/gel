#include "map.h"

#include "hashcode.h"
#include "native_procedure.h"
#include "to_string_helper.h"

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

auto Map::Get(Symbol* key) const -> Object* {
  ASSERT(key);
  const auto pos = Find(key);
  return pos == std::end(data()) ? pos->second : Nil::Get();
}

auto Map::GetHashCode() const -> HashCode {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return kInvalidHashCode;
}

auto Map::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsMap())
    return false;
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Map::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto Map::ToString() const -> std::string {
  ToStringHelper<Map> helper{};
  helper.AddField("size", GetSize());
  if (!IsEmpty()) {
    std::stringstream ss;
    ss << "{";
    auto remaining = data().size();
    for (const auto& [first, second] : data()) {
      ss << first->AsSymbol()->GetSymbolName() << ": " << second->ToString();
      if (--remaining >= 0)
        ss << ", ";
    }
    ss << "}";
    helper.AddField("data", ss.str());
  } else {
    helper.AddField("data", "{}");
  }
  return helper;
}

auto Map::New(const ObjectList& args) -> Map* {
  ASSERT(args.empty() || (args.size() % 2 == 0));
  if (args.empty())
    return Map::New();
  StorageType data{};
  for (auto idx = 0; idx < args.size(); idx += 2) {
    const auto key = args[idx];
    ASSERT(gel::IsSymbol(key));
    const auto value = args[idx + 1];
    ASSERT(value);
    data.insert({key->AsSymbol(), value});  // TODO: prolly should check this insertion
  }
  return Map::New(data);
}

auto Map::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Seq::GetClass(), "map");
}

namespace proc {
#define MAP_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(map_##Name)

MAP_PROCEDURE_F(contains) {
  REQUIRED_NATIVE_ARG(0, Map, map);
  REQUIRED_NATIVE_ARG(1, Symbol, key);
  return ReturnBool(map->Contains(key));
}

MAP_PROCEDURE_F(empty) {
  REQUIRED_NATIVE_ARG(0, Map, map);
  return ReturnBool(map->IsEmpty());
}

MAP_PROCEDURE_F(size) {
  REQUIRED_NATIVE_ARG(0, Map, map);
  return ReturnLong(map->GetSize());
}

MAP_PROCEDURE_F(get) {
  REQUIRED_NATIVE_ARG(0, Map, map);
  REQUIRED_NATIVE_ARG(1, Symbol, key);
  return Return(map->Get(key));
}

#undef MAP_PROCEDURE_F
}  // namespace proc
}  // namespace gel
