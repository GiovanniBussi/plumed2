#include "checks.h"
#include <mpi.h>

int main(int argc, char** argv) {
  MPI_Init(&argc,&argv);
  MPI_Comm world=MPI_COMM_WORLD;
  int rank=0,size=0;
  MPI_Comm_rank(world,&rank);
  MPI_Comm_size(world,&size);
  if(size!=2) {
    MPI_Abort(world,1);
  }
  const Box box{{{{3.1,0.4,-0.2}},{{0.7,2.7,0.5}},{{0.8,-0.3,3.4}}}};
  const Positions x{{{{0.2,0.4,0.3}},{{2.5,0.2,1.1}},{{0.7,1.6,0.2}},{{1.4,0.8,1.7}}}};
  std::ofstream output;
  if(rank==0) {
    output.open("output");
    writeReference(x,"fit.pdb");
  }
  MPI_Barrier(world);
  try {
    for(const std::string axes : {
          "ab","ac","bc","a","b","c"
        }) {
      const std::vector<std::string> lines{
        "before: DISTANCE ATOMS=1,3 NOPBC",
        "v: COM ATOMS=1,2 NOPBC",
        "PROJECT_COORDINATES "+std::string(axes.size()==2 ? "PLANE=" : "AXIS=")+axes,
        "d: DISTANCE ATOMS=v,4",
        "c: COORDINATION GROUPA=1,2 GROUPB=3,4 R_0=1",
        "RESTRAINT ARG=d,c,before AT=0,0,0 KAPPA=2,2,2"};
      const Result expected=evaluate(x,box,lines);
      std::unique_ptr<FILE,decltype(&std::fclose)> log(std::tmpfile(),std::fclose);
      if(!log) {
        throw std::runtime_error("cannot open log");
      }
      PLMD::Plumed p;
      int natoms=4,local=2;
      double timestep=0.001;
      p.cmd("setMPIComm",&world);
      p.cmd("setNatoms",&natoms);
      p.cmd("setTimestep",&timestep);
      p.cmd("setLog",log.get());
      p.cmd("init");
      for(const auto& line : lines) {
        p.cmd("readInputLine",line.c_str());
      }
      for(int step=0; step<3; ++step) {
        // Change ownership on every step; positions are scattered across ranks.
        std::array<int,2> indices{{(rank+step)%2,(rank+step)%2+2}};
        std::array<Vec,2> positions{{x[indices[0]],x[indices[1]]}},forces{};
        std::array<double,2> masses{{double(indices[0]+1),double(indices[1]+1)}};
        Box cell=box,virial{};
        double bias=0;
        p.cmd("setAtomsNlocal",&local);
        p.cmd("setAtomsGatindex",indices.data());
        p.cmd("setStep",&step);
        p.cmd("setPositions",&positions[0][0]);
        p.cmd("setMasses",masses.data());
        p.cmd("setBox",&cell[0][0]);
        p.cmd("setForces",&forces[0][0]);
        p.cmd("setVirial",&virial[0][0]);
        p.cmd("calc");
        p.cmd("getBias",&bias);
        close(bias,expected.energy,"MPI bias",1e-10);
        for(unsigned a=0; a<2; ++a)
          for(unsigned j=0; j<3; ++j) {
            close(forces[a][j],expected.forces[indices[a]][j],"MPI forces",1e-10);
          }
        for(unsigned i=0; i<3; ++i)
          for(unsigned j=0; j<3; ++j) {
            close(virial[i][j],expected.virial[i][j],"MPI virial",1e-10);
          }
      }
      if(rank==0) {
        output<<axes<<": distributed forces and virial match serial with changing ownership\n";
      }
    }
  } catch(const std::exception& error) {
    std::fprintf(stderr,"rank %d: %s\n",rank,error.what());
    MPI_Abort(world,1);
  }
  MPI_Finalize();
  return 0;
}
