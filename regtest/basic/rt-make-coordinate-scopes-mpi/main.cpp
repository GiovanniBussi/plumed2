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
    const std::vector<std::string> modifiers{
      "FIT_TO_TEMPLATE REFERENCE=fit.pdb TYPE=SIMPLE NOPBC",
      "FIT_TO_TEMPLATE REFERENCE=fit.pdb TYPE=OPTIMAL NOPBC",
      "FIT_TO_TEMPLATE REFERENCE=fit.pdb TYPE=OPTIMAL-FAST NOPBC",
      "RESET_CELL", "WHOLEMOLECULES ENTITY0=1-4", "WRAPAROUND ATOMS=4 AROUND=1"};
    for(const auto& modifier:modifiers) {
      const std::string action=modifier.substr(0,modifier.find(' '));
      std::vector<std::string> lines{"old: COM ATOMS=1,2 NOPBC",modifier};
      append(lines,measured("i"));
      lines.push_back(action+" END");
      append(lines,measured("o"));
      const Result expected=evaluate(x,box,lines);
      std::unique_ptr<FILE,decltype(&std::fclose)> log(std::tmpfile(),std::fclose);
      if(!log) {
        throw std::runtime_error("cannot open log");
      }
      PLMD::Plumed p;
      int natoms=6,local=3;
      double timestep=0.001;
      p.cmd("setMPIComm",&world);
      p.cmd("setNatoms",&natoms);
      p.cmd("setTimestep",&timestep);
      p.cmd("setLog",log.get());
      p.cmd("init");
      for(const auto& line : lines) {
        p.cmd("readInputLine",line.c_str());
      }
      for(int step=0; step<5; ++step) {
        // Scattered ownership changes each step; then each rank is empty once.
        local=step<3 ? 3 : (rank==(step-3) ? 6 : 0);
        std::array<int,6> indices{};
        std::array<Vec,6> positions{},forces{};
        std::array<double,6> masses{};
        for(int i=0; i<local; ++i) {
          indices[i]=local==6 ? i : (rank+step)%2+2*i;
          positions[i]=indices[i]<4 ? x[indices[i]] : Vec{{7,8,9}};
          masses[i]=indices[i]<4 ? indices[i]+1 : 1;
        }
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
        // The cell is a root-owned host input; its force is exported only on
        // that rank. Compare the summed host virial with the serial result.
        Box totalVirial{};
        MPI_Allreduce(&virial[0][0],&totalVirial[0][0],9,MPI_DOUBLE,MPI_SUM,world);
        close(bias,expected.energy,"MPI bias",1e-10);
        for(int a=0; a<local; ++a)
          for(unsigned j=0; j<3; ++j) {
            close(forces[a][j],(indices[a]<4 ? expected.forces[indices[a]][j] : 0.0),"MPI forces",1e-10);
          }
        for(unsigned i=0; i<3; ++i)
          for(unsigned j=0; j<3; ++j) {
            close(totalVirial[i][j],expected.virial[i][j],"MPI virial",1e-10);
          }
      }
      if(rank==0) {
        output<<modifier<<": distributed forces and virial match serial with changing ownership\n";
      }
    }
  } catch(const std::exception& error) {
    std::fprintf(stderr,"rank %d: %s\n",rank,error.what());
    MPI_Abort(world,1);
  }
  MPI_Finalize();
  return 0;
}
