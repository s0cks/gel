#include "gel/set.h"

#include <algorithm>

#include "gel/common.h"
#include "gel/hashcode.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/to_string_helper.h"

namespace gel {
auto Set::Union(Set* lhs, Set* rhs) -> Set* {
  StorageType result{};
  std::ranges::set_union(lhs->begin(), lhs->end(), rhs->begin(), rhs->end(), std::inserter(result, result.end()));
  return Of(result);
}

auto Set::Difference(Set* lhs, Set* rhs) -> Set* {
  StorageType result{};
  std::ranges::set_difference(lhs->begin(), lhs->end(), rhs->begin(), rhs->end(), std::inserter(result, result.end()),
                              ObjectComparator{});
  return Of(result);
}

auto Set::Intersection(Set* lhs, Set* rhs) -> Set* {
  std::vector<Object*> results{};
  std::ranges::set_intersection(lhs->begin(), lhs->end(), rhs->begin(), rhs->end(), std::back_inserter(results),
                                ObjectComparator{});
  return Of(results);
}

auto Set::GetHashCode() const -> HashCode {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return kInvalidHashCode;
}

auto Set::Equals(Object* rhs) const -> bool {
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto Set::ToString() const -> std::string {
  ToStringHelper<Set> helper;
  helper.AddField("size", GetSize());
  return helper;
}

auto Set::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto Set::Of(Object* value) -> Set* {
  if (value->IsNil())
    return Set::Of();
  else if (value->IsSet())
    return value->AsSet();
  else if (value->IsPair()) {
    if (value->AsPair()->IsEmpty())
      return Of();
    else if (value->AsPair()->IsTuple())
      return Of(ObjectList{
          value->AsPair()->GetFirst(),
          value->AsPair()->GetSecond(),
      });
    ObjectList values;
    auto v = value;
    while (!v->IsNil()) {
      values.push_back(gel::Car(v));
      v = gel::Cdr(v);
    }
    return Of(StorageType(std::begin(values), std::end(values)));
  }
  return Of(StorageType{value});
}

auto Set::New(const ObjectList& args) -> Set* {
  if (args.empty())
    return Of();
  else if (args.size() == 1)
    return Of(args[0]);
  StorageType data(args.begin(), args.end());
  return Of(data);
}

auto Set::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Seq::GetClass(), "Set");
}

void Set::Init() {
  InitClass();

  using namespace proc;
  InitNative<gel_union>();
  InitNative<gel_difference>();
  InitNative<gel_intersection>();
  InitNative<gel_subset>();
#define INIT_SET_NATIVE(Name) InitNative<set_##Name>()
  INIT_SET_NATIVE(contains);
  INIT_SET_NATIVE(empty);
  INIT_SET_NATIVE(count);
#undef INIT_SET_NATIVE
}

namespace proc {
NATIVE_PROCEDURE_F(gel_union) {
  REQUIRED_NATIVE_ARG(0, Set, a);
  REQUIRED_NATIVE_ARG(1, Set, b);
  return Return(Set::Union(a, b));
}

NATIVE_PROCEDURE_F(gel_difference) {
  REQUIRED_NATIVE_ARG(0, Set, a);
  REQUIRED_NATIVE_ARG(1, Set, b);
  return Return(Set::Difference(a, b));
}

NATIVE_PROCEDURE_F(gel_intersection) {
  REQUIRED_NATIVE_ARG(0, Set, a);
  REQUIRED_NATIVE_ARG(1, Set, b);
  return Return(Set::Intersection(a, b));
}

NATIVE_PROCEDURE_F(gel_subset) {
  REQUIRED_NATIVE_ARG(0, Set, a);
  REQUIRED_NATIVE_ARG(1, Set, b);
  return ReturnBool(std::includes(a->begin(), a->end(), b->begin(), b->end()));
}

#define SET_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(set_##Name)

SET_PROCEDURE_F(insert) {
  REQUIRED_NATIVE_ARG(0, Set, set);
  REQUIRED_NATIVE_ARG(1, Object, value);
  return ReturnBool(set->Insert(value));
}

SET_PROCEDURE_F(contains) {
  REQUIRED_NATIVE_ARG(0, Set, set);
  REQUIRED_NATIVE_ARG(1, Object, value);
  return ReturnBool(set->Contains(value));
}

SET_PROCEDURE_F(count) {
  REQUIRED_NATIVE_ARG(0, Set, set);
  return ReturnNumber(set->GetSize());
}

SET_PROCEDURE_F(empty) {
  REQUIRED_NATIVE_ARG(0, Set, set);
  return ReturnBool(set->IsEmpty());
}

#undef SET_PROCEDURE_F
}  // namespace proc
}  // namespace gel