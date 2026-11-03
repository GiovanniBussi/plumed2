/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2012-2023 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed.org for more information.

   This file is part of plumed, version 2.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#include "../wrapper/Plumed.h"

#include <cstdio>
#include <cstring>
#include <exception>

#ifdef __PLUMED_HAS_MPI
#include <mpi.h>
#endif

int main(int argc,char **argv) {
#ifdef __PLUMED_HAS_MPI
  bool nompi=false;
  for(int iarg=1; iarg<argc; ++iarg) {
    if(!std::strcmp(argv[iarg],"--no-mpi")) {
      nompi=true;
    }
    if(!std::strcmp(argv[iarg],"--mpi")) {
      nompi=false;
    }
    if(argv[iarg] && argv[iarg][0]!='-') {
      break;
    }
  }
  if(!nompi) {
    MPI_Init(&argc,&argv);
  }
#endif

  int result=0;
  try {
    PLMD::Plumed command;
    command.cmd("CLTool setArgc",&argc);
    command.cmd("CLTool setArgv",argv, {static_cast<std::size_t>(argc)});
#ifdef __PLUMED_HAS_MPI
    if(!nompi) {
      MPI_Comm comm;
      MPI_Comm_dup(MPI_COMM_WORLD,&comm);
      command.cmd("CLTool setMPIComm",&comm);
    }
#endif
    command.cmd("CLTool run",&result);
  } catch(const std::exception &error) {
    std::fprintf(stderr,"ERROR: %s\n",error.what());
    result=1;
  } catch(...) {
    std::fprintf(stderr,"ERROR: unknown exception while running PLUMED\n");
    result=1;
  }

#ifdef __PLUMED_HAS_MPI
  if(!nompi) {
    MPI_Finalize();
  }
#endif
  return result;
}
