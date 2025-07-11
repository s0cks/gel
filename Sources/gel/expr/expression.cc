#include "gel/expr/expression.h"

#include <algorithm>
#include <glog/logging.h>
#include <sstream>
#include <string>

#include "gel/common.h"
#include "gel/expr/exprs.h"
#include "gel/heap.h"
#include "gel/local.h"
#include "gel/map.h"
#include "gel/module.h"
#include "gel/natives.h"
#include "gel/object.h"
#include "gel/runtime.h"
#include "gel/to_string_helper.h"

namespace gel::expr {
Class* Expression::kClass = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
void Expression::Init() {
  ASSERT(kClass == nullptr);
  kClass = Class::New(Object::GetClass(), kClassName);
  ASSERT(kClass);
}

FOR_EACH_EXPRESSION_NODE(DEFINE_NEW_OPERATOR)  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
#undef DEFINE_NEW_OPERATOR

#define DEFINE_ACCEPT(Name)                           \
  auto Name::Accept(ExpressionVisitor& vis) -> bool { \
    return vis.Visit##Name(this);                     \
  }
FOR_EACH_EXPRESSION_NODE(DEFINE_ACCEPT)
#undef DEFINE_ACCEPT

// auto RxOpExpr::ToString() const -> std::string {
//   ToStringHelper<RxOpExpr> helper;
//   helper.AddField("symbol", GetSymbol());
//   helper.AddField("args", GetBody());
//   return helper;
// }

// auto RxOpExpr::IsSubscribe() const -> bool {
//   return IsCallToNative<proc::rx_subscribe>(GetSymbol());
// }

// auto RxOpExpr::IsComplete() const -> bool {
//   return IsCallToNative<proc::rx_complete>(GetSymbol());
// }

// auto LetRxExpr::ToString() const -> std::string {
//   ToStringHelper<LetRxExpr> helper;
//   helper.AddField("scope", GetScope());
//   helper.AddField("source", GetSource());
//   helper.AddField("body", GetBody());
//   return helper;
// }

// auto LetRxExpr::HasSubscribe() const -> bool {
//   const auto last = GetLastOp();
//   if (!last)
//     return last;
//   return last->IsSubscribe();
// }

// auto LetRxExpr::HasComplete() const -> bool {
//   const auto last = GetLastOp();
//   if (!last)
//     return last;
//   return last->IsComplete();
// }

// auto InstanceOfExpr::ToString() const -> std::string {
//   ToStringHelper<InstanceOfExpr> helper;
//   helper.AddField("target", GetTarget());
//   helper.AddField("value", GetValue());
//   return helper;
// }

// auto InstanceOfExpr::EvalToConstant(LocalScope* scope) const -> Object* {
//   ASSERT(IsConstantExpr());
//   const auto value = GetValue()->EvalToConstant(scope);
//   ASSERT(value);
//   DLOG(INFO) << "checking " << GetValue() << " is an instanceof " << GetTarget();
//   return Bool::Box(value->GetType()->IsInstanceOf(GetTarget()));
// }

// auto InstanceOfExpr::IsConstantExpr() const -> bool {
//   return GetValue()->IsConstantExpr();
// }

// auto ListExpr::ToString() const -> std::string {
//   ToStringHelper<ListExpr> helper;
//   helper.AddField("values", GetBody());
//   return helper;
// }

// auto ListExpr::IsConstantExpr() const -> bool {
//   if (IsEmpty())
//     return true;
//   for (auto idx = 0; idx < GetNumberOfChildren(); idx++) {
//     const auto child = GetChildAt(idx);
//     ASSERT(child);
//     if (!child->IsConstantExpr())
//       return false;
//   }
//   return true;
// }

// auto CastExpr::ToString() const -> std::string {
//   ToStringHelper<CastExpr> helper;
//   helper.AddField("target", GetTargetType());
//   helper.AddField("value", GetValue());
//   return helper;
// }

// auto ListExpr::EvalToConstant(LocalScope* scope) const -> Object* {
//   ASSERT(IsConstantExpr());
//   if (IsEmpty())
//     return Pair::Empty();
//   Object* value = Pair::Empty();
//   for (auto idx = GetNumberOfChildren(); idx > 0; idx--) {
//     const auto child = GetChildAt(idx - 1);
//     ASSERT(child && child->IsConstantExpr());
//     value = gel::Cons(child->EvalToConstant(scope), value);
//   }
//   return value;
// }

static inline auto IsConstantExpr(const NewMapExpr::Entry& entry) -> bool {
  return entry.first && entry.second->IsConstantExpr();
}

auto NewMapExpr::IsConstantExpr() const -> bool {
  for (const auto& entry : data_) {
    if (!expr::IsConstantExpr(entry))
      return false;
  }
  return true;
}

auto NewMapExpr::EvalToConstant(LocalScope* scope) const -> Object* {
  ASSERT(scope);
  Map::StorageType data{};
  for (const auto& entry : data_) {
    ASSERT(entry.first && entry.second);
    const auto value = entry.second->EvalToConstant(scope);
    ASSERT(value);
    data.insert({entry.first, value});  // TODO: prolly should check this insertion
  }
  return Map::New(data);
}

auto NewMapExpr::ToString() const -> std::string {
  ToStringHelper<NewMapExpr> helper;
  helper.AddField("num_entries", GetNumberOfChildren());
  return helper;
}

auto NewMapExpr::VisitChildren(ExpressionVisitor& vis) -> bool {
  for (const auto& e : data()) {
    ASSERT(e.first && e.second);
    if (!e.second->Accept(vis))
      return false;
  }
  return true;
}
}  // namespace gel::expr