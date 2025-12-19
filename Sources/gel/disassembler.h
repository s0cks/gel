#ifndef GEL_DISASSEMBLER_H
#define GEL_DISASSEMBLER_H

#include <concepts>
#include <string_view>
#include <type_traits>

#include "common.h"
#include "compiled_code.h"
#include "disassembler_vm.h"
#include "hashcode.h"
#include "local.h"
#include "local_scope.h"
#include "type_traits.h"

namespace gel {
template <class T>
concept HasFullyQualifiedName = requires(T value) {
  { value.GetFullyQualifiedName() } -> std::convertible_to<std::string>;
};

template <class T>
concept DisassemblerTarget = HasCompiledCode<T> && HasFullyQualifiedName<T>;

class CompiledCode;
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
  std::stringstream stream_{};
  LocalScope* scope_;
  std::ostream::pos_type instr_startp_;

  inline auto stream() -> std::stringstream& {
    return stream_;
  }

  void WriteLabel(const std::string_view label);
  void WritePrefix(const uword address, const uword pos);

  inline void WriteOffset(int32_t rhs) {
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

  inline auto Comment(const std::string rhs) -> std::ostream& {
    return Comment() << rhs;
  }

  inline auto Comment(const LocalVariable& rhs) -> std::ostream& {
    if (rhs.HasValue())
      return PrintValue(Comment(rhs.GetSymbol()) << " idx=" << rhs.GetIndex(), rhs.GetValue());
    return Comment(rhs.GetSymbol()) << " idx=" << rhs.GetIndex();
  }

  inline auto Comment(const uint32_t rhs) -> std::ostream& {
    return Comment() << "#" << rhs;
  }

  inline void Pointer(Object* rhs) {
    ASSERT(rhs);
    stream() << rhs->GetStartingAddressPointer();
    Comment(rhs);
  }

  void Invoke(BytecodeDecoder& decoder, const Bytecode::Op op);

 public:
  Disassembler(Config config, LocalScope* scope) :
    config_(config),
    scope_(scope) {}
  Disassembler(LocalScope* scope) :
    Disassembler(kDefaultConfig, scope) {}
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

  auto stream() const -> const std::stringstream& {
    return stream_;
  }

  inline auto str() const -> std::string {
    return stream().str();
  }

  void Disassemble(const Region region, const std::string_view label);

  friend auto operator<<(std::ostream& stream, const Disassembler& rhs) -> std::ostream& {
    return stream << rhs.stream().rdbuf();
  }

 private:
  template <DisassemblerTarget Target>
  static inline void CreateLabel(const Target& target, std::string& label) {
    std::stringstream ss{};
    ss << target.GetFullyQualifiedName();
#ifdef GEL_DEBUG
    ss << " " << target.GetType()->GetName()->Get();
#endif  // GEL_DEBUG
    label = ss.str();
  }

 public:
  template <DisassemblerTarget Target>
  static inline void Disassemble(std::ostream& stream, const Target& target, LocalScope* parent_scope = nullptr,
                                 const std::string_view prefix = "", const std::string_view suffix = "") {
    if (!target.IsCompiled()) {
      stream << target << " is not compiled.";
      return;
    }
    const auto scope = LocalScope::New(parent_scope);
    ASSERT(scope);
    if (target.HasScope())
      scope->AddAll(target.GetScope());
    Disassembler disassembler(scope);
    std::string label{};
    CreateLabel(target, label);
    disassembler.Disassemble(*target.GetCode(), label);
    stream << std::endl << prefix << std::endl;
    stream << disassembler;
    stream << std::endl << suffix << std::endl;
  }
};
}  // namespace gel

#endif  // GEL_DISASSEMBLER_H
