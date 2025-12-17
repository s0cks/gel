#include "gel/frontend/expr/expr.h"

#include <algorithm>
#include <glog/logging.h>
#include <sstream>
#include <string>

#include "gel/common.h"
#include "gel/frontend/expr/exprs.h"
#include "gel/heap/heap.h"
#include "gel/local.h"
#include "gel/module.h"
#include "gel/natives.h"
#include "gel/type/object.h"
#include "gel/runtime.h"
#include "gel/to_string_helper.h"

namespace gel::expr {
void Expression::Init() {
  // TODO: implement @s0cks
}

#define DEFINE_ACCEPT(Name)                           \
  auto Name::Accept(ExpressionVisitor& vis) -> bool { \
    return vis.Visit##Name(this);                     \
  }
FOR_EACH_EXPRESSION_NODE(DEFINE_ACCEPT)
#undef DEFINE_ACCEPT
}
