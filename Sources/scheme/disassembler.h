#ifndef SCM_DISASSEMBLER_H
#define SCM_DISASSEMBLER_H

#include "scheme/common.h"
#include "scheme/instruction.h"

namespace scm {
class Disassembler : public instr::InstructionVisitor {
  DEFINE_NON_COPYABLE_TYPE(Disassembler);

 public:
  Disassembler() = default;
  ~Disassembler() override = default;
#define DECLARE_VISIT(Name) auto Visit##Name(Name* instr) -> bool override;
  FOR_EACH_INSTRUCTION(DECLARE_VISIT)
#undef DECLARE_VISIT
 public:
  static auto Disassemble(GraphEntryInstr* instr) -> bool;
};
}  // namespace scm

#endif  // SCM_DISASSEMBLER_H
