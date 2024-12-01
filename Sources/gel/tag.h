#ifndef GEL_TAG_H
#define GEL_TAG_H

#include <units.h>

#include "gel/bitfield.h"
#include "gel/common.h"
#include "gel/platform.h"

namespace gel {
using RawTag = uword;

static constexpr const RawTag kInvalidTag = 0x0;

class Tag {
  DEFINE_DEFAULT_COPYABLE_TYPE(Tag);

 private:
  enum Layout {
    // references
    kReferencesOffset = 0,
    kBitsForReferences = 16,
    // new bit
    kNewBitOffset = kReferencesOffset + kBitsForReferences,
    // old bit
    kOldBitOffset = kNewBitOffset + 1,
    // marked bit
    kMarkedBitOffset = kOldBitOffset + 1,
    // remembered bit
    kRememberedBitOffset = kMarkedBitOffset + 1,
    // size
    kSizeOffset = kRememberedBitOffset + 1,
    kBitsForSize = 32,

    kTotalNumberOfBits = kBitsForReferences + kBitsForSize + 4,
  };

  template <typename T, const int Pos, const int Size>
  class TagField : public BitField<RawTag, T, Pos, Size> {};
  template <const int Pos>
  class TagBit : public TagField<bool, Pos, 1> {};

 public:
  class ReferencesField : public TagField<uword, kReferencesOffset, kBitsForReferences> {};
  class NewBit : public TagBit<kNewBitOffset> {};
  class OldBit : public TagBit<kOldBitOffset> {};
  class MarkedBit : public TagBit<kMarkedBitOffset> {};
  class RememberedBit : public TagBit<kRememberedBitOffset> {};
  class SizeField : public TagField<uword, kSizeOffset, kBitsForSize> {};

 public:
 private:
  RawTag raw_;

 public:
  constexpr Tag(const RawTag raw = kInvalidTag) :
    raw_(raw) {}
  ~Tag() = default;

  constexpr auto raw() const -> RawTag {
    return raw_;
  }

  constexpr auto IsInvalid() const -> bool {
    return raw() == kInvalidTag;
  }

  constexpr auto GetNumberOfReferences() const -> uword {
    return ReferencesField::Decode(raw());
  }

  constexpr auto IsNew() const -> bool {
    return NewBit::Decode(raw());
  }

  void SetNewBit(const bool value = true) {
    raw_ = NewBit::Update(value, raw());
  }

  inline void ClearNewBit() {
    return SetNewBit(false);
  }

  constexpr auto IsOld() const -> bool {
    return OldBit::Decode(raw());
  }

  void SetOldBit(const bool value = true) {
    raw_ = OldBit::Update(value, raw());
  }

  inline void ClearOldBit() {
    return SetOldBit(false);
  }

  constexpr auto IsMarked() const -> bool {
    return MarkedBit::Decode(raw());
  }

  void SetMarkedBit(const bool value = true) {
    raw_ = MarkedBit::Update(value, raw());
  }

  inline void ClearMarkedBit() {
    return SetMarkedBit(false);
  }

  constexpr auto IsRemembered() const -> bool {
    return RememberedBit::Decode(raw());
  }

  void SetRememberedBit(const bool value = true) {
    raw_ = RememberedBit::Update(value, raw());
  }

  inline void ClearRememberedBit() {
    return SetRememberedBit(false);
  }

  constexpr auto GetSize() const -> uword {
    return SizeField::Decode(raw());
  }

  void SetSize(const uword value) {
    raw_ = SizeField::Update(value, raw());
  }

  inline void ClearSize() {
    return SetSize(0);
  }

  constexpr operator RawTag() const {
    return raw();
  }

  friend auto operator<<(std::ostream& stream, const Tag& rhs) -> std::ostream& {
    stream << "Tag(";
    stream << "num_references=" << rhs.GetNumberOfReferences() << ", ";
    stream << "size=" << units::data::byte_t(static_cast<double>(rhs.GetSize())) << ", ";
    stream << "new=" << rhs.IsNew() << ", ";
    stream << "old=" << rhs.IsOld() << ", ";
    stream << "marked=" << rhs.IsMarked() << ", ";
    stream << "remembered=" << rhs.IsRemembered();
    stream << ")";
    return stream;
  }

 public:
  static inline constexpr auto Invalid() -> Tag {
    return {};
  }

  static inline constexpr auto New(const uword size) -> Tag {
    return kInvalidTag | NewBit::Encode(true) | SizeField::Encode(size);
  }

  static inline constexpr auto Old(const uword size) -> Tag {
    return kInvalidTag | OldBit::Encode(true) | SizeField::Encode(size);
  }
};
}  // namespace gel

#endif  // GEL_TAG_H
