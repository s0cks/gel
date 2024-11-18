#include "scheme/memory_region.h"
#ifdef OS_IS_OSX

#include <glog/logging.h>
#include <sys/mman.h>
#include <units.h>

#include "scheme/common.h"

#undef MAP_FAILED
#define MAP_FAILED (reinterpret_cast<void*>(-1))

namespace scm {
using namespace units::data;

static inline auto IsMapFailed(const void* ptr) -> bool {
  return ptr == MAP_FAILED;  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

static inline auto GetError() -> std::string {
  return strerror(errno);
}

MemoryRegion::MemoryRegion(const uword size, const ProtectionMode mode) :
  MemoryRegion() {
  const auto total_size = RoundUpPow2(size);
  const auto ptr = mmap(nullptr, total_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  LOG_IF(FATAL, IsMapFailed(ptr)) << "failed to mmap MemoryRegion of " << byte_t(total_size) << ": " << GetError();
  SetStartingAddress((uword)ptr);
  SetSize(size);
}

void MemoryRegion::FreeRegion() {
  if (!IsAllocated())
    return;
  int error = 0;
  if ((error = munmap(GetStartingAddressPointer(), GetSize())) != 0)
    LOG(FATAL) << "failed to munmap " << (*this) << ": " << GetError();
  DVLOG(SCM_VLEVEL_1) << "freed " << (*this);
  SetSize(0);
  SetStartingAddress(0);
}

void MemoryRegion::Protect(const ProtectionMode mode) {
  int protection = PROT_NONE;
  switch (mode) {
    case kReadOnly:
      protection = PROT_READ;
      break;
    case kReadWrite:
      protection = PROT_READ | PROT_WRITE;
      break;
    case kReadExecute:
      protection = PROT_READ | PROT_EXEC;
      break;
    case kReadWriteExecute:
      protection = PROT_READ | PROT_WRITE | PROT_EXEC;
      break;
    case kNoAccess:
    default:
      break;
  }

  int error = 0;
  if ((error = mprotect(GetStartingAddressPointer(), GetSize(), protection)) != 0)
    LOG(FATAL) << "failed to protect " << (*this) << " w/ " << mode;
  DVLOG(SCM_VLEVEL_1) << "changed " << (*this) << " protection to: " << mode;
}
}  // namespace scm

#endif  // OS_IS_OSX