#ifndef GEL_BITVECTOR_H
#define GEL_BITVECTOR_H

#include <cstdlib>

#include "common.h"
#include "platform.h"

namespace gel {
class BitVector {
  DEFINE_NON_COPYABLE_TYPE(BitVector);

 public:
  static const int kBitsPerByte = 8;
  static const int kBitsPerWord = sizeof(word) * 8;

 private:
  word size_;
  word asize_;
  uword* data_;

 public:
  explicit BitVector(const word size) :
    size_(size),
    asize_(1 + ((size - 1) / kBitsPerWord)),
    data_((uword*)malloc(sizeof(uword) * asize_)) {  // NOLINT(cppcoreguidelines-no-malloc,cppcoreguidelines-pro-type-cstyle-cast)
    Clear();
  }
  ~BitVector() {
    free(data_);  // NOLINT(cppcoreguidelines-no-malloc)
  }

  void Clear() {
    for (int i = 0; i < asize_; i++) data_[i] = 0x0;
  }

  void Add(const word i) {
    data_[i / kBitsPerWord] |= static_cast<uword>(1 << (i % kBitsPerWord));
  }

  void Remove(const word i) {
    data_[i / kBitsPerWord] &= ~static_cast<uword>(1 << (i % kBitsPerWord));
  }

  void Intersect(BitVector* other);
  auto Contains(const word i) -> bool;
  auto AddAll(BitVector* from) -> bool;
  auto KillAndAdd(BitVector* kill, BitVector* gen) -> bool;
};
}  // namespace gel

#endif  // GEL_BITVECTOR_H
