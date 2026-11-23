#include "checks.h"
int main() {
  try {
    return scopeChecks();
  } catch(const std::exception& e) {
    std::fprintf(stderr,"%s\n",e.what());
    return 1;
  }
}
