#ifndef GEL_ASSEMBLER_BASE_H
#define GEL_ASSEMBLER_BASE_H

#include "gel/common.h"
#include "gel/platform.h"

namespace gel {
class Label {
  friend class Assembler;
  friend class AssemblerTest;
  DEFINE_DEFAULT_COPYABLE_TYPE(Label);

 private:
  word pos_ = 0;

  void BindTo(const word pos) {
    pos_ = -pos - static_cast<word>(kWordSize);
  }

  void LinkTo(const word pos) {
    pos_ = pos + static_cast<word>(kWordSize);
  }

 public:
  explicit Label(const word pos = 0) :
    pos_(pos) {}
  ~Label() = default;

  auto pos() const -> word {
    return pos_;
  }

  auto GetPos() const -> word {
    return IsBound() ? (-pos_ - static_cast<word>(kWordSize)) : (pos_ - static_cast<word>(kWordSize));
  }

  auto GetLinkPos() const -> word {
    return pos_ - static_cast<word>(kWordSize);
  }

  auto IsBound() const -> bool {
    return pos_ < 0;
  }

  auto IsLinked() const -> bool {
    return pos_ > 0;
  }

  friend auto operator<<(std::ostream& stream, const Label& rhs) -> std::ostream& {
    stream << "Label(";
    stream << "pos=" << rhs.GetPos();
    stream << ")";
    return stream;
  }
};

class AssemblerBuffer {
  friend class Assembler;
  static constexpr const uword kDefaultInitSize = 4096;
  DEFINE_DEFAULT_COPYABLE_TYPE(AssemblerBuffer);

 private:
  uword start_ = UNALLOCATED;
  uword current_ = UNALLOCATED;
  uword asize_ = UNALLOCATED;

  inline auto AddressAt(const uword pos) const -> uword {
    ASSERT(pos >= 0 && pos <= GetAllocatedSize());
    return GetStartingAddress() + pos;
  }

  template <typename T>
  inline auto At(const uword pos) const -> T* {
    return (T*)AddressAt(pos);
  }

 public:
  AssemblerBuffer(const uword init_size = kDefaultInitSize);
  ~AssemblerBuffer();

  auto GetStartingAddress() const -> uword {
    return start_;
  }

  auto GetStartingAddressPointer() const -> void* {
    return (void*)GetStartingAddress();  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }

  auto GetCurrentAddress() const -> uword {
    return current_;
  }

  auto GetEndingAddress() const -> uword {
    return GetStartingAddress() + GetAllocatedSize();
  }

  auto GetSize() const -> uword {
    return GetCurrentAddress() - GetStartingAddress();
  }

  auto GetAllocatedSize() const -> uword {
    return asize_;
  }

  template <typename T>
  void Emit(const T rhs) {
    StoreAt<T>(GetCurrentAddress() - GetStartingAddress(), rhs);
    current_ += sizeof(T);
  }

  template <typename T>
  auto LoadAt(const uword pos) const -> T {
    return *At<T>(pos);
  }

  template <typename T>
  void StoreAt(const uword pos, const T rhs) {
    ASSERT(pos >= 0 && pos <= GetAllocatedSize());
    *At<T>(pos) = rhs;
  }

  void Clear() {
    memset(GetStartingAddressPointer(), 0, GetAllocatedSize());
  }
};
}  // namespace gel

#endif  // GEL_ASSEMBLER_BASE_H
