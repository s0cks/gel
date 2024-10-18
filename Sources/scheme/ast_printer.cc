#include "scheme/ast_printer.h"

#include "scheme/ast.h"
#include "scheme/common.h"

namespace scm::ast {
#define __ (google::LogMessage(GetFile(), GetLine(), GetSeverity())).stream() << GetIndentString()

auto AstPrinter::AstPrinter::VisitSyntaxDef(SyntaxDef* defn) -> bool {
  ASSERT(defn);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto AstPrinter::VisitConstantExpr(ConstantExpr* expr) -> bool {
  ASSERT(expr);
  __ << expr->ToString();
  return true;
}

auto AstPrinter::VisitBeginDef(BeginDef* defn) -> bool {
  __ << "BeginDef()";
  Indent();
  {
    if (!defn->VisitChildren(this))
      return false;
  }
  Deindent();
  return true;
}

auto AstPrinter::VisitQuoteExpr(QuoteExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto AstPrinter::VisitLoadVariableExpr(LoadVariableExpr* expr) -> bool {
  ASSERT(expr);
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return true;
}

auto AstPrinter::VisitProgram(Program* p) -> bool {
  ASSERT(p);
  __ << "Program()";
  Indent();
  if (!p->VisitAllForms(this))
    return false;
  Deindent();
  return true;
}

auto AstPrinter::VisitBody(Body* body) -> bool {
  ASSERT(body);
  __ << "Body()";
  return true;
}

auto AstPrinter::VisitVariableDef(VariableDef* defn) -> bool {
  ASSERT(defn);
  __ << "VariableDef()";
  Indent();
  __ << "var=" << defn->GetVar()->ToString();
  __ << "val=" << defn->GetVal()->ToString();
  Deindent();
  return true;
}

auto AstPrinter::VisitCallProcExpr(CallProcExpr* expr) -> bool {
  __ << "CallProcExpr()";
  return true;
}

auto AstPrinter::VisitExpressionList(ExpressionList* expressions) -> bool {
  __ << "ExpressionList()";
  return true;
}

auto AstPrinter::VisitBinaryOpExpr(BinaryOpExpr* expr) -> bool {
  ASSERT(expr);
  __ << expr->ToString();
  return true;
}

#undef __

}  // namespace scm::ast