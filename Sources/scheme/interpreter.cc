#include "scheme/interpreter.h"

#include <glog/logging.h>

#include "scheme/ast.h"
#include "scheme/common.h"
#include "scheme/instruction.h"

namespace scm {
auto Interpreter::LoadVariable(Variable* var) -> Datum* {
  ASSERT(var);
  DLOG(INFO) << "loading " << var->ToString() << "....";
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return nullptr;
}

static inline auto Add(Datum* lhs, Datum* rhs) -> Datum* {
  ASSERT(lhs);
  ASSERT(rhs);
  return lhs->Add(rhs);
}

auto Interpreter::Execute(EntryInstr* entry) -> Datum* {
  ASSERT(entry);
  InstructionIterator iter(entry->GetFirstInstruction());
  while (iter.HasNext()) {
    const auto current = iter.Next();
    ASSERT(current);
    DLOG(INFO) << "current: " << current->ToString();
    if (current->IsConstantInstr()) {
      const auto value = current->AsConstantInstr()->GetValue();
      Push(value);
      continue;
    } else if (current->IsStoreVariableInstr()) {
      const auto value = Pop();
      ASSERT(value);
      const auto variable = current->AsStoreVariableInstr()->GetVariable();
      ASSERT(variable);
      DLOG(INFO) << "setting " << variable->ToString() << " := " << value->ToString();
      Push(value);
      continue;
    } else if (current->IsLoadVariableInstr()) {
      const auto variable = current->AsLoadVariableInstr()->GetVariable();
      const auto value = LoadVariable(variable);
      Push(value);
      continue;
    } else if (current->IsReturnInstr()) {
      ASSERT(!stack_.empty());
      break;
    } else if (current->IsBinaryOpInstr()) {
      const auto op = current->AsBinaryOpInstr()->GetOp();
      DLOG(INFO) << "op: " << op;
      const auto left = Pop();
      ASSERT(left);
      DLOG(INFO) << "left: " << left;
      const auto right = Pop();
      ASSERT(right);
      DLOG(INFO) << "right: " << right;
      switch (op) {
        case ast::kAddOp:
          Push(Add(left, right));
          continue;
        default:
          LOG(ERROR) << "invalid BinaryOp: " << op;
      }
    }

    LOG(FATAL) << "invalid instruction: " << current->ToString();
    return nullptr;
  }

  ASSERT(!stack_.empty());
  const auto result = Pop();
  ASSERT(result);
  ASSERT(stack_.empty());
  return result;
}
}  // namespace scm