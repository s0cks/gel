#include "gel/stack_frame.h"

#include "gel/local_scope.h"
#include "gel/object.h"
#include "gel/runtime.h"
#include "gel/script.h"
#include "gel/to_string_helper.h"

namespace gel {
auto StackFrame::GetTargetName() const -> std::string {
  if (IsScriptFrame()) {
    return "Script";  // TODO: implement
  } else if (IsNativeFrame()) {
    return GetNativeProcedure()->GetSymbol()->Get();
  } else if (IsLambdaFrame()) {
    const auto lambda = GetLambda();
    ASSERT(lambda);
    return lambda->HasName() ? lambda->GetName()->Get() : "Lambda";
  }
  return "Unknown";
}

auto StackFrame::ToString() const -> std::string {
  ToStringHelper<StackFrame> helper;
  helper.AddField("id", GetId());
  if (IsScriptFrame()) {
    helper.AddField("target", GetScript());
  } else if (IsNativeFrame()) {
    helper.AddField("target", GetNativeProcedure());
  } else if (IsLambdaFrame()) {
    helper.AddField("target", GetLambda());
  }
  if (HasReturnAddress()) {
    helper.AddField("return_address", GetReturnInstr());
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
  if (runtime->HasStackFrame())
    enter_ = std::optional<StackFrame>(runtime->GetCurrentStackFrame());
}

StackFrameGuardBase::~StackFrameGuardBase() {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  if (runtime->HasStackFrame())
    exit_ = std::optional<StackFrame>(runtime->GetCurrentStackFrame());
  if ((!enter_ && !exit_) || (enter_ == exit_))
    return;
  if (runtime->HasError()) {
    LOG(ERROR) << "Error: " << runtime->GetError();
  } else {
    LOG(ERROR) << "Error: Invalid frame state after executing target";
  }
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
  if (frame.HasReturnAddress()) {
    __ << "Return Address: " << frame.GetReturnAddressPointer() << " ; " << frame.GetReturnInstr()->ToString();
  } else {
    __ << "Return Address: " << frame.GetReturnAddressPointer() << " ; null";
  }
  if (frame.IsScriptFrame()) {
    __ << "Script: " << AddressOf(frame.GetScript()) << " ;; " << frame.GetScript()->ToString();
  } else if (frame.IsLambdaFrame()) {
    __ << "Lambda: " << AddressOf(frame.GetLambda()) << " ;; " << frame.GetLambda()->ToString();
  } else if (frame.IsNativeFrame()) {
    __ << "Native: " << AddressOf(frame.GetNativeProcedure()) << " ;; " << frame.GetNativeProcedure()->ToString();
  }
  LocalScopePrinter::Print<google::INFO, false>(frame.GetLocals(), file(), line(), indent());
}
#undef __
}  // namespace gel