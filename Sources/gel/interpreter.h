#ifndef GEL_INTERPRETER_H
#define GEL_INTERPRETER_H

#include <sstream>
#include <type_traits>

#include "gel/bytecode.h"
#include "gel/common.h"
#include "gel/local_scope.h"
#include "gel/operation_stack.h"
#include "gel/platform.h"
#include "gel/region.h"
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

  auto GetOperationStack() -> OperationStack*;

  inline auto NextBytecode() -> vm::Bytecode {
    const auto next = *((vm::RawBytecode*)current_);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    current_ += sizeof(vm::RawBytecode);
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
    return Long::New(NextWord());
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

  inline auto NextField() -> Field* {
    const auto next_object = NextObjectPointer();
    ASSERT(next_object && next_object->IsField());
    return next_object->AsField();
  }

  auto GetScope() const -> LocalScope*;
  void nop();
  void bt();
  void Pop();
  void Dup();
  void Throw();
  void Lookup(Symbol* rhs);
  void PopLookup();
  void LoadField(Field* field);
  void StoreField(Field* field);
  void Invoke(const vm::Bytecode::Op op);
  void Push(const vm::Bytecode code);
  void LoadLocal(const uword idx);
  void StoreLocal(const uword idx);
  void ExecUnaryOp(const vm::Bytecode code);
  void ExecBinaryOp(const vm::Bytecode code);
  void New(Class* cls, const uword num_args);
  void NewList(const uword length);
  void Cast(Class* cls);
  void CheckInstance(Class* cls);
  void Jump(const uword address);

  inline void Branch(const vm::BranchCondition cond, const uword target) {
    switch (cond) {
      case vm::BranchCondition::kIsTrue:
        return BranchTrue(target);
      case vm::BranchCondition::kIsFalse:
        return BranchFalse(target);
      case vm::BranchCondition::kEquals:
        return BranchEq(target);
      case vm::BranchCondition::kNotEquals:
        return BranchNe(target);
      case vm::BranchCondition::kGreaterThan:
        return BranchGt(target);
      case vm::BranchCondition::kLessThan:
        return BranchLt(target);
      default:
        LOG(FATAL) << "invalid BranchCondition: " << static_cast<word>(cond);
    }
  }

  void BranchEq(const uword address);
  void BranchNe(const uword address);
  void BranchGt(const uword address);
  void BranchLt(const uword address);
  void BranchTrue(const uword address);
  void BranchFalse(const uword address);

  void Throw(Error* error);

  inline void Throw(const std::stringstream& ss) {
    return Throw(Error::New(ss.str()));
  }

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

  template <class T>
  inline void Run(T* target, std::enable_if_t<gel::has_code<T>::value>* = nullptr) {
    ASSERT(target);
    const auto& code = target->GetCode();
    if (!code->IsCompiled()) {
      DLOG(ERROR) << "cannot run " << target << ", target is not compiled.";
      return;
    }
    return Run(code->GetStartingAddress());
  }
};
}  // namespace gel

#endif  // GEL_INTERPRETER_H
