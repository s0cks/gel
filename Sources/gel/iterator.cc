#include "gel/iterator.h"

#include "gel/common.h"
#include "gel/native_procedure.h"
#include "gel/to_string_helper.h"

namespace gel {
auto Iterator::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto Iterator::Equals(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto Iterator::HashCode() const -> uword {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return 0;
}

auto Iterator::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto Iterator::New(const ObjectList& args) -> Iterator* {
  if (args.empty()) {
    NOT_IMPLEMENTED(FATAL);  // TODO: implement
  } else if (args[0]->IsSet()) {
    ASSERT(args.size() == 1);
    return new SetIterator(args[0]->AsSet());
  }
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
}

auto Iterator::ToString() const -> std::string {
  ToStringHelper<Iterator> helper{};
  return helper;
}

#define INIT_ITERATOR_NATIVE(Name) InitNative<iter_##Name>();

void Iterator::Init() {
  InitClass();

  using namespace proc;
  INIT_ITERATOR_NATIVE(next);
  INIT_ITERATOR_NATIVE(has_next);
}

#undef INIT_ITERATOR_NATIVE

namespace proc {
#define ITERATOR_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(iter_##Name)

ITERATOR_PROCEDURE_F(has_next) {
  REQUIRED_NATIVE_ARG(0, Iterator, iter);
  return ReturnBool(iter->HasNext());
}

ITERATOR_PROCEDURE_F(next) {
  REQUIRED_NATIVE_ARG(0, Iterator, iter);
  return Return(iter->Next());
}

#undef ITERATOR_PROCEDURE_F
}  // namespace proc
}  // namespace gel