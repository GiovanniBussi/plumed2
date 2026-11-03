/*
  Test-only MPI surface for compiling external engines against a serial
  PLUMED kernel with an injected real-MPI provider. Only Init and Finalize
  belong to the host MPI runtime; all other names use PLUMED's wrapper ABI.
*/
#ifndef PLUMED_REGTEST_MPI_PROVIDER_SHIM_H
#define PLUMED_REGTEST_MPI_PROVIDER_SHIM_H

#include "plumed/mpiwrap/mpiwrap.h"

using PLMD::MPI_Comm;
using PLMD::MPI_Datatype;
using PLMD::MPI_Errhandler;
using PLMD::MPI_Group;
using PLMD::MPI_Op;
using PLMD::MPI_Request;
using PLMD::MPI_Status;
using PLMD::MPI_User_function;
using PLMD::MPI_Comm_errhandler_function;

namespace PLMD {
int regtestNativeMPIInit(int *argc,char ***argv);
int regtestNativeMPIFinalize();
}

#ifdef MPI_Init
#undef MPI_Init
#endif
#ifdef MPI_Finalize
#undef MPI_Finalize
#endif
#define MPI_Init(...) ::PLMD::regtestNativeMPIInit(__VA_ARGS__)
#define MPI_Finalize(...) ::PLMD::regtestNativeMPIFinalize(__VA_ARGS__)

#endif
