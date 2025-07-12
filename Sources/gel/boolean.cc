#include "gel/boolean.h"

#include "gel/class.h"
#include "gel/number.h"

namespace gel {
static Bool* kTrue = nullptr;   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static Bool* kFalse = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

auto Bool::CreateClass() -> Class* {
  return Class::New(Class::kBoolClassId, Object::GetClass(), kClassName);
}

void Bool::Init() {
  InitClass();
  kTrue = NewTrue();
  kFalse = NewFalse();
}

auto Bool::Equals(Object* rhs) const -> bool {
  if (!rhs->IsBool())
    return false;
  return Get() == rhs->AsBool()->Get();
}

auto Bool::New(const ObjectList& args) -> Bool* {
  if (args.empty())
    return False();
  else if (args.size() == 1) {
    if (args[0]->IsNumber() && (args[0]->AsNumber()->GetLong() == 0))
      return False();
    return Box(gel::Truth(args[0]));
  }
  return Box(gel::Truth(gel::ToList(args)));
}

auto Bool::New(const bool value) -> Bool* {
  return new Bool(value);
}

auto Bool::True() -> Bool* {
  ASSERT(kTrue);
  return kTrue;
}

auto Bool::False() -> Bool* {
  ASSERT(kFalse);
  return kFalse;
}

auto Bool::Compare(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsBool())
    return false;
  return Get() < rhs->AsBool()->Get();
}

auto Bool::GetHashCode() const -> HashCode {
  HashCode hash{};
  hash ^= Get();
  return hash;
}

auto Bool::ToString() const -> std::string {
  return Get() ? "#T" : "#F";
}
}  // namespace gel