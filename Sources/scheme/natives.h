#ifndef SCM_NATIVES_H
#define SCM_NATIVES_H

#include "scheme/procedure.h"

namespace scm::proc {
class print : public NativeProcedure {
  DEFINE_NON_COPYABLE_TYPE(print);

 public:
  print();
  ~print() override = default;
  auto Apply(Runtime* state) const -> bool override;
};
}  // namespace scm::proc

#endif  // SCM_NATIVES_H
