#include "plumed/wrapper/Plumed.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
int fileNumber=0;
void require(bool condition,const std::string& message) {
  if(!condition) {
    throw std::runtime_error(message);
  }
}

template<class T> struct Result {
  std::array<T,18> force;
  std::array<T,9> virial;
  T bias=0;
};

template<class T>
Result<T> evaluate(const std::string& input,int pressure,int step,bool repeat=false,
                   bool energyOnly=false,bool nullVirial=false) {
  const std::string file="case"+std::to_string(fileNumber++);
  { std::ofstream out(file+".dat"); out<<input; }
  PLMD::Plumed p;
  int precision=sizeof(T),natoms=6;
  T dt=0.002;
  p.cmd("setRealPrecision",precision);
  p.cmd("setNatoms",natoms);
  p.cmd("setTimestep",dt);
  p.cmd("setPressureCouplingStride",pressure);
  p.cmd("setLogFile",file+".log");
  p.cmd("setPlumedDat",file+".dat");
  p.cmd("init");
  std::array<T,18> x={{.1,.2,.3,.7,.3,.4,.3,.9,.2,.2,.4,1.1,1.1,.6,.9,-.2,1.3,.7}};
  std::array<T,6> mass={{1,2,3,4,5,6}},charge={{0,0,0,0,0,0}};
  std::array<T,9> box={{3,0,0,.3,3.2,0,-.2,.1,3.4}};
  const std::array<T,9> incoming={{11,2,3,5,17,7,8,9,23}};
  Result<T> result;
  for(int evaluation=0; evaluation<(repeat?2:1); ++evaluation) {
    for(unsigned i=0; i<18; ++i) {
      result.force[i]=T(.125*(i+1));
    }
    result.virial.fill(0);
    p.cmd("setStep",step);
    p.cmd("setPositions",x.data(),{6,3});
    p.cmd("setMasses",mass.data(),{6});
    p.cmd("setCharges",charge.data(),{6});
    p.cmd("setBox",box.data(),{3,3});
    p.cmd("setForces",result.force.data(),{6,3});
    if(!nullVirial) {
      p.cmd("setVirial",result.virial.data(),{3,3});
    }
    // A legacy host can fill an already-bound virial after prepareCalc.
    p.cmd("prepareCalc");
    T energy=37.5;
    p.cmd("setEnergy",energy);
    result.virial=incoming;
    p.cmd(energyOnly ? "performCalcNoForces" : (repeat ? "performCalcNoUpdate" : "performCalc"));
    p.cmd("getBias",&result.bias);
  }
  return result;
}

template<class T>
void compare(const std::string& name,const std::string& input,int pressure,int step,
             int divisor,std::ofstream& out,bool repeat=false,bool energyOnly=false) {
  const auto raw=evaluate<T>(input,1,step,repeat,energyOnly);
  const auto corrected=evaluate<T>(input,pressure,step,repeat,energyOnly);
  require(raw.force==corrected.force,name+": atomic force changed");
  require(raw.bias==corrected.bias,name+": bias energy changed");
  const std::array<T,9> incoming={{11,2,3,5,17,7,8,9,23}};
  for(unsigned i=0; i<9; ++i) {
    const double expected=incoming[i]+(double(raw.virial[i])-incoming[i])/divisor;
    const double tolerance=(sizeof(T)==4 ? 2e-5 : 2e-12)*(1+std::abs(expected));
    require(std::abs(corrected.virial[i]-expected)<tolerance,name+": wrong virial delta");
  }
  out<<(sizeof(T)==4 ? "float " : "double ")<<name<<": PASS\n";
}

void rejected(const std::string& name,const std::string& input,int pressure,
              const std::string& text,std::ofstream& out) {
  bool threw=false;
  try {
    evaluate<double>(input,pressure,0);
  } catch(const std::exception& e) {
    require(std::string(e.what()).find(text)!=std::string::npos,name+": unexpected error");
    threw=true;
  }
  require(threw,name+": incompatible input accepted");
  out<<name<<": rejected\n";
}

