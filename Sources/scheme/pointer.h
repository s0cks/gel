#ifndef SCM_POINTER_H
#define SCM_POINTER_H

#include "scheme/common.h"
#include "scheme/platform.h"
#include "scheme/tag.h"

namespace scm {
class Pointer;
class PointerVisitor {
  DEFINE_NON_COPYABLE_TYPE(PointerVisitor);

 protected:
  PointerVisitor() = default;

 public:
  virtual ~PointerVisitor() = default;
  virtual auto Visit(Pointer* ptr) -> bool = 0;
};

class PointerIterator {
  DEFINE_NON_COPYABLE_TYPE(PointerIterator);

 protected:
  PointerIterator() = default;

 public:
  virtual ~PointerIterator() = default;
  virtual auto HasNext() const -> bool = 0;
  virtual auto Next() -> Pointer* = 0;
};

class Pointer {
  friend class NewZone;
  friend class OldZone;
  DEFINE_NON_COPYABLE_TYPE(Pointer);

 private:
  Tag tag_;
  uword forwarding_ = UNALLOCATED;

  explicit Pointer(const Tag& tag) :
    tag_(tag) {}

  void SetForwardingAddress(const uword address) {
    forwarding_ = address;
  }

  void SetTag(const Tag& rhs) {
    tag_ = rhs;
  }

  inline void ClearTag() {
    return SetTag(kInvalidTag);
  }

 public:
  ~Pointer() = default;

  auto GetStartingAddress() const -> uword {
    return (uword)this;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto GetStartingAddressPointer() const -> void* {
    return (void*)GetStartingAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto GetObjectSize() const -> uword {
    return GetTag().GetSize();
  }

  auto GetObjectAddress() const -> uword {
    return GetStartingAddress() + sizeof(Pointer);
  }

  auto GetObjectAddressPointer() const -> void* {
    return (void*)GetObjectAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto GetTotalSize() const -> uword {
    return sizeof(Pointer) + GetObjectSize();
  }

  auto GetEndingAddress() const -> uword {
    return GetStartingAddress() + GetTotalSize();
  }

  auto GetEndingAddressPointer() const -> void* {
    return (void*)GetEndingAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto GetForwardingAddress() const -> uword {
    return forwarding_;
  }

  auto GetForwardingAddressPointer() const -> void* {
    return (void*)GetForwardingAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  inline auto IsForwarding() const -> bool {
    return GetForwardingAddress() != UNALLOCATED;
  }

  auto GetTag() const -> const Tag& {
    return tag_;
  }

  friend auto operator<<(std::ostream& stream, const Pointer& rhs) -> std::ostream& {
    stream << "Pointer(";
    stream << "tag=" << rhs.GetTag() << ", ";
    stream << "starting_address=" << rhs.GetStartingAddress() << ", ";
    stream << "forwarding_address=" << rhs.GetForwardingAddressPointer();
    stream << ")";
    return stream;
  }

 private:
  static inline auto New(const uword address, const uword size) -> Pointer* {
    return new ((void*)address) Pointer(Tag::New(size));  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

 public:
  static inline auto At(const uword address) -> Pointer* {
    return (Pointer*)address;  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }
};
}  // namespace scm

#endif  // SCM_POINTER_H
