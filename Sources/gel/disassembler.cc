#include "gel/disassembler.h"

#include <type_traits>

#include "gel/compiled_code.h"
#include "gel/lambda.h"
#include "gel/script.h"
#include "gel/type_traits.h"

namespace gel {
void Disassembler::Disassemble(CompiledCode* code, const std::string& label) {
  ASSERT(code);
  return Disassemble(code->GetRegion(), label);
}
}  // namespace gel