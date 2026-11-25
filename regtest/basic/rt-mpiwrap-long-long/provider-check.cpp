#include "mpiwrap/internal.h"
#include "mpiwrap/mpi.h"
#include "checks.h"
#include <cstdlib>

extern "C" {
  const void *long_long_test_provider(int);
  void long_long_test_init();
  void long_long_test_finish();
  void long_long_test_abort();
  const void *long_long_test_world();
}

int main() {
  using namespace PLMD;
  using namespace mpiwrap;
  const bool missing=std::getenv("PLUMED_TEST_MISSING_LONG_LONG")!=nullptr;
  const auto *descriptor=static_cast<const plumed_mpi_provider *>(long_long_test_provider(missing));
  std::string error;
  if(!currentRegistry().install(descriptor,error)) {
    std::fprintf(stderr,"%s\n",error.c_str());
    return 1;
  }
  // Logical predefined types must be usable as values before initialization;
  // only their native transport datatype requires an initialized runtime.
  const auto type=LONG_LONG_INT();
  long_long_regtest::require(type.fallback_kind==(missing ? 1U : 0U),__LINE__);
  long_long_test_init();
  try {
    const MPI_Comm comm=Comm_from_native(long_long_test_world());
    int rank=0,size=0;
    Comm_rank(comm,&rank);
    Comm_size(comm,&size);
    long_long_regtest::checks(comm,rank,size);
    if(rank==0) {
      std::puts(missing ? "forced missing long-long provider passed" : "native long-long provider passed");
    }
  } catch(const std::exception &exception) {
    std::fprintf(stderr,"%s\n",exception.what());
    long_long_test_abort();
    return 1;
  }
  long_long_test_finish();
  return 0;
}
