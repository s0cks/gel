#include <gel/plugin.h>
#include <glog/logging.h>

#include <cstdlib>

using namespace gel;

DEFINE_PLUGIN(Test) {  // NOLINT(modernize-use-trailing-return-type)
  LOG(INFO) << "initializing....";
  return EXIT_SUCCESS;
}