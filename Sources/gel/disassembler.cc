#include "gel/disassembler.h"

#include "gel/lambda.h"
#include "gel/script.h"

namespace gel {
void Disassembler::DisassembleLambda(std::ostream& stream, Lambda* lambda, LocalScope* parent_scope) {
  ASSERT(lambda);
  ASSERT(parent_scope);
  const auto scope = LocalScope::New(parent_scope);
  ASSERT(scope);
  if (lambda->HasScope())
    LOG_IF(FATAL, !scope->Add(lambda->GetScope())) << "failed to add " << lambda << " scope to current scope.";
  const auto label = lambda->HasSymbol() ? lambda->GetSymbol()->GetFullyQualifiedName() : "lambda";
  Disassembler disassembler(scope);
  disassembler.Disassemble(lambda, label);
  stream << disassembler;
}

void Disassembler::DisassembleScript(std::ostream& stream, Script* script, LocalScope* parent_scope) {
  ASSERT(script);
  ASSERT(parent_scope);
  const auto scope = LocalScope::New(parent_scope);
  ASSERT(scope);
  const auto label = script->HasName() ? script->GetName()->Get() : "Script";
  Disassembler disassembler(scope);
  disassembler.Disassemble(script, label);
  stream << disassembler;
}
}  // namespace gel