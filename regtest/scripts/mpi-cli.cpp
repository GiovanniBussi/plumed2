/*
  MPI-aware PLUMED command launcher used by MPI provider regression tests.

  Compile this file with the MPI C++ compiler belonging to the provider under
  test. Keeping the wrapper implementation in this translation unit makes the
  native provider adapter use that same MPI ABI and runtime.
*/
#define __PLUMED_HAS_DLOPEN 1
#define __PLUMED_WRAPPER_LINK_RUNTIME 1
#define __PLUMED_WRAPPER_IMPLEMENTATION 1
#define PLUMED_MPIW_HOST_ADAPTER 1

#include <mpi.h>
#include "../../src/wrapper/Plumed.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <stdexcept>

int main(int argc,char **argv) {
  // Host-only option; remove it before forwarding the command to PLUMED.
  const bool inject=!(argc>1 && std::strcmp(argv[1],"--no-injection")==0);
  if(!inject) {
    for(int i=1; i<argc; ++i) {
      argv[i]=argv[i+1];
    }
    --argc;
  }
  if(MPI_Init(&argc,&argv)!=MPI_SUCCESS) {
    return 1;
  }

  // The focused checker supplies its requested rank count. Detect launcher
  // singletons before attributing their output filenames to the MPI adapter.
  const char *expectedRanks=std::getenv("PLUMED_MPI_HOST_EXPECTED_RANKS");
  if(expectedRanks) {
    int rank=-1;
    int size=0;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);
    std::fprintf(stdout,"MPI host native rank=%d size=%d expected=%s\n",rank,size,expectedRanks);
    std::fflush(stdout);
    if(size!=std::atoi(expectedRanks)) {
      std::fprintf(stderr,"MPI host: launcher supplied the wrong native communicator size\n");
      MPI_Abort(MPI_COMM_WORLD,1);
      return 1;
    }
  }

  int result=1;
  try {
    {
      // Test both creation paths without exposing another public handle type.
      // The host, rather than the PLUMED kernel, owns MPI initialization.
      const char *path=std::getenv("PLUMED_MPI_HOST_EXPLICIT_PATH");
      // Environment-provider mode exercises the same serial kernel without
      // injection; the external host still owns MPI initialization/finalization.
      plumed raw=path && path[0] ? plumed_create_dlopen(path) : plumed_create();
      if(!plumed_valid(raw)) {
        plumed_finalize(raw);
        throw std::runtime_error("could not create PLUMED with the host MPI implementation");
      }
      PLMD::Plumed command(raw);
      plumed_finalize(raw);
      if(inject) {
        command.cmd("injectMPIImplementation",PLMD::plumed_mpi_implementation());
      }
      MPI_Comm world=MPI_COMM_WORLD;
      command.cmd("CLTool setMPIComm",&world);
      command.cmd("CLTool setArgc",&argc);
      command.cmd("CLTool setArgv",argv);
      command.cmd("CLTool run",&result);
    }
  } catch(const std::exception &error) {
    std::fprintf(stderr,"MPI CLI launcher: %s\n",error.what());
    MPI_Abort(MPI_COMM_WORLD,1);
    return 1;
  } catch(...) {
    std::fprintf(stderr,"MPI CLI launcher: unknown exception\n");
    MPI_Abort(MPI_COMM_WORLD,1);
    return 1;
  }

  if(MPI_Finalize()!=MPI_SUCCESS) {
    return 1;
  }
  return result;
}
