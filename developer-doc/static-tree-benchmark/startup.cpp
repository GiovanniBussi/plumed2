// Standalone wrapper: load exactly one kernel per process, including old builds.
#define __PLUMED_WRAPPER_IMPLEMENTATION 1
#define __PLUMED_WRAPPER_LINK_RUNTIME 1
#define __PLUMED_HAS_DLOPEN 1
#include "Plumed.h"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
int main(int argc,char** argv) {
  if(argc!=2) {
    return 2;
  }
  int n;
  std::ifstream coordinates("positions.dat");
  coordinates>>n;
  if(!coordinates || n<=0) {
    return 2;
  }
  std::vector<double> x(3*n),forces(3*n,0),mass(n,1),charge(n,0);
  for(auto& v:x) {
    coordinates>>v;
  }
  if(!coordinates) {
    return 2;
  }
  double box[3][3]= {{100,0,0},{0,100,0},{0,0,100}},virial[3][3]= {};
  int precision=sizeof(double),step=0;
  double timestep=1;
  auto now=[] { return std::chrono::steady_clock::now(); };
  auto ms=[](auto start,auto finish) {
    return std::chrono::duration<double,std::milli>(finish-start).count();
  };
  FILE* log=std::fopen("plumed.log","w");
  if(!log) {
    return 2;
  }
  const auto start=now();
  {
    auto p=PLMD::Plumed::dlopen(argv[1]);
    if(!p.valid()) {
      return 2;
    }
    const auto loaded=now();
    p.cmd("setRealPrecision",&precision);
    p.cmd("setNatoms",&n);
    p.cmd("setMDEngine","static-tree-startup-benchmark");
    p.cmd("setTimestep",&timestep);
    p.cmd("setLog",log);
    p.cmd("init");
    std::ifstream input("plumed.dat");
    std::string line;
    while(std::getline(input,line)) {
      p.cmd("readInputLine",line.c_str());
    }
    const auto setup=now();
    p.cmd("setStep",&step);
    p.cmd("setBox",box);
    p.cmd("setPositions",x.data());
    p.cmd("setMasses",mass.data());
    p.cmd("setCharges",charge.data());
    p.cmd("setForces",forces.data());
    p.cmd("setVirial",virial);
    const auto calc=now();
    p.cmd("calc");
    const auto finished=now();
    std::cout<<std::setprecision(12)<<ms(start,loaded)<<","<<ms(loaded,setup)<<","
             <<ms(calc,finished)<<","<<ms(loaded,setup)+ms(calc,finished)<<"\n";
  }
  std::fclose(log);
}
