#include "gel/assembler.h"
#include "gel/common.h"
#include "gel/disassembler.h"
#include "gel/local.h"
#include "gel/object.h"
#include "gel/platform.h"

namespace gel {
void Disassembler::WritePrefix(const uword address, const uword pos) {
  static constexpr const auto kMaxTensPlaces = 4;
  if (ShouldShowInstrAddress()) {
    stream() << ((void*)address);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
    if (ShouldShowInstrOffset())
      stream() << " ";
  }
  if (ShouldShowInstrOffset()) {
    stream() << std::string(abs(static_cast<long>(kMaxTensPlaces - static_cast<uword>(floor(log10(pos))))), ' ');
    stream() << pos;
  }
  if (ShouldShowInstrAddress() || ShouldShowInstrOffset())
    stream() << ":";
}

void Disassembler::WriteLabel(const char* label) {
  static constexpr const auto kPrefixLength = 17;
  ASSERT(ShouldShowLabels());
  const auto len = strlen(label);
  ASSERT(label && len > 0);
  if (len <= kPrefixLength)  // left-pad
    stream() << std::string(kPrefixLength - len, ' ');
  stream() << label << ":" << std::endl;
}

static inline void WriteRightPad(std::ostream& stream, const int num) {
  stream << std::string(std::max(0, num), ' ');
}

auto Disassembler::Comment() -> std::ostream& {
  const auto len = (stream().tellp() - instr_startp_);
  const auto remain = static_cast<int>(kDisassemblyMaxLength - len);
  WriteRightPad(stream(), remain);
  return stream() << ";; ";
}

void Disassembler::Disassemble(const Region& region, const char* label) {
  stream() << std::endl;
  if (ShouldShowLabels() && label && (strlen(label) > 0))
    WriteLabel(label);
  BytecodeDecoder decoder(region);
  while (decoder.HasNext()) {
    const auto ipos = decoder.GetPos();
    WritePrefix(decoder.GetCurrentAddress(), ipos);
    instr_startp_ = stream().tellp();
    const auto op = decoder.NextBytecode();
    Mnemonic(op);
    switch (op.op()) {
      case Bytecode::kPushQ: {
        const auto value = decoder.NextObjectPointer();
        ASSERT(value);
        Pointer(value);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
        break;
      }
      case Bytecode::kPushI: {
        const auto value = decoder.NextAddress();
        stream() << value;
        break;
      }
      case Bytecode::kLoadLocal: {
        const auto index = decoder.NextAddress();
        const auto local = GetScope()->GetLocalAt(index);
        ASSERT(local);
        Local((*local));
        break;
      }
      case Bytecode::kLoadLocal0:
      case Bytecode::kLoadLocal1:
      case Bytecode::kLoadLocal2:
      case Bytecode::kLoadLocal3: {
        const auto index = (op.op() - Bytecode::kLoadLocal0);
        if (GetScope()->IsEmpty() || index > GetScope()->GetNumberOfLocals())
          break;
        const auto local = GetScope()->GetLocalAt(index);
        ASSERT(local);
        Local((*local), false);
        break;
      }
      case Bytecode::kStoreLocal: {
        const auto index = decoder.NextAddress();
        if (GetScope()->IsEmpty() || index > GetScope()->GetNumberOfLocals())
          break;
        const auto local = GetScope()->GetLocalAt(index);
        ASSERT(local);
        Local((*local));
        break;
      }
      case Bytecode::kStoreLocal0:
      case Bytecode::kStoreLocal1:
      case Bytecode::kStoreLocal2:
      case Bytecode::kStoreLocal3: {
        const auto index = (op.op() - Bytecode::kStoreLocal0);
        if (GetScope()->IsEmpty() || index > GetScope()->GetNumberOfLocals())
          break;
        const auto local = GetScope()->GetLocalAt(index);
        ASSERT(local);
        Local((*local), false);
        break;
      }
      case Bytecode::kJump:
      case Bytecode::kJz:
      case Bytecode::kJnz:
      case Bytecode::kJeq:
      case Bytecode::kJne: {
        const auto offset = decoder.NextWord();
        WriteOffset(static_cast<int32_t>(offset));
        Comment(static_cast<uint32_t>(ipos + offset));
        break;
      }
      case Bytecode::kCheckInstance: {
        const auto cls = decoder.NextObjectPointer();
        ASSERT(cls && cls->IsClass());
        Pointer(cls);
        break;
      }
      case Bytecode::kCast: {
        const auto cls = decoder.NextObjectPointer();
        ASSERT(cls && cls->IsClass());
        Pointer(cls);
        break;
      }
      case Bytecode::kNew: {
        const auto cls = decoder.NextObjectPointer();
        ASSERT(cls && cls->IsClass());
        Comment(cls) << ", num_args=" << decoder.NextUWord();
        break;
      }
      case Bytecode::kInvokeNative:
      case Bytecode::kInvokeDynamic: {
        const auto num_args = decoder.NextUWord();
        Comment() << "num_args=" << num_args;
        break;
      }
      default:
        break;
    }
    stream() << std::endl;
  }
  stream() << std::endl;
}
}  // namespace gel