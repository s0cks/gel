#include "gel/str.h"

#include "gel/boolean.h"
#include "gel/buffer.h"
#include "gel/number.h"
#include "gel/symbol.h"
#include "gel/to_string_helper.h"

namespace gel {
auto StringObject::Equals(Object* rhs) const -> bool {
  if (!rhs || !(rhs->IsString() || rhs->IsSymbol()))
    return false;
  if (rhs->IsSymbol())
    return Equals(rhs->AsSymbol()->GetFullyQualifiedName());
  ASSERT(rhs->IsString());
  return Equals(rhs->AsString()->Get());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
}

auto StringObject::Equals(const std::string& rhs) const -> bool {
  return Get().compare(rhs) == 0;
}

auto String::New() -> String* {
  static auto kEmptyString = new String();
  ASSERT(kEmptyString);
  return kEmptyString;
}

auto String::New(Symbol* rhs) -> String* {
  ASSERT(rhs);
  return New(rhs->GetFullyQualifiedName());
}

auto String::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto String::Compare(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsString())
    return false;
  return Get().compare(rhs->AsString()->Get());
}

auto String::ToBuffer() const -> Buffer* {
  return Buffer::Copy(Get());
}

auto String::Equals(const std::string& rhs) const -> bool {
  return StringObject::Equals(rhs);
}

auto String::Equals(Object* rhs) const -> bool {
  return StringObject::Equals(rhs);
}

auto String::Eq(Object* rhs) const -> Object* {
  return Bool::Box(Equals(rhs));
}

auto String::New(const ObjectList& args) -> String* {
  if (args.empty() || args[0]->IsNil())
    return New();
  if (args[0]->IsString())
    return String::New(args[0]->AsString()->Get());
  else if (gel::IsBuffer(args[0])) {
    const auto buffer = args[0]->AsBuffer();
    ASSERT(buffer);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    std::string value((const char*)buffer->data(), buffer->GetCapacity());
    return String::New(value);
  }
  return ValueOf(args[0]);
}

auto String::ToString() const -> std::string {
  ToStringHelper<String> helper;
  helper.AddField("value", Get());
  return helper;
}

auto String::Empty() -> String* {
  static String* kEmpty = String::New();
  ASSERT(kEmpty);
  return kEmpty;
}

auto String::ValueOf(Object* rhs) -> String* {
  if (rhs->IsString())
    return rhs->AsString();
  else if (rhs->IsSymbol())
    return String::New(rhs->AsSymbol()->GetFullyQualifiedName());
  std::stringstream ss;
  if (rhs->IsBool()) {
    ss << (Bool::Unbox(rhs->AsBool()) ? "#t" : "#f");
  } else if (rhs->IsNumber()) {
    ss << rhs->AsNumber()->Get();
  } else if (rhs->IsDouble()) {
    ss << rhs->AsDouble()->Get();
  } else if (rhs->IsSymbol()) {
    ss << rhs->AsSymbol()->GetFullyQualifiedName();
  } else if (rhs->IsPair()) {
    const auto pair = rhs->AsPair();
    ASSERT(pair);
    ss << "(";
    if (pair->IsEmpty()) {
      ss << ")";
    } else {
      PrintValue(ss, pair->GetFirst());
      auto next = pair->GetSecond();
      do {
        if (next->IsNil()) {
          ss << ")";
          break;
        }
        if (!next->IsPair()) {
          ss << " ";
          PrintValue(ss, next);
          ss << ")";
          break;
        }
        ss << " ";
        PrintValue(ss, next->AsPair()->GetFirst());
        next = next->AsPair()->GetSecond();
      } while (true);
    }
  } else if (rhs->IsError()) {
    ss << rhs->AsError()->GetMessage()->Get();
  } else {
    ss << rhs->ToString();
  }
  return String::New(ss.str());
}

auto String::GetHashCode() const -> HashCode {
  return StringObject::GetHashCode();
}

auto StringObject::GetHashCode() const -> HashCode {
  HashCode hash{};
  hash ^= Get();
  return hash;
}
}  // namespace gel