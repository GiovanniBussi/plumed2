// Internal rank launcher for the embedded, namespaced thread-MPI runtime.
#ifndef PLUMED_MPIWRAP_RUNTIME_THREAD_MPI_LAUNCHER_HPP
#define PLUMED_MPIWRAP_RUNTIME_THREAD_MPI_LAUNCHER_HPP

#include <cstddef>

namespace PLMD {
namespace mpiwrap_provider {

int launchInternalThreadMPI(int ranks,
                            int (*callback)(int, const void *, std::size_t, void *),
                            void *callback_context, int *result);

}  // namespace mpiwrap_provider
}  // namespace PLMD

#endif
