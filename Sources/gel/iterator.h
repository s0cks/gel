#ifndef GEL_ITERATOR_H
#define GEL_ITERATOR_H

#include "common.h"
#include "native_procedure.h"
#include "object.h"
#include "set.h"

namespace gel {
class Iterator : public Object {
 public:
  Iterator() = default;
  ~Iterator() override = default;

  virtual auto Next() -> Object* = 0;
  virtual auto HasNext() const -> bool = 0;
  DECLARE_TYPE(Iterator);

 private:
  static void Init();
};

class SetIterator : public Iterator {
  using Delegate = Set::StorageType::const_iterator;
  DEFINE_NON_COPYABLE_TYPE(SetIterator);

 private:
  Delegate current_{};
  Delegate end_{};

 public:
  explicit SetIterator(Set* value) :
    Iterator() {
    if (value) {
      current_ = value->begin();
      end_ = value->end();
    }
  }
  ~SetIterator() override = default;

  auto Next() -> Object* override {
    const auto next = (*current_);
    current_++;
    return next;
  }

  auto HasNext() const -> bool override {
    return current_ != end_;
  }
};

namespace proc {
#define _DECLARE_ITERATOR_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(iter_##Name, "Iterator:" Sym)
#define DECLARE_ITERATOR_PROCEDURE(Name)       _DECLARE_ITERATOR_PROCEDURE(Name, #Name);

_DECLARE_ITERATOR_PROCEDURE(has_next, "has-next?");
DECLARE_ITERATOR_PROCEDURE(next);

#undef _DECLARE_ITERATOR_PROCEDURE
#undef DECLARE_ITERATOR_PROCEDURE
}  // namespace proc
}  // namespace gel

#endif  // GEL_ITERATOR_H
