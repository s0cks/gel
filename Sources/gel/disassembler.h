#ifndef GEL_DISASSEMBLER_H
#define GEL_DISASSEMBLER_H

#include "gel/common.h"
#include "gel/disassembler_vm.h"
#include "gel/local.h"
#include "gel/local_scope.h"

namespace gel {
class Disassembler {
  DEFINE_NON_COPYABLE_TYPE(Disassembler);

 public:
  static constexpr const auto kDisassemblyMaxLength = 64;
  struct Config {
    bool show_labels;
    bool show_instr_addr;
    bool show_instr_offset;
    bool show_comments;
  };
  static constexpr const Config kDefaultConfig = {
      .show_labels = true,
      .show_instr_addr = true,
      .show_instr_offset = true,
      .show_comments = true,
  };

 private:
  Config config_;
  std::ostream& stream_;
  LocalScope* scope_;
  std::ostream::pos_type instr_startp_;

  inline auto stream() const -> std::ostream& {
    return stream_;
  }

  void WriteLabel(const char* label);
  void WritePrefix(const uword address, const uword pos);

  inline void WriteOffset(int32_t rhs) {
    if (rhs < 0)
      stream() << "-";
    else
      stream() << "+";
    stream() << rhs;
  }

  inline auto LocalIndex(const uword idx) -> std::ostream& {
    return stream() << "#" << idx;
  }

  inline auto Local(const LocalVariable& rhs, const bool write_index = true) -> std::ostream& {
    if (write_index)
      LocalIndex(rhs.GetIndex());
    return Comment(rhs);
  }

  inline auto Mnemonic(const Bytecode& rhs) -> std::ostream& {
    return stream() << " " << rhs.mnemonic() << " ";
  }

  auto Comment() -> std::ostream&;

  inline auto Comment(Object* rhs) -> std::ostream& {
    return PrintValue(Comment(), rhs);
  }

  inline auto Comment(const std::string& rhs) -> std::ostream& {
    return Comment() << rhs;
  }

  inline auto Comment(const LocalVariable& rhs) -> std::ostream& {
    if (rhs.HasValue())
      return Comment(rhs.GetValue());
    return Comment(rhs.GetName());
  }

  inline auto Comment(const uint32_t rhs) -> std::ostream& {
    return Comment() << "#" << rhs;
  }

  inline void Pointer(Object* rhs) {
    ASSERT(rhs);
    stream() << rhs->GetStartingAddressPointer();
    Comment(rhs);
  }

 public:
  Disassembler(Config config, std::ostream& stream, LocalScope* scope) :
    config_(config),
    stream_(stream),
    scope_(scope) {}
  Disassembler(std::ostream& stream, LocalScope* scope) :
    Disassembler(kDefaultConfig, stream, scope) {}
  ~Disassembler() = default;

  auto config() const -> const Config& {
    return config_;
  }

  inline auto ShouldShowLabels() const -> bool {
    return config().show_labels;
  }

  inline auto ShouldShowInstrAddress() const -> bool {
    return config().show_instr_addr;
  }

  inline auto ShouldShowInstrOffset() const -> bool {
    return config().show_instr_offset;
  }

  inline auto ShouldShowComments() const -> bool {
    return config().show_comments;
  }

  auto GetScope() const -> LocalScope* {
    return scope_;
  }

  void Disassemble(const Region& region, const char* label = nullptr);
};
}  // namespace gel

#endif  // GEL_DISASSEMBLER_H
