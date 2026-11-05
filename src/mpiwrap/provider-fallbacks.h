// PLUMED_HEADER_ONLY
// Shared kernel-side fallbacks for provider MPI operations.
#ifndef __PLUMED_mpiwrap_provider_fallbacks_h
#define __PLUMED_mpiwrap_provider_fallbacks_h

#include <cstddef>

namespace PLMD {
namespace mpiwrap {
struct MPI_Request;
struct MPI_Status;
struct MPI_Datatype;
struct MPI_Comm;

bool can_fallback_Wait();
bool can_fallback_Waitsome();
bool can_fallback_Testsome();
bool can_fallback_Allgather();
bool can_fallback_Allgatherv();
int fallback_Wait(MPI_Request *, MPI_Status *);
int fallback_Waitsome(int, MPI_Request [], int *, int [], MPI_Status []);
int fallback_Testsome(int, MPI_Request [], int *, int [], MPI_Status []);
int fallback_Allgather(const void *, int, MPI_Datatype,
                       void *, int, MPI_Datatype, MPI_Comm);
int fallback_Allgatherv(const void *, int, MPI_Datatype,
                        void *, const int [], const int [],
                        MPI_Datatype, MPI_Comm);
bool is_empty_status(const MPI_Status *);
void set_empty_status(MPI_Status *);
bool requests_all_null(int, const MPI_Request []);
bool try_singleton_reduce(const void *, void *, int, MPI_Datatype,
                          int, MPI_Comm, int *);
} // namespace mpiwrap
}

#endif
