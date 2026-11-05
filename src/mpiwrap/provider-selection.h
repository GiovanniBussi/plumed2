// PLUMED_HEADER_ONLY
/* Kernel-side environment selection; this is not part of the host MPI ABI. */
#ifndef __PLUMED_mpiwrap_provider_selection_h
#define __PLUMED_mpiwrap_provider_selection_h

namespace PLMD {
namespace mpiwrap {
// Return true when an explicit troubleshooting override has installed its
// provider. Throws on invalid configuration or an attempted late replacement.
bool forceEnvironmentProvider();
}
}

#endif
