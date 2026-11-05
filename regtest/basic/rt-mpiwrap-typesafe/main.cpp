// Include the local MPI type before the public wrapper can erase argument types.
#include "plumed/tools/Communicator.h"
#include "plumed/wrapper/Plumed.h"
#include <fstream>
#include <stdexcept>
#include <string>

namespace PLMD {
namespace test_mpi_typesafe {
void require(bool condition) {
  if(!condition) {
    throw std::runtime_error("MPI pointer type check failed");
  }
}
template<class Function>
void typeError(Function function,const char* text) {
  bool caught=false;
  try {
    function();
  } catch(const ExceptionTypeError& error) {
    require(std::string(error.what()).find(text)!=std::string::npos);
    caught=true;
  }
  require(caught);
}
template<class Function>
void wrapperTypeError(Function function,const char* text) {
  bool caught=false;
  try {
    function();
  } catch(const Plumed::ExceptionTypeError& error) {
    require(std::string(error.what()).find(text)!=std::string::npos);
    caught=true;
  }
  require(caught);
}
void run() {
  struct Mutable {
    int value;
  } mutableData{};
  require(TypesafePtr(&mutableData).get<Mutable*>()==&mutableData);
#if defined(__PLUMED_MPIWRAP) || defined(__PLUMED_HAS_MPI)
  MPI_Comm communicator {};
  const MPI_Comm constant{};
  double wrong=1.0;
  require(TypesafePtr(&communicator).get<const MPI_Comm*>()==&communicator);
  require(TypesafePtr(&constant).get<const MPI_Comm*>()==&constant);
  require(TypesafePtr(static_cast<const void*>(&communicator)).get<const MPI_Comm*>()==&communicator);
  typeError([&] { TypesafePtr(&wrong).get<const MPI_Comm*>(); },"pointer to MPI_Comm");
  typeError([&] { TypesafePtr(1).get<const MPI_Comm*>(); },"pointer to MPI_Comm");

  // An older caller supplies no negative annotation. In provider mode its
  // native integer can be smaller than our wrapper object: do not dereference.
  int native=7;
  TypesafePtr legacy(&native,0,nullptr,sizeof(int) | 0x30000u | 0x4000000u);
#if defined(__PLUMED_MPIWRAP) && !defined(__PLUMED_HAS_MPI)
  require(static_cast<const void*>(legacy.get<const MPI_Comm*>())==&native);
#endif
  require(TypesafePtr::unchecked(&communicator).get<const MPI_Comm*>()==&communicator);
  // Future annotations remain ignored.
  TypesafePtr future(&communicator,0,nullptr,0x40000000u | 0x70000u);
  require(future.get<const MPI_Comm*>()==&communicator);

  Plumed command;
  for(const char* key : {
        "setMPIComm","setMPImultiSimComm","CLTool setMPIComm",
        "GREX setMPIIntracomm","GREX setMPIIntercomm"
      }) {
    wrapperTypeError([&] { command.cmd(key,&wrong); },"pointer to MPI_Comm");
    wrapperTypeError([&] { command.cmd(key,1.5); },"pointer to MPI_Comm");
  }
#endif

  mpiwrap::plumed_mpiw_provider_descriptor descriptor{};
  const auto* provider=&descriptor;
  require(TypesafePtr(provider).get<const mpiwrap::plumed_mpiw_provider_descriptor*>()==provider);
  require(((TypesafePtr(provider).getFlags()>>16)&0xffu)==6);
  require(TypesafePtr(static_cast<const void*>(provider)).get<const mpiwrap::plumed_mpiw_provider_descriptor*>()==provider);
  typeError([&] { TypesafePtr(&mutableData).get<const mpiwrap::plumed_mpiw_provider_descriptor*>(); },"MPI provider descriptor pointer");
  TypesafePtr futureProvider(const_cast<void*>(static_cast<const void*>(provider)),0,nullptr,0x80070000u);
  require(futureProvider.get<const mpiwrap::plumed_mpiw_provider_descriptor*>()==provider);
  int wrongProvider=1;
  typeError([&] { TypesafePtr(&wrongProvider).get<const mpiwrap::plumed_mpiw_provider_descriptor*>(); },"MPI provider descriptor pointer");
#if defined(__PLUMED_MPIWRAP) && !defined(__PLUMED_HAS_MPI)
  wrapperTypeError([&] { command.cmd("injectMPIImplementation",&wrongProvider); },"MPI provider descriptor pointer");
  wrapperTypeError([&] { command.cmd("injectMPIImplementation",&mutableData); },"MPI provider descriptor pointer");
#endif
}
}
}
int main() {
  PLMD::test_mpi_typesafe::run();
  std::ofstream("passed")<<"MPI pointer type checks passed\n";
}
