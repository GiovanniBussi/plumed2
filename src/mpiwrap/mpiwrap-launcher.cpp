#if defined(__PLUMED_MPIWRAP) && defined(__PLUMED_HAS_THREAD_MPI)
// Keep the internal rank launcher in a separate kernel translation unit.
#include "thread-mpi-launcher.cpp.inc"
#endif
