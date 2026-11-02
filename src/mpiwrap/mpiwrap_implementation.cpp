/* Shared native-MPI provider implementation; opt in only for adapter builds. */
#if defined(PLUMED_MPIW_NATIVE_ADAPTER)
#include <mpi.h>
#include "provider-abi.h"
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#ifdef __cplusplus
namespace PLMD {
extern "C" {
#endif
#include "mpiwrap_implementation.cpp.inc"
const plumed_mpiw_provider_descriptor *plumed_mpiw_provider_factory(void) {
  return plumed_mpiw_get_descriptor();
}
#ifdef __cplusplus
}
}
#endif
#endif
