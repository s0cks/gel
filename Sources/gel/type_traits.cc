#include "gel/type_traits.h"

#include "gel/object.h"

namespace gel {
template <class T>
auto GetDocs(T* value, std::enable_if_t<has_docs<T>::value>*) -> String* {
  if (!value)
    return String::Empty();
  return value->GetDocs();
}
}  // namespace gel