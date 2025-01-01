#include "gel/symbol.h"

#include "gel/common.h"
#include "gel/namespace.h"
#include "gel/natives.h"
#include "gel/thread_local.h"
#include "gel/to_string_helper.h"
#include "gel/trie.h"

namespace gel {
DEFINE_uword(symbol_pool_size, 65535, "Defines the maximum number of Symbols to be pooled (interned).");

static ThreadLocal<Symbol::PoolNode> trie_(new Symbol::PoolNode());
static ThreadLocal<uword> pool_size_(new uword(0));  // TODO: this is a really weird use

static inline void IncrementPoolSize() {
  *(pool_size_.Get()) += 1;
}

auto GetCurrentThreadSymbolPoolRoot() -> Symbol::PoolNode* {
  ASSERT(trie_);
  return trie_;
}

auto GetCurrentThreadSymbolPoolSize() -> uword {
  return *(pool_size_.Get());
}

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
  if ((GetCurrentThreadSymbolPoolSize() + 1) <= GetSymbolPoolMaxSize()) {
    const auto created = trie::SearchOrCreate<Symbol*>(trie_.Get(), ss.str(), &symbol, &Symbol::NewInternal);
    LOG_IF(FATAL, !created) << "failed to internalize Symbol: " << ss.str();
    IncrementPoolSize();
  } else {
    return Symbol::NewInternal(ss.str());
  }

  ASSERT(symbol);
  return symbol;
}

void Symbol::Init() {
  InitClass();
  ASSERT(trie_);
  InitNative<proc::gel_get_symbol_pool_size>();
  InitNative<proc::gel_get_symbol_pool_max_size>();
}

#ifdef GEL_DEBUG
namespace proc {
NATIVE_PROCEDURE_F(gel_get_symbol_pool_size) {
  return ReturnLong(GetCurrentThreadSymbolPoolSize());
}

NATIVE_PROCEDURE_F(gel_get_symbol_pool_max_size) {
  return ReturnLong(GetSymbolPoolMaxSize());
}
}  // namespace proc

#endif  // GEL_DEBUG
}  // namespace gel