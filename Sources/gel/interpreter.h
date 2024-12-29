#ifndef GEL_INTERPRETER_H
#define GEL_INTERPRETER_H

#include <type_traits>

#include "gel/bytecode.h"
#include "gel/common.h"
#include "gel/instruction.h"
#include "gel/local_scope.h"
#include "gel/platform.h"
#include "gel/section.h"
#include "gel/stack_frame.h"
#include "gel/type.h"
#include "gel/type_traits.h"

namespace gel {
class Runtime;
class Interpreter {
  friend class Runtime;
  DEFINE_NON_COPYABLE_TYPE(Interpreter);

 private:
  Runtime* runtime_;
  uword current_ = 0;

  inline auto NextBytecode() -> Bytecode {
    const auto next = *((RawBytecode*)current_);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    current_ += sizeof(RawBytecode);
    return next;
  }

  auto NextUWord() -> uword {
    auto next = *((uword*)current_);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    current_ += sizeof(uword);
    return next;
  }

  auto NextWord() -> word {
    auto next = *((word*)current_);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    current_ += sizeof(word);
    return next;
  }

  inline auto NextLong() -> Long* {
    return Long::New(NextUWord());
  }

  inline auto NextAddress() -> uword {
    return NextUWord();
  }

  inline auto NextObjectPointer() -> Object* {
    return (Object*)NextAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto NextClass() -> Class* {
    const auto next_object = NextObjectPointer();
    ASSERT(next_object && next_object->IsClass());
    return next_object->AsClass();
  }

  auto GetScope() const -> LocalScope*;
  void nop();
  void bt();
  void Pop();
  void Dup();
  void Throw();
  void Lookup(Symbol* rhs);
  void PopLookup();
  void Invoke(const Bytecode::Op op);
  void Push(const Bytecode code);
  void LoadLocal(const uword idx);
  void StoreLocal(const uword idx);
  void ExecUnaryOp(const Bytecode code);
  void ExecBinaryOp(const Bytecode code);
  void New(Class* cls, const uword num_args);
  void Cast(Class* cls);
  void CheckInstance(Class* cls);
  void Jump(const Bytecode code, const uword address);

 protected:
  explicit Interpreter(Runtime* runtime) :
    runtime_(runtime) {}

  inline auto GetCurrentAddress() const -> uword {
    return current_;
  }

  inline void SetCurrentAddress(const uword address) {
    current_ = address;
  }

  inline void SetRegion(const Region& rhs) {
    SetCurrentAddress(rhs.GetStartingAddress());
  }

 public:
  virtual ~Interpreter() = default;
  void Run(const uword address);
};
}  // namespace gel

#endif  // GEL_INTERPRETER_H
