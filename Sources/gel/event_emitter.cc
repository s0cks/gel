#include "gel/event_emitter.h"

#include "gel/common.h"
#include "gel/native_procedure.h"
#include "gel/natives.h"
#include "gel/object.h"
#include "gel/runtime.h"
#include "gel/to_string_helper.h"
#include "gel/trie.h"

namespace gel {
void EventEmitter::EventListener::Call(Object* data) {
  ASSERT(data);
  ObjectList args{};
  if (data)
    args.push_back(data);
  return GetRuntime()->Call(callback, args);
}

auto EventEmitter::ToString() const -> std::string {
  ToStringHelper<EventEmitter> helper;
  NOT_IMPLEMENTED(WARNING);  // TODO: implement
  return helper;
}

auto EventEmitter::Equals(Object* rhs) const -> bool {
  if (!rhs || !rhs->IsEventEmitter())
    return false;
  const auto other = rhs->AsEventEmitter();
  ASSERT(other);
  NOT_IMPLEMENTED(WARNING);  // TODO: implement
  return false;
}

auto EventEmitter::HashCode() const -> uword {
  uword hash = 0;
  NOT_IMPLEMENTED(WARNING);  // TODO: implement
  return hash;
}

auto EventEmitter::Compare(Object* rhs) const -> bool {
  ASSERT(rhs);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return -1;
}

auto EventEmitter::CreateClass() -> Class* {
  ASSERT(kClass == nullptr);
  const auto cls = Class::New(Object::GetClass(), kClassName);
  ASSERT(cls);
  using namespace proc;
  cls->AddFunction(event_emitter_on::Get()->GetNative());
  cls->AddFunction(event_emitter_emit::Get()->GetNative());
  return cls;
}

auto EventEmitter::New(const ObjectList& args) -> EventEmitter* {
  ASSERT(args.empty());
  return New();
}

void EventEmitter::Insert(const Path& key, Procedure* callback) {
  auto current = GetRoot();
  for (const auto& c : key) {
    if (current->children.at(c) == nullptr)
      current->children.at(c) = new Node();
    current = current->children.at(c);
  }
  current->epsilon = true;
  EventListener::Append(&current->value, callback);
}

void EventEmitter::On(String* event, Procedure* callback) {
  ASSERT(event && !event->IsEmpty());
  ASSERT(callback);
  return Insert(event->Get(), callback);
}

void EventEmitter::Emit(String* event, Object* data) {
  ASSERT(event && !event->IsEmpty());
  EventListener* listeners = nullptr;
  if (!trie::Search(GetRoot(), event->Get(), &listeners))
    return;
  EventListenerListIterator iter(listeners);
  while (iter.HasNext()) {
    const auto next = iter.Next();
    next->Call(data);
  }
}

void EventEmitter::Init() {
  using namespace proc;
  InitNative<event_emitter_on>();
  InitNative<event_emitter_emit>();
  InitClass();
}

namespace proc {
#define EVENT_EMITTER_PROCEEDURE_F(Name) NATIVE_PROCEDURE_F(event_emitter_##Name)

EVENT_EMITTER_PROCEEDURE_F(emit) {
  REQUIRED_NATIVE_ARG(0, EventEmitter, emitter);
  REQUIRED_NATIVE_ARG(1, String, event);
  OptionalNativeArgument<2> data(args);
  CHECK_NATIVE_ARG(data);
  emitter->Emit(event, data ? data : Null());
  return ReturnNull();
}

EVENT_EMITTER_PROCEEDURE_F(on) {
  REQUIRED_NATIVE_ARG(0, EventEmitter, emitter);
  REQUIRED_NATIVE_ARG(1, String, event);
  REQUIRED_NATIVE_ARG(2, Procedure, callback);
  emitter->On(event, callback);
  return ReturnNull();
}

#undef EVENT_EMITTER_PROCEDURE_F
}  // namespace proc
}  // namespace gel