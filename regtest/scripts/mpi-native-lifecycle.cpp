/* Host MPI lifecycle for external-engine regtests using the wrapper MPI ABI. */
#include <mpi.h>

namespace PLMD {

int regtestNativeMPIInit(int *argc,char ***argv) {
  return MPI_Init(argc,argv);
}

int regtestNativeMPIFinalize() {
  return MPI_Finalize();
}

}
