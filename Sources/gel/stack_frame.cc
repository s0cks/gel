#include "gel/stack_frame.h"

#include <exception>
#include <set>

#include "gel/local_scope.h"
#include "gel/native_procedure.h"
#include "gel/object.h"
#include "gel/pointer.h"
#include "gel/runtime.h"
#include "gel/script.h"
#include "gel/to_string_helper.h"

namespace gel {
auto StackFrame::VisitAllPointerPointers(const std::function<bool(Pointer**)>& vis) -> bool {
  auto target = GetTarget()->raw_ptr();
  if (!vis(&target))
    return false;
  if (!GetTarget()->raw_ptr()->Equals(target))
    target_ = target->GetObjectPointer();
  ASSERT(target_);

  if (HasReturnAddress()) {
    auto ret = GetReturnObjectPointer()->raw_ptr();
    if (!vis(&ret))
      return false;
    if (!GetReturnObjectPointer()->raw_ptr()->Equals(ret))
      return_address_ = ret->GetObjectPointer()->GetStartingAddress();
  }

  // TODO: visit locals
  return true;
}

auto StackFrame::VisitAllPointerPointers(PointerPointerVisitor* vis) -> bool {
  ASSERT(vis);
  auto target = GetTarget()->raw_ptr();
  if (!vis->Visit(&target))
    return false;
  if (!GetTarget()->raw_ptr()->Equals(target))
    target_ = target->GetObjectPointer();

  // OperationStack::Iterator iter(stack_);
  // while (iter.HasNext()) {
  //   const auto next = iter.Next();
  //   ASSERT(next);
  // }
  // TODO: visit locals
  return true;
}

auto StackFrame::GetTargetName() const -> std::string {
  if (IsScriptFrame()) {
    return "Script";  // TODO: implement
  } else if (IsNativeFrame()) {
    return GetTarget()->AsNativeFn()->GetSymbol()->GetFullyQualifiedName();
  } else if (IsLambdaFrame()) {
    const auto lambda = GetTarget()->AsLambda();
    ASSERT(lambda);
    return lambda->HasSymbol() ? lambda->GetSymbol()->GetFullyQualifiedName() : "Lambda";
  }
  return "Unknown";
}

auto StackFrame::ToString() const -> std::string {
  ToStringHelper<StackFrame> helper;
  helper.AddField("id", GetId());
  helper.AddField("target", GetTarget());
  if (HasReturnAddress()) {
    helper.AddField("return_address", GetReturnAddressPointer());
  } else {
    helper.AddField("return_address", "0x0");
  }
  helper.AddField("locals", GetLocals());
  return helper;
}

StackFrameGuardBase::StackFrameGuardBase(TargetInfoCallback target_info) :
  target_info_(target_info) {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  if (!runtime->GetCallStack().IsEmpty())
    enter_ = runtime->GetCallStack().GetTop();
}

StackFrameGuardBase::~StackFrameGuardBase() {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  if (!runtime->GetCallStack().IsEmpty())
    exit_ = runtime->GetCallStack().GetTop();
  if ((!enter_ && !exit_) || (enter_ == exit_) || std::uncaught_exceptions() > 0)
    return;
  LOG(ERROR) << "Error: Invalid frame state after executing target";
  LOG(ERROR) << "";
  if (enter_) {
    LOG(ERROR) << "Enter Frame: ";
    StackFrameLogger::LogStackFrame<google::INFO, 1, false>(__FILE__, __LINE__, (*enter_));
  } else {
    LOG(ERROR) << "Enter Frame:";
    LOG(ERROR) << "  0x0";
  }
  if (exit_) {
    LOG(ERROR) << "Exit Frame: ";
    StackFrameLogger::LogStackFrame<google::INFO, 1, false>(__FILE__, __LINE__, (*exit_));
  } else {
    LOG(ERROR) << "Exit Frame:";
    LOG(ERROR) << "  0x0";
  }
  target_info_();
  LOG(FATAL);
}

template <typename T>
static inline constexpr auto AddressOf(const T* rhs) -> const void* {
  return reinterpret_cast<const void*>(rhs);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

#define __ (google::LogMessage(file(), line(), GetSeverity())).stream() << GetIndentString()
void StackFrameLogger::Visit(const StackFrame& frame) {
  __ << "Stack Frame #" << frame.GetId();
  __ << "Return Address: " << frame.GetReturnAddressPointer() << " ; "
     << (frame.HasReturnAddress() ? frame.GetReturnObjectPointer()->ToString() : "null");
  __ << "Target: " << AddressOf(frame.GetTarget()) << " ;; " << frame.GetTarget()->ToString();
  LocalScopePrinter::Print<google::INFO, false>(frame.GetLocals(), file(), line(), indent());
}
#undef __
}  // namespace gel