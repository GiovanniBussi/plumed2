#include "plumed/wrapper/Plumed.h"
#if defined(EXCHANGE_NATIVE_MPI)
#include <mpi.h>
#else
#include "mpi-test.h"
#endif
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace PLMD {
namespace regtest_mpi {

static void require(bool condition, MPI_Comm world, const char* message) {
  if(!condition) {
    std::fprintf(stderr,"exchange veto test: %s\n",message);
    MPI_Abort(world,1);
  }
}

int run(MPI_Comm world, int, char**) {
  int rank,size;
  MPI_Comm_rank(world,&rank);
  MPI_Comm_size(world,&size);
  require(size==4 || size==8,world,"expected four replicas with one or two ranks each");
  const int ranksPerReplica=size/4;
  const int replica=rank/ranksPerReplica;
  const int localRank=rank%ranksPerReplica;
  MPI_Comm intra,inter;
  MPI_Comm_split(world,replica,localRank,&intra);
  MPI_Comm_split(world,localRank==0 ? 0 : MPI_UNDEFINED,replica,&inter);
  {
    Plumed p;
    p.cmd("setMPIComm",&intra);
    p.cmd("GREX setMPIIntracomm",&intra);
    if(localRank==0) {
      p.cmd("GREX setMPIIntercomm",&inter);
    }
    p.cmd("GREX init");
    p.cmd("setNatoms",2);
    p.cmd("setTimestep",0.001);
    const std::string log="test."+std::to_string(rank)+".log";
    p.cmd("setLogFile",log.c_str());
    p.cmd("init");
    p.cmd("readInputLine","LOAD FILE=./fixed." __PLUMED_SOEXT);
    p.cmd("readInputLine","FIXED_EXCHANGES ORDER=0,1,2,3 REJECT=2,3");
    p.cmd("readInputLine","d: DISTANCE ATOMS=1,2 NOPBC");
    p.cmd("readInputLine","r: RESTRAINT ARG=d AT=@replicas:0.2,0.4,0.6,0.8 KAPPA=10");
    std::array<double,6> positions{},forces{};
    std::array<double,9> box{},virial{};
    std::array<double,2> masses{{1,1}};
    for(int phase : {
          1,0
        }) {
      const int step=phase==1 ? 5 : 10;
      positions[3]=0.3+0.1*replica;
      forces.fill(0);
      virial.fill(0);
      p.cmd("setStep",step);
      p.cmd("setPositions",positions.data());
      p.cmd("setMasses",masses.data());
      p.cmd("setForces",forces.data());
      p.cmd("setBox",box.data());
      p.cmd("setVirial",virial.data());
      p.cmd("calc");
      double bias=0;
      p.cmd("getBias",&bias);
      const double target=0.2+0.2*replica;
      const double displacement=positions[3]-target;
      require(std::abs(bias-5*displacement*displacement)<1e-12,world,"physical bias was changed");
      const auto originalForces=forces;
      p.cmd("GREX savePositions");
      int partner=replica;
      if(localRank==0) {
        p.cmd("setNumberOfReplicas",4);
        std::array<int,4> order{};
        p.cmd("getExchangesList",order.data(), {order.size()});
        for(int i=1; i<4; ++i) {
          if(i%2==phase) {
            if(order[i-1]==replica) {
              partner=order[i];
            } else if(order[i]==replica) {
              partner=order[i-1];
            }
          }
        }
        p.cmd("GREX setPartner",partner);
        p.cmd("GREX calculate");
      } else {
        p.cmd("GREX prepare");
      }
      MPI_Bcast(&partner,1,MPI_INT,0,intra);
      double local=0;
      p.cmd("GREX getLocalDeltaBias",&local);
      if(phase==1 && replica>=2) {
        require(std::isinf(local) && local>0,world,"dummy attempt was not vetoed on every rank");
      } else {
        const double other=0.3+0.1*partner-target;
        const double expected=5*(other*other-displacement*displacement);
        require(std::isfinite(local) && std::abs(local-expected)<1e-12,world,"real exchange correction changed");
      }
      require(forces==originalForces,world,"GREX veto modified the MD forces");
      p.cmd("getBias",&bias);
      require(std::isfinite(bias),world,"veto leaked into the physical bias");
      if(localRank==0) {
        p.cmd("GREX shareAllDeltaBias");
        for(int i=0; i<4; ++i) {
          double value=0;
          const std::string command="GREX getDeltaBias "+std::to_string(i);
          p.cmd(command,&value);
          require((phase==1 && i>=2) ? (std::isinf(value) && value>0) : std::isfinite(value),
                  world,"shared correction lost veto or contaminated a real pair");
        }
      }
    }
  }
  if(localRank==0) {
    MPI_Comm_free(&inter);
  }
  MPI_Comm_free(&intra);
  MPI_Barrier(world);
  if(rank==0) {
    std::ofstream("output") << "GREX real pairs and idle self-pairs: finite corrections\n"
                            << "GREX dummy pairs: positive infinity, physical bias and forces unchanged\n";
  }
  return 0;
}

}
}

int main(int argc, char** argv) {
#if defined(EXCHANGE_NATIVE_MPI)
  MPI_Init(&argc,&argv);
  int result=1;
  try {
    result=PLMD::regtest_mpi::run(MPI_COMM_WORLD,argc,argv);
  } catch(const std::exception& e) {
    std::fprintf(stderr,"%s\n",e.what());
    MPI_Abort(MPI_COMM_WORLD,1);
  }
  MPI_Finalize();
  return result;
#else
  return PLMD::mpi_test::run(argc,argv,8,PLMD::regtest_mpi::run);
#endif
}
