#include "gel/bitvector.h"

namespace gel {
void BitVector::Intersect(BitVector* rhs) {
  for (int i = 0; i < asize_; i++) {
    data_[i] = data_[i] & rhs->data_[i];
  }
}

auto BitVector::Contains(const word i) -> bool {
  uword block = data_[i / kBitsPerWord];
  return (block & static_cast<uword>(1 << (i % kBitsPerWord))) != 0;
}

auto BitVector::AddAll(BitVector* from) -> bool {
  bool changed = false;

  for (int i = 0; i < asize_; i++) {
    uword before = data_[i];
    uword after = data_[i] | from->data_[i];
    if (before != after) {
      changed = true;
      data_[i] = after;
    }
  }

  return changed;
}

auto BitVector::KillAndAdd(BitVector* kill, BitVector* gen) -> bool {
  bool changed = false;

  for (int i = 0; i < asize_; i++) {
    uword before = data_[i];
    uword after = data_[i] | (gen->data_[i] & ~kill->data_[i]);
    if (before != after) {
      changed = true;
      data_[i] = after;
    }
  }

  return changed;
}
}  // namespace gel