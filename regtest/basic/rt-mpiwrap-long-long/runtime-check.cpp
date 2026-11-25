#include "mpiwrap/mpiwrap.h"
#include "mpiwrap/mpi.h"
#include "checks.h"
#include <cstdlib>

#if defined(_WIN32)
static_assert(sizeof(long)==4 && sizeof(long long)==8,"Windows test must exercise LLP64");
#endif

namespace PLMD {
namespace long_long_regtest {
int runtime_body(int rank,const void *native,std::size_t,void *) {
  MPI_Comm comm=mpiwrap::Comm_from_native(native);
  int size=0;
  require(MPI_Comm_size(comm,&size)==MPI_SUCCESS,__LINE__);
  fallback_checks(comm,rank,size,true);
  return 0;
}
}
}

int main(int argc,char **argv) {
  if(argc!=2) {
    return 2;
  }
  const int ranks=std::atoi(argv[1]);
  if(ranks<1 || ranks>4) {
    return 2;
  }
  int result=-1;
  const int rc=PLMD::mpiwrap::launchRanks(ranks,PLMD::long_long_regtest::runtime_body,
                                          nullptr,&result,PLMD::mpiwrap::MPILifecyclePolicy::internal_thread_mpi);
  return rc==0 ? result : 1;
}
