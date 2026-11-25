#include "plumed/mpiwrap/mpiwrap.h"
#include "plumed/mpiwrap/mpi.h"
#include "plumed/tools/Communicator.h"
#include "checks.h"

namespace PLMD {
namespace long_long_regtest {
int body(int rank,const void *native,std::size_t,void *) {
  MPI_Comm comm=mpiwrap::Comm_from_native(native);
  int size=0;
  require(MPI_Comm_size(comm,&size)==MPI_SUCCESS,__LINE__);
#if !defined(__PLUMED_HAS_MPI)
  // Mutate only the private regression process, between rank barriers. Force
  // the same missing values as an ordinary GROMACS tMPI build, even though
  // PLUMED's embedded snapshot normally enables them.
  static mpiwrap::plumed_mpi_provider_constant_pointer saved[3];
  if(rank==0) {
    saved[0]=mpiwrap::pointers::constant_LONG_LONG;
    saved[1]=mpiwrap::pointers::constant_LONG_LONG_INT;
    saved[2]=mpiwrap::pointers::constant_UNSIGNED_LONG_LONG;
    mpiwrap::pointers::constant_LONG_LONG=nullptr;
    mpiwrap::pointers::constant_LONG_LONG_INT=nullptr;
    mpiwrap::pointers::constant_UNSIGNED_LONG_LONG=nullptr;
  }
  require(MPI_Barrier(comm)==MPI_SUCCESS,__LINE__);
  require(MPI_LONG_LONG_INT.fallback_kind==1 && MPI_UNSIGNED_LONG_LONG.fallback_kind==2,__LINE__);
#endif
  checks(comm,rank,size);
  {
    Communicator typed;
    typed.Set_comm(comm);
    unsigned long long counter=(1ULL<<60)+rank;
    typed.Sum(counter);
    require(counter==(1ULL<<60)*size+size*(size-1)/2,__LINE__);
    counter=(1ULL<<63)+rank;
    std::vector<unsigned long long> gathered(size);
    typed.Allgather(counter,gathered);
    for(int r=0; r<size; ++r) {
      require(gathered[r]==(1ULL<<63)+r,__LINE__);
    }
    long long step=-(1LL<<45)+rank;
    typed.Bcast(step,size-1);
    require(step==-(1LL<<45)+size-1,__LINE__);
  }
#if !defined(__PLUMED_HAS_MPI)
  require(MPI_Barrier(comm)==MPI_SUCCESS,__LINE__);
  if(rank==0) {
    mpiwrap::pointers::constant_LONG_LONG=saved[0];
    mpiwrap::pointers::constant_LONG_LONG_INT=saved[1];
    mpiwrap::pointers::constant_UNSIGNED_LONG_LONG=saved[2];
  }
  require(MPI_Barrier(comm)==MPI_SUCCESS,__LINE__);
#endif
  if(rank==0) {
    FILE *output=std::fopen("output","w");
    if(!output) {
      return 1;
    }
    std::fputs("signed and unsigned long-long MPI checks passed\n",output);
    if(std::fclose(output)!=0) {
      return 1;
    }
  }
  return 0;
}
}
}

int main(int argc,char **argv) {
  if(argc!=3 || std::strcmp(argv[1],"-n") || std::strcmp(argv[2],"4")) {
    return 2;
  }
  using namespace PLMD;
  int result=-1;
#if defined(__PLUMED_HAS_THREAD_MPI) && !defined(__PLUMED_HAS_MPI)
  for(int ranks: {
        4,1,2,4
      }) {
    if(mpiwrap::launchRanks(ranks,long_long_regtest::body,nullptr,&result,
                            mpiwrap::MPILifecyclePolicy::internal_thread_mpi)!=0 || result) {
      return 1;
    }
  }
#else
  if(mpiwrap::launchRanks(4,long_long_regtest::body,nullptr,&result,
                          mpiwrap::MPILifecyclePolicy::standalone_owned)!=0 || result) {
    return 1;
  }
#endif
  return 0;
}
