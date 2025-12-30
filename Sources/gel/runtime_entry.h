#ifndef GEL_RUNTIME_ENTRY_H
#define GEL_RUNTIME_ENTRY_H

#include "common.h"
#include "compiled_code.h"
#include "expr/seq_expr.h"

namespace gel {
class RuntimeEntry {
  DEFINE_NON_COPYABLE_TYPE(RuntimeEntry);

 private:
  CompiledCode* code_ = nullptr;
  expr::SeqExpr* body_ = nullptr;

 public:
  RuntimeEntry() = default;
  ~RuntimeEntry() = default;

  auto GetCode() const -> CompiledCode* {
    return code_;
  }

  void SetCode(CompiledCode* rhs) {
    ASSERT(rhs);
    code_ = rhs;
  }

  auto GetBody() const -> expr::SeqExpr* {
    return body_;
  }

  void SetBody(expr::SeqExpr* rhs) {
    ASSERT(rhs);
    body_ = rhs;
  }
};
}  // namespace gel

#endif  // GEL_RUNTIME_ENTRY_H
