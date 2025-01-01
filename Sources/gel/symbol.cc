#include "gel/symbol.h"

#include "gel/namespace.h"
#include "gel/thread_local.h"
#include "gel/to_string_helper.h"
#include "gel/trie.h"

namespace gel {
using SymbolNode = trie::Node<Symbol*, Symbol::kAlphabetSize>;

static ThreadLocal<SymbolNode> trie_(new SymbolNode());

void Symbol::SetNamespace(Namespace* ns) {
  ASSERT(ns);
  return SetNamespace(ns->GetSymbol()->GetSymbolName());
}

auto Symbol::Equals(const std::string& rhs) const -> bool {
  ASSERT(!rhs.empty());
  return GetFullyQualifiedName() == rhs;
}

auto Symbol::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsSymbol())
    return false;
  const auto other = rhs->AsSymbol();
  ASSERT(other);
  if ((HasNamespace() && !other->HasNamespace()) || (!HasNamespace() && other->HasNamespace()) ||
      (GetNamespace() != other->GetNamespace()))
    return false;
  return GetSymbolName() == other->GetSymbolName();
}

auto Symbol::HashCode() const -> uword {
  uword hash = 0;
  if (HasNamespace())
    CombineHash(hash, GetNamespace());
  CombineHash(hash, GetSymbolName());
  return hash;
}

auto Symbol::CreateClass() -> Class* {
  return Class::New(Object::GetClass(), kClassName);
}

auto Symbol::New(const ObjectList& args) -> Symbol* {
  NOT_IMPLEMENTED(FATAL);
}

auto Symbol::ToString() const -> std::string {
  ToStringHelper<Symbol> helper;
  helper.AddField("value", GetFullyQualifiedName());
  return helper;
}

auto Symbol::New(const std::string& ns, const std::string& type, const std::string& name) -> Symbol* {
  ASSERT(!name.empty());
  std::stringstream ss;  // TODO: remove this
  if (!ns.empty())
    ss << ns << "/";
  if (!type.empty())
    ss << type << ":";
  ss << name;

  Symbol* symbol = nullptr;
  LOG_IF(FATAL, !trie::SearchOrCreate(trie_.Get(), ss.str(), &symbol,
                                      (const std::function<Symbol*(const std::string&)>)&Symbol::NewInternal))
      << "failed to internalize Symbol: " << ss.str();
  ASSERT(symbol);
  return symbol;
}

void Symbol::Init() {
  InitClass();
  ASSERT(trie_);
}
}  // namespace gel