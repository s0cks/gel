#ifndef GEL_RUNTIME_ENTRY_H
#define GEL_RUNTIME_ENTRY_H

#include "gel/common.h"
#include "gel/entry.h"
#include "gel/vm/compiled_code.h"
#include "gel/frontend/expr/expr.h"

namespace gel {
class RuntimeEntry : public Entry {
  DEFINE_DEFAULT_COPYABLE_TYPE(RuntimeEntry);
private:
  expr::Expression* body_ = nullptr;
  CompiledCode* code_ = nullptr;
public:
  RuntimeEntry() = default;
  ~RuntimeEntry() = default;

  auto GetBody() const -> expr::Expression* {
    return body_;
  }

  inline auto HasBody() const -> bool {
    return GetBody() != nullptr;
  }

  void SetBody(expr::Expression* rhs) {
    ASSERT(rhs);
    body_ = rhs;
  }

  inline void RemoveBody() {
    body_ = nullptr;
  }

  auto GetCode() const -> CompiledCode* {
    return code_;
  }

  inline auto HasCode() const -> bool {
    return GetCode() != nullptr;
  }

  void SetCode(CompiledCode* rhs) {
    ASSERT(rhs);
    code_ = rhs;
  }

  inline void RemoveCode() {
    code_ = nullptr;
  }

  auto ToString() const -> std::string override;

  auto AsRuntimeEntry() -> RuntimeEntry* override {
    return this;
  }

  inline friend auto operator<<(std::ostream& stream, const RuntimeEntry& rhs) -> std::ostream& {
    return stream << rhs.ToString();
  }
};
}

#endif // GEL_RUNTIME_ENTRY_H
