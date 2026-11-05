// PLUMED_HEADER_ONLY
/* Minimal MPI runtime inquiries shared by PLUMED modules. */
#ifndef __PLUMED_mpiwrap_mpi_h
#define __PLUMED_mpiwrap_mpi_h

namespace PLMD {
namespace mpiwrap {

#if defined(__PLUMED_HAS_MPI)
inline bool provider_installed() {
  return true;
}
inline bool provider_ranks_are_threads() {
  return false;
}
inline bool is_thread_mpi_active() {
  return false;
}
#elif !defined(__PLUMED_MPIWRAP)
inline bool provider_installed() {
  return false;
}
inline bool provider_ranks_are_threads() {
  return false;
}
inline bool is_thread_mpi_active() {
  return false;
}
#else
bool provider_installed();
// Describes the selected provider without requiring MPI initialization.
bool provider_ranks_are_threads();
bool is_thread_mpi_active();
#endif

} // namespace mpiwrap
}  // namespace PLMD

#endif
