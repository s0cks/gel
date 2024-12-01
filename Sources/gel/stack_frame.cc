#include "gel/stack_frame.h"

#include "gel/local_scope.h"
#include "gel/runtime.h"
#include "gel/to_string_helper.h"

namespace gel {
auto StackFrame::ToString() const -> std::string {
  ToStringHelper<StackFrame> helper;
  helper.AddField("id", GetId());
  if (IsTargetEntryInstr()) {
    helper.AddField("target", std::get<instr::TargetEntryInstr*>(GetTarget()));
  } else if (IsNativeFrame()) {
    helper.AddField("target", std::get<NativeProcedure*>(GetTarget()));
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
  if (runtime->HasFrame())
    enter_ = std::optional<StackFrame>(*(runtime->GetCurrentFrame()));
}

StackFrameGuardBase::~StackFrameGuardBase() {
  const auto runtime = GetRuntime();
  ASSERT(runtime);
  if (runtime->HasFrame())
    exit_ = std::optional<StackFrame>(*(runtime->GetCurrentFrame()));
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

#define __ (google::LogMessage(file(), line(), GetSeverity())).stream() << GetIndentString()
void StackFrameLogger::Visit(const StackFrame& frame) {
  __ << "Stack Frame #" << frame.GetId();
  if (frame.HasReturnAddress()) {
    __ << "Return Address: " << frame.GetReturnAddressPointer() << " ; " << frame.GetReturnInstr()->ToString();
  } else {
    __ << "Return Address: " << frame.GetReturnAddressPointer() << " ; null";
  }
  if (frame.IsTargetEntryInstr()) {
    __ << "Target: " << ((void*)frame.GetTargetAsTargetEntryInstr()) << " ; " << frame.GetTargetAsTargetEntryInstr()->ToString();
  } else if (frame.IsNativeFrame()) {
    __ << "Native: " << ((void*)frame.GetTargetAsNativeProcedure()) << " ; " << frame.GetTargetAsNativeProcedure()->ToString();
  }
  LocalScopePrinter::Print<google::INFO, false>(frame.GetLocals(), file(), line(), indent());
}
#undef __
}  // namespace gel