#ifndef SCM_MEMORY_REGION_H
#define SCM_MEMORY_REGION_H

#include <ostream>

#include "scheme/common.h"
#include "scheme/platform.h"
#include "scheme/section.h"

namespace scm {
class MemoryRegion : public Section {
  DEFINE_DEFAULT_COPYABLE_TYPE(MemoryRegion);

 public:
  enum ProtectionMode : int {
    kNoAccess,
    kReadOnly,
    kReadWrite,
    kReadExecute,
    kReadWriteExecute,
  };

  inline friend auto operator<<(std::ostream& stream, const ProtectionMode& mode) -> std::ostream& {
    switch (mode) {
      case MemoryRegion::kNoAccess:
        return stream << "[n/a]";
      case MemoryRegion::kReadOnly:
        return stream << "[ro]";
      case MemoryRegion::kReadWrite:
        return stream << "[rw]";
      case MemoryRegion::kReadExecute:
        return stream << "[r+]";
      case MemoryRegion::kReadWriteExecute:
        return stream << "[rw+]";
      default:
        return stream << "[unknown]";
    }
  }

 public:
  MemoryRegion() = default;
  MemoryRegion(const uword start, const uword size) :
    Section(start, size) {}
  MemoryRegion(const uword size, const ProtectionMode mode = kNoAccess);
  explicit MemoryRegion(const Section& section) :
    Section(section) {}
  ~MemoryRegion() = default;

  virtual void FreeRegion();
  virtual void Protect(const ProtectionMode mode);

  friend auto operator<<(std::ostream& stream, const MemoryRegion& rhs) -> std::ostream& {
    stream << "MemoryRegion(";
    stream << "start=" << rhs.GetStartingAddressPointer() << ", ";
    stream << "size=" << rhs.GetSize();
    stream << ")";
    return stream;
  }
};
}  // namespace scm

#endif  // SCM_MEMORY_REGION_H
