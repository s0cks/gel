#include <cstdlib>
#include <glog/logging.h>
#include <uv.h>

#include "common.h"
#include "event_loop.h"
#include "native_procedure.h"
#include "plugin.h"
#include "procedure.h"
#include "runtime.h"

using namespace gel;

// TODO: switch based on sandbox flags
#define _DECLARE_FS_PROCEDURE(Name, Sym) _DECLARE_NATIVE_PROCEDURE(fs_##Name, "fs/" Sym);
#define DECLARE_FS_PROCEDURE(Name)       _DECLARE_FS_PROCEDURE(Name, #Name)

_DECLARE_FS_PROCEDURE(get_cwd, "get-cwd");
DECLARE_FS_PROCEDURE(stat);
DECLARE_FS_PROCEDURE(rename);
DECLARE_FS_PROCEDURE(mkdir);
DECLARE_FS_PROCEDURE(rmdir);
DECLARE_FS_PROCEDURE(open);
DECLARE_FS_PROCEDURE(close);
DECLARE_FS_PROCEDURE(unlink);
DECLARE_FS_PROCEDURE(fsync);
DECLARE_FS_PROCEDURE(ftruncate);
DECLARE_FS_PROCEDURE(access);
DECLARE_FS_PROCEDURE(chmod);
DECLARE_FS_PROCEDURE(link);
DECLARE_FS_PROCEDURE(symlink);
DECLARE_FS_PROCEDURE(readlink);
DECLARE_FS_PROCEDURE(chown);
_DECLARE_FS_PROCEDURE(copy_file, "copy-file");

DECLARE_FS_PROCEDURE(readdir);

#undef DECLARE_FS_PROCEDURE
#undef _DECLARE_FS_PROCEDURE

#define NATIVE_FS_PROCEDURE_F(Name) NATIVE_PROCEDURE_F(fs_##Name)

NATIVE_FS_PROCEDURE_F(get_cwd) {
  ASSERT(args.empty());
  return ReturnNew<String>(std::filesystem::current_path());
}

NATIVE_FS_PROCEDURE_F(stat) {
  NativeArgument<0, String> path(args);
  CHECK_NATIVE_ARG(path);
  NativeArgument<1, Procedure> on_next(args);
  CHECK_NATIVE_ARG(on_next);
  OptionalNativeArgument<2, Procedure> on_error(args);
  CHECK_NATIVE_ARG(on_error);
  OptionalNativeArgument<3, Procedure> on_finished(args);
  CHECK_NATIVE_ARG(on_finished);
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  return ReturnBool(loop->Stat(path->Get(), on_next, on_error, on_finished));
}

NATIVE_FS_PROCEDURE_F(rename) {
  NativeArgument<0, String> old_path(args);
  CHECK_NATIVE_ARG(old_path);
  NativeArgument<1, String> new_path(args);
  CHECK_NATIVE_ARG(new_path);
  OptionalNativeArgument<2, Procedure> on_success(args);
  CHECK_NATIVE_ARG(on_success);
  OptionalNativeArgument<3, Procedure> on_error(args);
  CHECK_NATIVE_ARG(on_error);
  OptionalNativeArgument<4, Procedure> on_finished(args);
  CHECK_NATIVE_ARG(on_finished);
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  return ReturnBool(loop->Rename(old_path->Get(), new_path->Get(), on_success, on_error, on_finished));
}

NATIVE_FS_PROCEDURE_F(mkdir) {
  NativeArgument<0, String> path(args);
  CHECK_NATIVE_ARG(path);
  NativeArgument<1, Long> mode(args);
  CHECK_NATIVE_ARG(mode);
  OptionalNativeArgument<2, Procedure> on_success(args);
  CHECK_NATIVE_ARG(on_success);
  OptionalNativeArgument<3, Procedure> on_error(args);
  CHECK_NATIVE_ARG(on_error);
  OptionalNativeArgument<4, Procedure> on_finished(args);
  CHECK_NATIVE_ARG(on_finished);
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  return ReturnBool(loop->Mkdir(path->Get(), static_cast<int>(mode->Get()), on_success, on_error, on_finished));
}

NATIVE_FS_PROCEDURE_F(rmdir) {
  NativeArgument<0, String> path(args);
  CHECK_NATIVE_ARG(path);
  OptionalNativeArgument<1, Procedure> on_success(args);
  CHECK_NATIVE_ARG(on_success);
  OptionalNativeArgument<2, Procedure> on_error(args);
  CHECK_NATIVE_ARG(on_error);
  OptionalNativeArgument<3, Procedure> on_finished(args);
  CHECK_NATIVE_ARG(on_finished);
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  return ReturnBool(loop->Rmdir(path->Get(), on_success, on_error, on_finished));
}

static inline auto WrapOpenFileOnNext(Procedure* on_next) -> FileOpenedCallback {
  return [on_next](Long* next) {
    if (on_next)
      GetRuntime()->Call(*on_next, {next});
  };
}

NATIVE_FS_PROCEDURE_F(open) {
  NativeArgument<0, String> path(args);
  CHECK_NATIVE_ARG(path);
  NativeArgument<1, Long> flags(args);
  CHECK_NATIVE_ARG(flags);
  NativeArgument<2, Long> mode(args);
  CHECK_NATIVE_ARG(mode);
  OptionalNativeArgument<3, Procedure> on_next(args);
  CHECK_NATIVE_ARG(on_next);
  OptionalNativeArgument<4, Procedure> on_error(args);
  CHECK_NATIVE_ARG(on_error);
  OptionalNativeArgument<5, Procedure> on_finished(args);
  CHECK_NATIVE_ARG(on_finished);
  return ReturnBool(OpenFileAsync(path->Get(), static_cast<int>(flags->Get()), static_cast<int>(mode->Get()),
                                  WrapOpenFileOnNext(on_next), WrapOnError(on_error), WrapOnFinished(on_finished)));
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

NATIVE_FS_PROCEDURE_F(readdir) {
  NativeArgument<0, String> path(args);
  CHECK_NATIVE_ARG(path);
  const auto loop = GetThreadEventLoop();
  ASSERT(loop);
  return ThrowNotImplementedError();
}

#undef NATIVE_FS_PROCEDURE_F

#define INIT_FS_NATIVE(Name) InitNative<fs_##Name>();

DEFINE_PLUGIN(fs) {
  INIT_FS_NATIVE(get_cwd);
  INIT_FS_NATIVE(stat);
  INIT_FS_NATIVE(rename);
  INIT_FS_NATIVE(mkdir);
  INIT_FS_NATIVE(rmdir);
  INIT_FS_NATIVE(open);
  INIT_FS_NATIVE(close);
  INIT_FS_NATIVE(unlink);
  INIT_FS_NATIVE(fsync);
  INIT_FS_NATIVE(ftruncate);
  INIT_FS_NATIVE(access);
  INIT_FS_NATIVE(chmod);
  INIT_FS_NATIVE(link);
  INIT_FS_NATIVE(symlink);
  INIT_FS_NATIVE(readlink);
  INIT_FS_NATIVE(chown);
  INIT_FS_NATIVE(copy_file);
  INIT_FS_NATIVE(readdir);
  return EXIT_SUCCESS;
}

#undef INIT_FS_NATIVE
