#include "gel/common.h"
#include "gel/platform.h"
#ifndef GEL_DISASSEMBLER_H
#error "Please #include <gel/disassembler.h> instead."
#endif  // GEL_DISASSEMBLER_H

#ifndef GEL_DISASSEMBLER_VM_H
#define GEL_DISASSEMBLER_VM_H

#include "gel/bytecode.h"
#include "gel/section.h"

namespace gel {
class BytecodeDecoder {
  DEFINE_NON_COPYABLE_TYPE(BytecodeDecoder);

 private:
  Region region_;
  uword current_;

 public:
  BytecodeDecoder(const Region& region) :
    region_(region),
    current_(region.GetStartingAddress()) {}
  ~BytecodeDecoder() = default;

  auto region() const -> const Region& {
    return region_;
  }

  auto GetCurrentAddress() const -> uword {
    return current_;
  }

  auto GetPos() const -> uword {
    return (GetCurrentAddress() - region().GetStartingAddress());
  }

  auto HasNext() const -> bool {
    return (GetCurrentAddress() + sizeof(RawBytecode)) <= region_.GetEndingAddress();
  }

  auto NextBytecode() -> Bytecode {
    const auto next = *((RawBytecode*)GetCurrentAddress());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    current_ += sizeof(RawBytecode);
    return next;
  }

  auto NextUWord() -> uword {
    auto next = *((uword*)current_);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    current_ += sizeof(uword);
    return next;
  }

  auto NextWord() -> word {
    const auto next = *((word*)current_);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    current_ += sizeof(word);
    return next;
  }

  inline auto NextLong() -> uword {
    return NextUWord();
  }

  inline auto NextAddress() -> uword {
    return NextUWord();
  }

  inline auto NextObjectPointer() -> Object* {
    return ((Object*)NextAddress());  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
  }
};
}  // namespace gel

#endif  // GEL_DISASSEMBLER_VM_H
