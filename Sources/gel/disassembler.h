#ifndef GEL_DISASSEMBLER_H
#define GEL_DISASSEMBLER_H

#include "gel/common.h"
#include "gel/instruction.h"

namespace gel {
class Disassembler : public instr::InstructionVisitor {
  DEFINE_NON_COPYABLE_TYPE(Disassembler);

 public:
  Disassembler() = default;
  ~Disassembler() override = default;
#define DECLARE_VISIT(Name) auto Visit##Name(Name* instr)->bool override;
  FOR_EACH_INSTRUCTION(DECLARE_VISIT)
#undef DECLARE_VISIT
 public:
  static auto Disassemble(GraphEntryInstr* instr) -> bool;
};
}  // namespace gel

#endif  // GEL_DISASSEMBLER_H
