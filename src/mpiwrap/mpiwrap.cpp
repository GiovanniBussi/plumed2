#if defined(__PLUMED_MPIWRAP) && defined(__PLUMED_HAS_THREAD_MPI)
/* This unity translation unit is built only when the internal provider is
   enabled. It combines registry, frontend, and internal adapter fragments. */
#include "provider-registry.cpp.inc"
#include "provider-pointer-definitions.inc"
#include "provider-frontend.cpp.inc"
#include "thread-mpi-adapter.cpp.inc"
#endif
