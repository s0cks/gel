#ifndef GEL_MARKER_H
#define GEL_MARKER_H

#include "gel/common.h"
#include "gel/pointer.h"

namespace gel {
class Marker : public PointerVisitor {
  DEFINE_NON_COPYABLE_TYPE(Marker);

 public:
  Marker() = default;
  ~Marker() override = default;
};
}  // namespace gel

#endif  // GEL_MARKER_H
