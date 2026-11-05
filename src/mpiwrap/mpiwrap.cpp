#include "mpiwrap.h"
#if defined(__PLUMED_MPIWRAP) && !defined(__PLUMED_HAS_MPI)
/* Registry and frontend support are present in every wrapper-mode kernel so
   runtime installation remains available even for an explicitly serial
   build. The embedded provider and launcher are optional. */
#include "provider-registry.cpp.inc"
#include "provider-pointer-definitions.inc"
#include "provider-frontend.cpp.inc"
#include "provider-fallbacks.cpp.inc"
#include "provider-loader.cpp.inc"
#if defined(__PLUMED_HAS_THREAD_MPI)
#include "thread-mpi-adapter.cpp.inc"
#endif
#endif
