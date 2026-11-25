#include "plumed/mpiwrap/mpiwrap.h"
#include "plumed/mpiwrap/mpi.h"
#include "plumed/tools/Communicator.h"
#include "checks.h"

namespace PLMD {
namespace long_long_regtest {
void communicator_checks(MPI_Comm comm,int rank,int size) {
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
int body(int rank,const void *native,std::size_t,void *) {
  MPI_Comm comm=mpiwrap::Comm_from_native(native);
  int size=0;
  require(MPI_Comm_size(comm,&size)==MPI_SUCCESS,__LINE__);
#if defined(__PLUMED_HAS_MPI)
  checks(comm,rank,size);
  communicator_checks(comm,rank,size);
#else
  fallback_checks(comm,rank,size,true,communicator_checks);
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
