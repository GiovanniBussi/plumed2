#ifndef PROJECT_COORDINATES_CHECKS_H
#define PROJECT_COORDINATES_CHECKS_H

#include "plumed/wrapper/Plumed.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using Vec = std::array<double,3>;
using Box = std::array<Vec,3>;
using Positions = std::array<Vec,4>;

static Vec plus(Vec a, const Vec& b, double factor=1) {
  for(unsigned i=0; i<3; ++i) {
    a[i]+=factor*b[i];
  }
  return a;
}
static double dot(const Vec& a, const Vec& b) {
  return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
static Vec cross(const Vec& a, const Vec& b) {
  return {{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}};
}
static Vec projection(const Vec& r, const Box& box, const std::string& axes) {
  const double volume=dot(box[0],cross(box[1],box[2]));
  Vec result{};
  for(unsigned i=0; i<3; ++i) {
    if(axes.find('a'+i)!=std::string::npos) {
      const double s=dot(r,cross(box[(i+1)%3],box[(i+2)%3]))/volume;
      result=plus(result,box[i],s);
    }
  }
  return result;
}
// An independent lattice enumeration oracle, without the auxiliary cell.
static double projectedDistance2(Vec d, const Box& box, const std::string& axes) {
  d=projection(d,box,axes);
  double best=1e100;
  for(int i=-4; i<=4; ++i)
    for(int j=-4; j<=4; ++j)
      for(int k=-4; k<=4; ++k) {
        if((axes.find('a')==std::string::npos && i) ||
            (axes.find('b')==std::string::npos && j) ||
            (axes.find('c')==std::string::npos && k)) {
          continue;
        }
        Vec image=plus(plus(plus(d,box[0],i),box[1],j),box[2],k);
        best=std::min(best,dot(image,image));
      }
  return best;
}

struct Result {
  double energy=0;
  Positions forces{};
  Box virial{};
};

static Result evaluate(Positions x, Box box, const std::vector<std::string>& lines, int frames=1) {
  std::unique_ptr<FILE,decltype(&std::fclose)> log(std::tmpfile(),std::fclose);
  if(!log) {
    throw std::runtime_error("cannot open temporary log");
  }
  PLMD::Plumed p;
  int natoms=4;
  double timestep=0.001;
  p.cmd("setNatoms",&natoms);
  p.cmd("setTimestep",&timestep);
  p.cmd("setLog",log.get());
  p.cmd("init");
  for(const auto& line : lines) {
    p.cmd("readInputLine",line.c_str());
  }
  std::array<double,4> masses{{1,2,3,4}};
  Result result;
  for(int step=0; step<frames; ++step) {
    result=Result();
    p.cmd("setStep",&step);
    p.cmd("setPositions",&x[0][0]);
    p.cmd("setMasses",masses.data());
    p.cmd("setBox",&box[0][0]);
    p.cmd("setForces",&result.forces[0][0]);
    p.cmd("setVirial",&result.virial[0][0]);
    p.cmd("calc");
    p.cmd("getBias",&result.energy);
  }
  return result;
}

static void close(double actual, double expected, const std::string& description, double tolerance=3e-6) {
  if(!std::isfinite(actual) || std::fabs(actual-expected)>tolerance*(1+std::fabs(expected))) {
    throw std::runtime_error(description+": got "+std::to_string(actual)+", expected "+std::to_string(expected));
  }
}

static void derivatives(const Positions& x, const Box& box, const std::vector<std::string>& lines,
                        const std::string& name) {
  const Result result=evaluate(x,box,lines,3);
  const double h=1e-6;
  for(unsigned atom=0; atom<x.size(); ++atom)
    for(unsigned j=0; j<3; ++j) {
      Positions xp=x,xm=x;
      xp[atom][j]+=h;
      xm[atom][j]-=h;
      const double force=-(evaluate(xp,box,lines).energy-evaluate(xm,box,lines).energy)/(2*h);
      close(result.forces[atom][j],force,name+" force");
    }
  for(unsigned i=0; i<3; ++i)
    for(unsigned j=0; j<3; ++j) {
      Positions xp=x,xm=x;
      Box bp=box,bm=box;
      for(unsigned k=0; k<3; ++k) {
        bp[k][j]+=h*box[k][i];
        bm[k][j]-=h*box[k][i];
      }
      for(unsigned atom=0; atom<x.size(); ++atom) {
        xp[atom][j]+=h*x[atom][i];
        xm[atom][j]-=h*x[atom][i];
      }
      const double virial=(evaluate(xp,bp,lines).energy-evaluate(xm,bm,lines).energy)/(2*h);
      close(result.virial[i][j],virial,name+" virial "+std::to_string(i)+std::to_string(j));
    }
}

static void writeReference(const Positions& x, const std::string& path) {
  FILE* f=std::fopen(path.c_str(),"w");
  if(!f) {
    throw std::runtime_error("cannot write reference");
  }
  for(unsigned i=0; i<x.size(); ++i) {
    std::fprintf(f,"ATOM  %5u  CA  ALA A%4u    %8.3f%8.3f%8.3f%6.2f%6.2f\n",
                 i+1,i+1,10*x[i][0],10*x[i][1],10*x[i][2],1.0,1.0);
  }
  std::fprintf(f,"END\n");
  std::fclose(f);
}

int serialProjectionChecks() {
  const Box box{{{{3.1,0.4,-0.2}},{{0.7,2.7,0.5}},{{0.8,-0.3,3.4}}}};
  const Positions x{{{{0.2,0.4,0.3}},{{2.5,0.2,1.1}},{{0.7,1.6,0.2}},{{1.4,0.8,1.7}}}};
  std::ofstream output("output");
  output<<std::unitbuf;
  writeReference(x,"fit.pdb");
  for(const std::string axes : {
        "ab","ac","bc","a","b","c"
      }) {
    const std::string command="PROJECT_COORDINATES "+std::string(axes.size()==2 ? "PLANE=" : "AXIS=")+axes;
    std::vector<std::string> lines{command,"d: DISTANCE ATOMS=1,2","RESTRAINT ARG=d AT=0 KAPPA=2"};
    const Result result=evaluate(x,box,lines);
    close(result.energy,projectedDistance2(plus(x[1],x[0],-1),box,axes),"periodic oracle "+axes,1e-11);
    Positions image=x;
    image[1]=plus(image[1],box[0],2);
    image[1]=plus(image[1],box[1],-1);
    image[1]=plus(image[1],box[2],1);
    close(evaluate(image,box,lines).energy,result.energy,"image invariance "+axes,1e-11);
    // Nonzero normal forces, an explicit auxiliary-cell bias and an original-
    // frame CV exercise all parts of the reverse map and delayed bias ordering.
    lines.insert(lines.begin(),"original: DISTANCE ATOMS=1,3 NOPBC COMPONENTS");
    lines.push_back("p: POSITION ATOM=3 NOPBC");
    lines.push_back("cell: CELL");
    lines.push_back("RESTRAINT ARG=p.x,p.y,p.z,cell.ax,cell.bz,cell.cy,original.z AT=0.4,0.2,0.7,2,0.3,0.2,0.1 KAPPA=1,2,3,1,2,3,4");
    derivatives(x,box,lines,axes);
    output<<axes<<": lattice oracle, image invariance, forces and full virial pass\n";
  }
  for(bool nl : {
        false,true
      }) {
    const std::vector<std::string> lines{
      "PROJECT_COORDINATES PLANE=ab",
      std::string("c: COORDINATION GROUPA=1,2 GROUPB=3,4 SWITCH={RATIONAL R_0=1 NN=6 MM=12 D_MAX=2.5}")+(nl ? " NLIST NL_CUTOFF=2.5 NL_STRIDE=2" : ""),
      "RESTRAINT ARG=c AT=0 KAPPA=2"};
    double coordination=0;
    for(unsigned a=0; a<2; ++a)
      for(unsigned b=2; b<4; ++b) {
        const double d2=projectedDistance2(plus(x[b],x[a],-1),box,"ab");
        const double tail=1/(1+std::pow(2.5,6));
        if(d2<2.5*2.5) {
          coordination+=(1/(1+d2*d2*d2)-tail)/(1-tail);
        }
      }
    close(evaluate(x,box,lines,3).energy,coordination*coordination,"coordination oracle",1e-11);
    derivatives(x,box,lines,"coordination");
  }
  output<<"COORDINATION: direct and neighbor-list oracles and derivatives pass\n";
  Positions reference=x;
  for(auto& r : reference) {
    r[2]=0;
  }
  writeReference(reference,"projected.pdb");
  const Box rectangular{{{{3,0,0}},{{0,3,0}},{{0,0,3}}}};
  const std::vector<std::string> rmsd{
    "PROJECT_COORDINATES PLANE=ab",
    "r: RMSD REFERENCE=projected.pdb TYPE=SIMPLE NOPBC SQUARED",
    "RESTRAINT ARG=r AT=0 SLOPE=1"};
  close(evaluate(x,rectangular,rmsd).energy,0,"projected reference RMSD",1e-12);
  Positions displaced=x;
  displaced[0][0]+=0.21;
  derivatives(displaced,box,rmsd,"RMSD");
  output<<"RMSD: user-projected reference and derivatives pass\n";
  for(const std::string type : {
        "SIMPLE","OPTIMAL","OPTIMAL-FAST"
      }) {
    derivatives(x,box, {"PROJECT_COORDINATES PLANE=ab",
                        "FIT_TO_TEMPLATE REFERENCE=fit.pdb TYPE="+type+" NOPBC",
                        "p: POSITION ATOM=2 NOPBC",
                        "RESTRAINT ARG=p.x,p.y,p.z AT=0.2,0.3,0.4 KAPPA=1,2,3"
                       },"fit after "+type);
    derivatives(x,box, {"FIT_TO_TEMPLATE REFERENCE=fit.pdb TYPE="+type+" NOPBC",
                        "PROJECT_COORDINATES PLANE=ab",
                        "p: POSITION ATOM=2 NOPBC",
                        "RESTRAINT ARG=p.x,p.y,p.z AT=0.2,0.3,0.4 KAPPA=1,2,3"
                       },"fit before "+type);
  }
  output<<"FIT_TO_TEMPLATE: both orderings and all fit types pass\n";
  derivatives(x,box, {"v: COM ATOMS=1,3 NOPBC","PROJECT_COORDINATES PLANE=ab",
                      "g: GHOST ATOMS=1,2,3 COORDINATES=0.2,0.3,0.4",
                      "d: DISTANCE ATOMS=v,g NOPBC","RESTRAINT ARG=d AT=0 KAPPA=2"
                     },"virtual atoms");
  derivatives(x,box, {"PROJECT_COORDINATES PLANE=ab","PROJECT_COORDINATES PLANE=ac",
                      "d: DISTANCE ATOMS=1,2","RESTRAINT ARG=d AT=0 KAPPA=2"
                     },"successive projections");
  derivatives(x,box, {"PROJECT_COORDINATES PLANE=ab","RESET_CELL TYPE=TRIANGULAR",
                      "p: POSITION ATOM=2 NOPBC","RESTRAINT ARG=p.x,p.y,p.z AT=0.2,0.3,0.4 KAPPA=1,2,3"
                     },"reset cell");
  output<<"Virtual atoms, successive projections and RESET_CELL derivatives pass\n";
  for(const std::string command : {
        "PROJECT_COORDINATES","PROJECT_COORDINATES PLANE=xy",
        "PROJECT_COORDINATES AXIS=z","PROJECT_COORDINATES PLANE=ab AXIS=c",
        "PROJECT_COORDINATES PLANE=ab END","PROJECT_COORDINATES PLANE=ab STRIDE=2"
      }) {
    bool rejected=false;
    try {
      evaluate(x,box, {command});
    } catch(const PLMD::Plumed::Exception&) {
      rejected=true;
    }
    if(!rejected) {
      throw std::runtime_error("invalid syntax accepted: "+command);
    }
  }
  bool rejected=false;
  try {
    evaluate(x,Box(), {"PROJECT_COORDINATES PLANE=ab"});
  } catch(const PLMD::Plumed::Exception&) {
    rejected=true;
  }
  if(!rejected) {
    throw std::runtime_error("zero cell accepted");
  }
  output<<"Invalid syntax and missing cell rejected\n";
  return 0;
}

#endif
