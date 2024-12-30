#include "gel/natives.h"
#ifndef GEL_SANDBOX

#include "gel/event_loop.h"

namespace gel::proc {
#define NATIVE_FS_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(fs_##Name)

NATIVE_FS_PROCEDURE_F(get_cwd) {
  ASSERT(args.empty());
  return ReturnNew<String>(std::filesystem::current_path());
}

NATIVE_FS_PROCEDURE_F(stat) {
  NativeArgument<0, String> path(args);
  if (!path)
    return Throw(path.GetError());
  NativeArgument<1, Procedure> on_next(args);
  if (!on_next)
    return Throw(on_next.GetError());
  OptionalNativeArgument<2, Procedure> on_error(args);
  if (!on_error)
    return Throw(on_error.GetError());
  OptionalNativeArgument<3, Procedure> on_finished(args);
  if (!on_finished)
    return Throw(on_finished.GetError());
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  return ReturnBool(loop->Stat(path->Get(), on_next, on_error, on_finished));
}

NATIVE_FS_PROCEDURE_F(rename) {
  NativeArgument<0, String> old_path(args);
  if (!old_path)
    return Throw(old_path.GetError());
  NativeArgument<1, String> new_path(args);
  if (!new_path)
    return Throw(new_path.GetError());
  OptionalNativeArgument<2, Procedure> on_success(args);
  if (!on_success)
    return Throw(on_success);
  OptionalNativeArgument<3, Procedure> on_error(args);
  if (!on_error)
    return Throw(on_error.GetError());
  OptionalNativeArgument<4, Procedure> on_finished(args);
  if (!on_finished)
    return Throw(on_finished.GetError());
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  return ReturnBool(loop->Rename(old_path->Get(), new_path->Get(), on_success, on_error, on_finished));
}

NATIVE_FS_PROCEDURE_F(mkdir) {
  NativeArgument<0, String> path(args);
  if (!path)
    return Throw(path);
  NativeArgument<1, Long> mode(args);
  if (!mode)
    return Throw(mode);
  OptionalNativeArgument<2, Procedure> on_success(args);
  if (!on_success)
    return Throw(on_success);
  OptionalNativeArgument<3, Procedure> on_error(args);
  if (!on_error)
    return Throw(on_error);
  OptionalNativeArgument<4, Procedure> on_finished(args);
  if (!on_finished)
    return Throw(on_finished);
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  return ReturnBool(loop->Mkdir(path->Get(), static_cast<int>(mode->Get()), on_success, on_error, on_finished));
}

NATIVE_FS_PROCEDURE_F(rmdir) {
  NativeArgument<0, String> path(args);
  if (!path)
    return Throw(path.GetError());
  OptionalNativeArgument<1, Procedure> on_success(args);
  if (!on_success)
    return Throw(on_success.GetError());
  OptionalNativeArgument<2, Procedure> on_error(args);
  if (!on_error)
    return Throw(on_error.GetError());
  OptionalNativeArgument<3, Procedure> on_finished(args);
  if (!on_finished)
    return Throw(on_finished.GetError());
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  return ReturnBool(loop->Rmdir(path->Get(), on_success, on_error, on_finished));
}

NATIVE_FS_PROCEDURE_F(open) {
  NativeArgument<0, String> path(args);
  if (!path)
    return Throw(path);
  NativeArgument<1, Long> flags(args);
  if (!flags)
    return Throw(flags);
  NativeArgument<2, Long> mode(args);
  if (!mode)
    return Throw(mode);
  OptionalNativeArgument<3, Procedure> on_success(args);
  if (!on_success)
    return Throw(on_success.GetError());
  OptionalNativeArgument<4, Procedure> on_error(args);
  if (!on_error)
    return Throw(on_error.GetError());
  OptionalNativeArgument<5, Procedure> on_finished(args);  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
  if (!on_finished)
    return Throw(on_finished.GetError());
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  return loop->Open(path->Get(), static_cast<int>(flags->Get()), static_cast<int>(mode->Get()), on_success, on_error,
                    on_finished);
}

NATIVE_FS_PROCEDURE_F(close) {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return ThrowNotImplementedError();
}

NATIVE_FS_PROCEDURE_F(unlink) {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return ThrowNotImplementedError();
}

NATIVE_FS_PROCEDURE_F(fsync) {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return ThrowError("not implemented");
}

NATIVE_FS_PROCEDURE_F(ftruncate) {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return ThrowNotImplementedError();
}

NATIVE_FS_PROCEDURE_F(access) {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return ThrowNotImplementedError();
}

NATIVE_FS_PROCEDURE_F(chmod) {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return ThrowNotImplementedError();
}

NATIVE_FS_PROCEDURE_F(link) {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return ThrowNotImplementedError();
}

NATIVE_FS_PROCEDURE_F(symlink) {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return ThrowNotImplementedError();
}

NATIVE_FS_PROCEDURE_F(readlink) {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return ThrowNotImplementedError();
}

NATIVE_FS_PROCEDURE_F(chown) {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return ThrowNotImplementedError();
}

NATIVE_FS_PROCEDURE_F(copy_file) {
  NOT_IMPLEMENTED(ERROR);  // TODO: implement
  return ThrowNotImplementedError();
}

#undef NATIVE_FS_PROCEDURE_F
}  // namespace gel::proc

#endif  // GEL_SANDBOX