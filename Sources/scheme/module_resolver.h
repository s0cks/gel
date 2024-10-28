#ifndef SCM_MODULE_RESOLVER_H
#define SCM_MODULE_RESOLVER_H

#include "scheme/common.h"

namespace scm {
class Module;
class Symbol;
class ModuleResolver {
  DEFINE_NON_COPYABLE_TYPE(ModuleResolver);

 protected:
  ModuleResolver() = default;

 public:
  virtual ~ModuleResolver() = default;
  virtual auto ResolveModule(Symbol* symbol) -> Module* = 0;
};
}  // namespace scm

#endif  // SCM_MODULE_RESOLVER_H
