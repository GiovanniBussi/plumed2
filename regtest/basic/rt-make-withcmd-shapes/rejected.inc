// Compile with -x c++ and one of the rejection case macros.
#include "plumed/core/WithCmd.h"
#include <vector>
void rejected(PLMD::WithCmd& p) {
#if defined(NESTED_DYNAMIC)
  p.cmd("check",std::vector<std::vector<double>>{});
#elif defined(NESTED_BOOL)
  p.cmd("check",std::vector<std::vector<bool>>{});
#elif defined(VOID_DATA)
  struct BadData { std::size_t size() const; void data(); };
  p.cmd("check",BadData{});
#elif defined(BOOL_CONTAINER)
  p.cmd("check",std::vector<bool>{});
#else
#error Select NESTED_DYNAMIC, NESTED_BOOL, VOID_DATA or BOOL_CONTAINER
#endif
}
