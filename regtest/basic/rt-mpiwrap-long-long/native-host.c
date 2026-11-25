/* Native MPI stays in this C translation unit. The C++ probe sees only the
   provider frontend, including its distinct datatype and request handles. */
#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#define __PLUMED_WRAPPER_MPI_HOST_ADAPTER 1
#define __PLUMED_WRAPPER_IMPLEMENTATION 0
#include "wrapper/Plumed.h"

const void *long_long_test_provider(int missing) {
  const plumed_mpi_provider *native=plumed_mpi_implementation();
  static plumed_mpi_provider provider;
  static plumed_mpi_metadata_entry metadata[256];
  size_t i,n=0;
  provider=*native;
  for(i=0;i<native->metadata_count;++i) {
    const char *name=native->metadata[i].name;
    if(missing && (!strcmp(name,"value:MPI_LONG_LONG") ||
                   !strcmp(name,"value:MPI_LONG_LONG_INT") ||
                   !strcmp(name,"value:MPI_UNSIGNED_LONG_LONG"))) continue;
    if(n==256) abort();
    metadata[n++]=native->metadata[i];
  }
  provider.metadata=metadata;
  provider.metadata_count=n;
  /* Deliberately model a homogeneous threaded provider to exercise the byte
     fallback across Open MPI's larger supported function surface. */
  if(missing) provider.ranks_are_threads=1;
  return &provider;
}
void long_long_test_init(void) { MPI_Init(NULL,NULL); }
void long_long_test_finish(void) { MPI_Finalize(); }
void long_long_test_abort(void) { MPI_Abort(MPI_COMM_WORLD,1); }
const void *long_long_test_world(void) {
  static MPI_Comm world;
  world=MPI_COMM_WORLD;
  return &world;
}
