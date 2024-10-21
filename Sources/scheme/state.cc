#include "scheme/state.h"

#include <glog/logging.h>

#include "scheme/environment.h"

namespace scm {
State::State(Environment* env) :
  globals_(env) {
  if (env == nullptr && globals_ == nullptr) {
    SetGlobals(Environment::New());
    LOG_IF(FATAL, GetGlobals() == nullptr) << "failed to initialize global environment.";
  }
}

State::~State() {
  delete globals_;
}
}  // namespace scm