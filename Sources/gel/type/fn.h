#ifndef GEL_FN_H
#define GEL_FN_H

#include "gel/common.h"
#include "gel/hashcode.h"
#include "gel/entry.h"
#include "gel/type/value.h"
#include "gel/heap/pointer.h"
#include "gel/type/type.h"
#include "gel/type/type_traits.h"

namespace gel {
class Fn : public Value {
  friend class Script;
  friend class Runtime;
  friend class Namespace;
  friend class Interpreter;
  DEFINE_NON_COPYABLE_TYPE(Fn);

 private:
  Str* name_ = nullptr;
  Str* docs_ = nullptr;
  Entry* entry_ = nullptr;

  Fn() = default;

 protected:
  // auto VisitPointers(PointerVisitor* vis) -> bool override {
  //   ASSERT(vis);
  //   if (!Visit(symbol_, *vis))
  //     return false;
  //   if (!Visit(docs_, *vis))
  //     return false;
  //   if (!Visit(args_, *vis))
  //     return false;
  //   return true;
  // }
  //
  // auto VisitPointerPointers(PointerPointerVisitor* vis) -> bool override {
  //   ASSERT(vis);
  //   if (!VisitPointerPointer(vis, &symbol_))
  //     return false;
  //   if (!VisitPointerPointer(vis, &docs_))
  //     return false;
  //   if (!VisitPointerPointer(vis, &args_))
  //     return false;
  //   return true;
  // }

 public:
  ~Fn() override = default;

  DEFINE_OPTIONAL_PROPERTY(Str*, Name, name_);
  DEFINE_OPTIONAL_PROPERTY(Str*, Docs, docs_);
  DEFINE_OPTIONAL_PROPERTY(Entry*, Entry, entry_);

  virtual auto IsNative() const -> bool {
    return false;
  }

  inline friend auto operator<<(std::ostream& stream, const Fn& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }
 public:
  static void Init();
};
static_assert(HasDocs<Fn>);
}  // namespace gel

#endif  // GEL_FN_H
