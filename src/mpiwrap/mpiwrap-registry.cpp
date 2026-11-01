/* The runtime registry is linked into the kernel even when MPI wrapping is
   disabled, so the versioned kernel table can expose its opt-in installer. */
#include "runtime/provider-registry.cpp"
