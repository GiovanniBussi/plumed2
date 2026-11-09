// Compile with -x c++ and one of the rejection case macros.
#include "plumed/core/WithCmd.h"
#include <vector>
void rejected(PLMD::WithCmd& p) {
#if defined(NESTED_DYNAMIC)
  p.cmd("check",std::vector<std::vector<double>>{});
#elif defined(BOOL_CONTAINER)
  p.cmd("check",std::vector<bool>{});
#else
#error Select NESTED_DYNAMIC or BOOL_CONTAINER
#endif
}
