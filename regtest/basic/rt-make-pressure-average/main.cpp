#include "plumed/wrapper/Plumed.h"
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
};

template<class T> class Simulation {
  PLMD::Plumed p;
public:
  Simulation(const std::string& input,int pressure) {
    const std::string file="case"+std::to_string(fileNumber++);
    { std::ofstream out(file+".dat"); out<<input; }
    p.cmd("setRealPrecision",int(sizeof(T)));
    p.cmd("setNatoms",6);
    p.cmd("setTimestep",T(.002));
    p.cmd("setPressureCouplingStride",pressure);
    p.cmd("setLogFile",file+".log");
    p.cmd("setPlumedDat",file+".dat");
    p.cmd("init");
  }
  Result<T> frame(int step) {
    std::array<T,18> x={{.1,.2,.3,.7,.3,.4,.3,.9,.2,.2,.4,1.1,1.1,.6,.9,-.2,1.3,.7}};
    x[15]+=T(.003*step);
    std::array<T,6> mass={{1,2,3,4,5,6}},charge={{0,0,0,0,0,0}};
    std::array<T,9> box={{3,0,0,.3,3.2,0,-.2,.1,3.4}};
    Result<T> result;
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
    p.cmd("setVirial",result.virial.data(),{3,3});
    p.cmd("prepareCalc");
    p.cmd("setEnergy",T(37.5+.05*step));
    for(unsigned i=0; i<9; ++i) {
      result.virial[i]=T(11+i+.03125*step);
    }
    p.cmd("performCalc");
    return result;
  }
};

template<class T>
void compare(const std::string& name,const std::string& input,int pressure,std::ofstream& out) {
  Simulation<T> raw(input,1),averaged(input,pressure);
  std::array<double,9> sum{};
  for(int step=0; step<=3*pressure; ++step) {
    const auto a=raw.frame(step),b=averaged.frame(step);
    require(a.force==b.force,name+": atomic forces changed");
    for(unsigned i=0; i<9; ++i) {
      const T incoming=T(11+i+.03125*step);
      sum[i]+=double(a.virial[i])-incoming;
      const double expected=(step%pressure==0 ? incoming+sum[i]/pressure : double(a.virial[i]));
      const double tolerance=(sizeof(T)==4 ? 2e-5 : 2e-12)*(1+std::abs(expected));
      require(std::abs(b.virial[i]-expected)<tolerance,name+": incorrect causal average");
    }
    if(step%pressure==0) {
      sum.fill(0);
    }
  }
  out<<(sizeof(T)==4 ? "float " : "double ")<<name<<": PASS\n";
}

template<class T> void tests(std::ofstream& out) {
  compare<T>("VOLUME55","v: VOLUME\na: RESTRAINT ARG=v AT=0 SLOPE=0.125 STRIDE=5\n",5,out);
  const std::string volume="v: VOLUME\na: RESTRAINT ARG=v AT=0 SLOPE=0.125 STRIDE=2\nb: RESTRAINT ARG=v AT=0 SLOPE=0.25 STRIDE=4\n";
  compare<T>("mixed26 inactive steps",volume,6,out);
  compare<T>("coprime25",volume,5,out);
  compare<T>("pressure stride one",volume,1,out);
  const std::string energy="e: ENERGY\nf: CUSTOM ARG=e FUNC=x*x/100 PERIODIC=NO\na: RESTRAINT ARG=f AT=0 SLOPE=0.1 STRIDE=2\n";
  compare<T>("indirect ENERGY",energy,6,out);
  compare<T>("ENERGY mixed26",energy+"v: VOLUME\nb: RESTRAINT ARG=v AT=0 SLOPE=0.25 STRIDE=4\n",6,out);
  compare<T>("coordinate4","d: DISTANCE ATOMS=1,6 NOPBC\na: RESTRAINT ARG=d AT=0.4 KAPPA=3 STRIDE=4\n",6,out);
  const std::string virtuals="c: CENTER ATOMS=1,2 NOPBC\ng: GHOST ATOMS=c,3,4 COORDINATES=0.2,0.3,0.4 NOPBC\nd: DISTANCE ATOMS=g,6 NOPBC\na: RESTRAINT ARG=d AT=0.4 KAPPA=3 STRIDE=2\nv: VOLUME\nb: RESTRAINT ARG=v AT=0 SLOPE=0.25 STRIDE=4\n";
  compare<T>("nested virtual atoms",virtuals,6,out);
  for(const std::string type : {"SIMPLE","OPTIMAL","OPTIMAL-FAST"}) {
    const std::string fit="c: CENTER ATOMS=1,2 NOPBC\nFIT_TO_TEMPLATE REFERENCE=reference.pdb TYPE="+type+" NOPBC\ng: GHOST ATOMS=c,3,4 COORDINATES=0.2,0.3,0.4 NOPBC\np: POSITION ATOM=g\na: RESTRAINT ARG=p.x,p.y,p.z AT=0.4,0.5,0.6 KAPPA=2,3,4 STRIDE=2\nv: VOLUME\nb: RESTRAINT ARG=v AT=0 SLOPE=0.25 STRIDE=4\n";
    compare<T>("fit "+type,fit,6,out);
  }
  compare<T>("RESET_CELL","RESET_CELL\np: POSITION ATOM=5\na: RESTRAINT ARG=p.x AT=0.2 KAPPA=2 STRIDE=2\nv: VOLUME\nb: RESTRAINT ARG=v AT=0 SLOPE=0.25 STRIDE=4\n",6,out);
}
}

int main() {
  std::ofstream out("output");
  tests<double>(out);
  tests<float>(out);
  for(const int invalid : {0,2}) {
    Simulation<double> p("v: VOLUME\na: RESTRAINT ARG=v AT=0 SLOPE=0.125 STRIDE=2\n",6);
    p.frame(0);
    bool threw=false;
    try {
      p.frame(invalid);
    } catch(const std::exception& e) {
      threw=std::string(e.what()).find("consecutive steps")!=std::string::npos;
    }
    require(threw,"duplicate or skipped step accepted");
    out<<(invalid==0 ? "duplicate" : "skipped")<<" step: rejected\n";
  }
}
