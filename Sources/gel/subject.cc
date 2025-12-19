#include "subject.h"

#include "heap.h"
#include "to_string_helper.h"

namespace gel {
DEFINE_NEW_OPERATOR(Subject);         // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(PublishSubject);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
DEFINE_NEW_OPERATOR(ReplaySubject);   // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

auto Subject::to_exception_ptr(Error* error) -> std::exception_ptr {
  ASSERT(error && !error->GetMessage()->Get().empty());
  return std::make_exception_ptr(Exception(String::Unbox(error->GetMessage())));
}

auto Subject::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  return Class::New(Object::GetClass(), kClassName);
}

auto Subject::ToString() const -> std::string {
  return ToStringHelper<Subject>{};
}

auto Subject::New(const ObjectList& args) -> Subject* {
  NOT_IMPLEMENTED(FATAL);
}

auto PublishSubject::New(const ObjectList& args) -> PublishSubject* {
  ASSERT(args.empty());
  return New();
}

auto PublishSubject::ToString() const -> std::string {
  return ToStringHelper<PublishSubject>{};
}

auto PublishSubject::Equals(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto PublishSubject::GetHashCode() const -> HashCode {
  return Subject::GetHashCode();
}

auto PublishSubject::Compare(Object* rhs) const -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto PublishSubject::CreateClass() -> Class* {
  return Class::New(Subject::GetClass(), "publish-subject");
}

auto ReplaySubject::New(const ObjectList& args) -> ReplaySubject* {
  ASSERT(args.empty());
  return New();
}

auto ReplaySubject::ToString() const -> std::string {
  return ToStringHelper<ReplaySubject>{};
}

auto ReplaySubject::GetHashCode() const -> HashCode {
  return Subject::GetHashCode();
}

auto ReplaySubject::Compare(Object* rhs) const -> bool {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return false;
}

auto ReplaySubject::Equals(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(FATAL);  // TODO: implement
  return false;
}

auto ReplaySubject::CreateClass() -> Class* {
  return Class::New(Subject::GetClass(), "replay-subject");
}
}  // namespace gel
