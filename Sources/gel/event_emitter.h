#ifndef GEL_EVENT_EMITTER_H
#define GEL_EVENT_EMITTER_H

#include <string>

#include "common.h"
#include "native_procedure.h"
#include "object.h"
#include "pair.h"
#include "procedure.h"
#include "trie.h"

namespace gel {
class EventEmitter : public Object {
 public:
  struct EventListener {
    DEFINE_DEFAULT_COPYABLE_TYPE(EventListener);

   public:
    EventListener* next = nullptr;
    Fn* callback;

    explicit EventListener(Fn* func) :
      callback(func) {}
    ~EventListener() = default;

    void Call(Object* data);

   public:
    static void Append(EventListener** head, Fn* func) {
      ASSERT(func);
      const auto listener = new EventListener(func);
      ASSERT(listener);
      if ((*head) == nullptr) {
        (*head) = listener;
        return;
      }

      auto last = (*head);
      while (last->next != nullptr)
        last = last->next;
      last->next = listener;
    }
  };

  class EventListenerListIterator {
    DEFINE_NON_COPYABLE_TYPE(EventListenerListIterator);

   private:
    EventListener* current_;

   public:
    EventListenerListIterator(EventListener* head) :
      current_(head) {}
    ~EventListenerListIterator() = default;

    auto HasNext() const -> bool {
      return current_ != nullptr;
    }

    auto Next() -> EventListener* {
      const auto next = current_;
      current_ = current_->next;
      return next;
    }
  };

  static constexpr const auto kAlphabetSize = 127;
  using Path = std::string;
  using Node = trie::Node<Path, EventListener*, kAlphabetSize>;

 private:
  Node* root_;

  explicit EventEmitter(Node* root = new Node()) :
    Object(),
    root_(root) {
    ASSERT(root_);
  }

  auto GetRoot() const -> Node* {
    return root_;
  }

  void Insert(const Path& path, Fn* callback);

 public:
  ~EventEmitter() override = default;

  void On(String* event, Fn* callback);
  void Emit(String* event, Object* data = Nil::Get());

  DECLARE_TYPE(EventEmitter);

 private:
  static void Init();

 public:
  static inline auto New() -> EventEmitter* {
    return new EventEmitter();
  }
};

namespace proc {
#define _DECLARE_EVENT_EMITTER_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(event_emitter_##Name, "EventEmitter:" Sym)
#define DECLARE_EVENT_EMITTER_PROCEDURE(Name)       _DECLARE_EVENT_EMITTER_PROCEDURE(Name, #Name);

DECLARE_EVENT_EMITTER_PROCEDURE(on);
DECLARE_EVENT_EMITTER_PROCEDURE(emit);

#undef _DECLARE_EVENT_EMITTER_PROCEDURE
#undef DECLARE_EVENT_EMITTER_PROCEDURE
}  // namespace proc
}  // namespace gel

#endif  // GEL_EVENT_EMITTER_H
