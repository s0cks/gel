#ifndef SCM_MARKER_H
#define SCM_MARKER_H

#include "scheme/common.h"
#include "scheme/pointer.h"

namespace scm {
class Marker : public PointerVisitor {
  DEFINE_NON_COPYABLE_TYPE(Marker);

 public:
  Marker() = default;
  ~Marker() override = default;
};
}  // namespace scm

#endif  // SCM_MARKER_H
