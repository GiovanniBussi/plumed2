/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2012-2023 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed.org for more information.

   This file is part of plumed, version 2.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#include "wrapper/Plumed.h"

#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <vector>

#ifdef __PLUMED_HAS_MPI
#include <mpi.h>
#endif

namespace PLMD {
namespace {

struct CommandContext {
  Kernel *kernel;
  int argc;
  char **argv;
};

int runCommandOnRank(int, const void *native_world, std::size_t native_size,
                     void *opaque) {
  if(!native_world || native_size==0) {
    throw std::runtime_error("internal launcher did not supply the native world communicator");
  }
  CommandContext *context=static_cast<CommandContext *>(opaque);
  plumed raw=context->kernel->create();
  Plumed command(raw);
  plumed_finalize(raw);
  command.cmd("CLTool setMPIComm",const_cast<void *>(native_world));
  command.cmd("CLTool setArgc",context->argc);
  command.cmd("CLTool setArgv",context->argv,
  {static_cast<std::size_t>(context->argc)});
  int result=0;
  command.cmd("CLTool run",&result);
  return result;
}

bool parseThreadCount(const char *text,int &count) {
  if(!text || !text[0]) {
    return false;
  }
  errno=0;
  char *end=nullptr;
  const long value=std::strtol(text,&end,10);
  if(errno || end==text || *end!='\0' || value<1 || value>INT_MAX) {
    return false;
  }
  count=static_cast<int>(value);
  return true;
}

int runMain(int argc,char **argv) {
  int thread_count=0;
  bool launcher_seen=false;
  bool explicit_mpi=false;
  bool explicit_no_mpi=false;
  bool invalid_launcher_option=false;
  std::vector<char *> filtered_arguments;
  filtered_arguments.push_back(argv[0]);
  for(int i=1; i<argc; ++i) {
    if(std::strcmp(argv[i],"--tmpi-threads")==0) {
      if(launcher_seen || i+1>=argc ||
          !parseThreadCount(argv[i+1],thread_count)) {
        invalid_launcher_option=true;
      } else {
        launcher_seen=true;
      }
      if(i+1<argc) {
        ++i;
      }
    } else if(std::strncmp(argv[i],"--tmpi-threads=",15)==0) {
      if(launcher_seen || !parseThreadCount(argv[i]+15,thread_count)) {
        invalid_launcher_option=true;
      } else {
        launcher_seen=true;
      }
    } else {
      if(std::strcmp(argv[i],"--mpi")==0) {
        explicit_mpi=true;
      } else if(std::strcmp(argv[i],"--no-mpi")==0) {
        explicit_no_mpi=true;
      }
      filtered_arguments.push_back(argv[i]);
    }
  }
  if(invalid_launcher_option) {
    std::fprintf(stderr,"ERROR: --tmpi-threads requires one positive rank count and may be used once\n");
    return 1;
  }
  if(launcher_seen && explicit_mpi) {
    std::fprintf(stderr,"ERROR: --tmpi-threads and --mpi cannot be requested together\n");
    return 1;
  }
  if(launcher_seen && explicit_no_mpi) {
    std::fprintf(stderr,"ERROR: --tmpi-threads and --no-mpi cannot be requested together\n");
    return 1;
  }

  if(launcher_seen) {
#ifdef __PLUMED_HAS_MPI
    std::fprintf(stderr,"ERROR: --tmpi-threads is unavailable in a native-MPI PLUMED build\n");
    return 1;
#else
    try {
      Kernel kernel=Kernel::openSelected();
      CommandContext context{&kernel,
                             static_cast<int>(filtered_arguments.size()),
                             filtered_arguments.data()};
      return kernel.launchInternalThreads(thread_count,runCommandOnRank,&context);
    } catch(const std::exception &error) {
      std::fprintf(stderr,"ERROR: internal thread-MPI launch failed: %s\n",error.what());
      return 1;
    }
#endif
  }

#ifdef __PLUMED_HAS_MPI
  bool nompi=false;
  for(int iarg=1; iarg<argc; iarg++) {
    if(!std::strcmp(argv[iarg],"--no-mpi")) {
      nompi=true;
    }
    if(!std::strcmp(argv[iarg],"--mpi")) {
      nompi=false;
    }
// stop at first non-option
    if(argv[iarg] && argv[iarg][0]!='-') {
      break;
    }
  }
  if(!nompi) {
    MPI_Init(&argc,&argv);
  }
#endif
  int ret=0;

  try {
    Plumed p;
    p.cmd("CLTool setArgc",&argc);
    p.cmd("CLTool setArgv",argv, {static_cast<std::size_t>(argc)});
#ifdef __PLUMED_HAS_MPI
    if(!nompi) {
      MPI_Comm comm;
      MPI_Comm_dup(MPI_COMM_WORLD,&comm);
      p.cmd("CLTool setMPIComm",&comm);
    }
#endif
    p.cmd("CLTool run",&ret);
// end block deletes p also in case an exception occurs
  } catch(const std::exception &error) {
    std::fprintf(stderr,"ERROR: %s\n",error.what());
    ret=1;
  } catch(...) {
    std::fprintf(stderr,"ERROR: unknown exception while running PLUMED\n");
    ret=1;
  }

#ifdef __PLUMED_HAS_MPI
  if(!nompi) {
    MPI_Finalize();
  }
#endif
  return ret;
}

}  // namespace
}  // namespace PLMD

int main(int argc,char **argv) {
  return PLMD::runMain(argc,argv);
}
