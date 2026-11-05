// PLUMED_HEADER_ONLY
/* Default storage capacities; override consistently with compiler -D flags. */
#ifndef __PLUMED_mpiwrap_mpiwrap_capacity_h
#define __PLUMED_mpiwrap_mpiwrap_capacity_h

#ifndef PLUMED_MPIW_HANDLE_CAPACITY
#define PLUMED_MPIW_HANDLE_CAPACITY ((sizeof(void *) > 8U) ? sizeof(void *) : 8U)
#endif

#ifndef PLUMED_MPIW_STATUS_CAPACITY
#define PLUMED_MPIW_STATUS_CAPACITY 128
#endif

#endif
