#include "mpiwrap/mpiwrap.h"
#include "mpiwrap/mpi.h"
#include "checks.h"

int main(int argc,char **argv) {
  MPI_Init(&argc,&argv);
  try {
    int rank=0,size=0;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);
    PLMD::long_long_regtest::checks(MPI_COMM_WORLD,rank,size);
    if(rank==0) {
      std::puts("native direct long-long MPI passed");
    }
  } catch(const std::exception &exception) {
    std::fprintf(stderr,"%s\n",exception.what());
    MPI_Abort(MPI_COMM_WORLD,1);
  }
  MPI_Finalize();
}
