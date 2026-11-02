/* Shared native-MPI provider implementation; opt in only for adapter builds. */
#if defined(PLUMED_MPIW_NATIVE_ADAPTER)
#include <mpi.h>
#include "provider-abi.h"
#include "provider-callbacks.h"
namespace PLMD {
extern "C" {
#include "mpiwrap_implementation.cpp.inc"
  const plumed_mpiw_provider_descriptor *plumed_mpiw_provider_factory(void) {
    return plumed_mpiw_get_descriptor();
  }
}
}
#endif
