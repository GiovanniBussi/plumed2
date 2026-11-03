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
#include <exception>
#include <stdexcept>

int main(int argc,char **argv) {
  if(MPI_Init(&argc,&argv)!=MPI_SUCCESS) {
    return 1;
  }

  int result=1;
  try {
    {
      // Test both creation paths without exposing another public handle type.
      // The host, rather than the PLUMED kernel, owns MPI initialization.
      const char *path=std::getenv("PLUMED_MPI_HOST_EXPLICIT_PATH");
      plumed raw=path && path[0]
                 ? plumed_create_dlopen_with_mpi(path,PLMD::plumed_mpi_implementation())
                 : plumed_create_with_mpi(PLMD::plumed_mpi_implementation());
      if(!plumed_valid(raw)) {
        plumed_finalize(raw);
        throw std::runtime_error("could not create PLUMED with the host MPI implementation");
      }
      PLMD::Plumed command(raw);
      plumed_finalize(raw);
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
