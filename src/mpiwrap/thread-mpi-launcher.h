// PLUMED_HEADER_ONLY
// Internal rank launcher for the embedded, namespaced thread-MPI runtime.
#ifndef __PLUMED_mpiwrap_thread_mpi_launcher_h
#define __PLUMED_mpiwrap_thread_mpi_launcher_h

#include <cstddef>

namespace PLMD {
namespace mpiwrap {

enum class MPILifecyclePolicy {
  // The library/CLTools caller owns MPI initialization and finalization.
  host_owned,
  // A standalone program owns real MPI lifecycle, with mpiexec creating processes.
  standalone_owned,
  // Use the embedded launcher to create ranks inside one process.
  internal_thread_mpi
};

int launchInternalThreadMPI(int ranks,
                            int (*callback)(int, const void *, std::size_t, void *),
                            void *callback_context, int *result);

// Common C++ callback entry used by CLTools and standalone test programs.
// Native communicator bytes are borrowed for the duration of the callback.
// Construct rank-local objects inside the callback, after initialization.
// A standalone-owned callback failure aborts the job to release blocked peers;
// a host-owned failure propagates to the host without finalizing its MPI.
int launchRanks(int ranks,
                int (*callback)(int, const void *, std::size_t, void *),
                void *callback_context, int *result,
                MPILifecyclePolicy lifecycle);

}  // namespace mpiwrap
}  // namespace PLMD

#endif