template<class T> void tests(std::ofstream& out) {
  const std::string volume="v: VOLUME\na: RESTRAINT ARG=v AT=0 SLOPE=0.125 STRIDE=2\n"
                           "b: RESTRAINT ARG=v AT=0 SLOPE=0.25 STRIDE=4\n";
  compare<T>("mixed shared CV",volume,6,4,2,out);
  compare<T>("one active bias",volume,6,2,2,out);
  compare<T>("inactive biases",volume,6,3,2,out);
  compare<T>("coprime",volume,5,4,1,out);
  compare<T>("repeat no-update",volume,6,4,2,out,true);
  compare<T>("no-force trial",volume,6,4,1,out,false,true);
  const std::string energy="e: ENERGY\nf: CUSTOM ARG=e FUNC=x*x/100 PERIODIC=NO\n"
                           "a: RESTRAINT ARG=f AT=0 SLOPE=0.1 STRIDE=2\n";
  compare<T>("ENERGY indirect",energy,6,4,2,out);
  compare<T>("ENERGY plus VOLUME",energy+"v: VOLUME\nb: RESTRAINT ARG=v AT=0 SLOPE=0.25 STRIDE=4\n",6,4,2,out);
  compare<T>("two ENERGY biases",energy+"b: RESTRAINT ARG=e AT=0 SLOPE=0.05 STRIDE=2\n",6,4,2,out);
  compare<T>("ENERGY repeat",energy,6,4,2,out,true);
  const std::string virtuals="c: CENTER ATOMS=1,2 NOPBC\ng: GHOST ATOMS=c,3,4 COORDINATES=0.2,0.3,0.4 NOPBC\n"
                             "d: DISTANCE ATOMS=g,6 NOPBC\na: RESTRAINT ARG=d AT=0.4 KAPPA=3 STRIDE=2\n"
                             "v: VOLUME\nb: RESTRAINT ARG=v AT=0 SLOPE=0.25 STRIDE=4\n";
  compare<T>("nested virtual atoms",virtuals,6,4,2,out);
  for(const std::string type : {"SIMPLE","OPTIMAL","OPTIMAL-FAST"}) {
    const std::string fit="c: CENTER ATOMS=1,2 NOPBC\nFIT_TO_TEMPLATE REFERENCE=reference.pdb TYPE="+type+" NOPBC\n"
                          "g: GHOST ATOMS=c,3,4 COORDINATES=0.2,0.3,0.4 NOPBC\n"
                          "p: POSITION ATOM=g\na: RESTRAINT ARG=p.x,p.y,p.z AT=0.4,0.5,0.6 KAPPA=2,3,4 STRIDE=2\n"
                          "v: VOLUME\nb: RESTRAINT ARG=v AT=0 SLOPE=0.25 STRIDE=4\n";
    compare<T>("fit "+type,fit,6,4,2,out);
  }
  const std::string reset="RESET_CELL\np: POSITION ATOM=5\na: RESTRAINT ARG=p.x AT=0.2 KAPPA=2 STRIDE=2\n"
                          "v: VOLUME\nb: RESTRAINT ARG=v AT=0 SLOPE=0.25 STRIDE=4\n";
  compare<T>("reset cell",reset,6,4,2,out);
  // Null virial is permitted on a rank that owns no cell-force buffer.
  evaluate<T>("e: ENERGY\na: RESTRAINT ARG=e AT=0 SLOPE=0.125 STRIDE=2\n",6,4,false,false,true);
  out<<(sizeof(T)==4 ? "float " : "double ")<<"absent virial pointer: PASS\n";
}
}

int main() {
  std::ofstream out("output");
  tests<double>(out);
  tests<float>(out);
  rejected("different gcd","v: VOLUME\na: RESTRAINT ARG=v AT=0 SLOPE=1 STRIDE=2\nb: RESTRAINT ARG=v AT=0 SLOPE=1 STRIDE=3\n",6,"Incompatible pressure-coupling gcd",out);
  rejected("ENERGY different strides","e: ENERGY\nf: CUSTOM ARG=e FUNC=x*x PERIODIC=NO\na: RESTRAINT ARG=e AT=0 SLOPE=0.1 STRIDE=2\nb: RESTRAINT ARG=f AT=0 SLOPE=0.1 STRIDE=4\n",6,"ENERGY-dependent biases must have the same force STRIDE",out);
  rejected("invalid pressure stride","",0,"Pressure coupling stride must be positive",out);
}
