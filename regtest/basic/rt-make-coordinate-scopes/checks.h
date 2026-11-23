#ifndef COORDINATE_SCOPE_CHECKS_H
#define COORDINATE_SCOPE_CHECKS_H
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

struct Result {
  double energy=0;
  Positions forces{};
  Box virial{};
};

static Result evaluate(Positions x, Box box, const std::vector<std::string>& lines, int frames=1, double drift=0, bool seeded=false) {
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
    if(step>0) {
      for(unsigned i=0; i<4; ++i)
        for(unsigned j=0; j<3; ++j) {
          x[i][j]+=drift*(i+1)*(j+1);
        }
      box[1][0]+=0.5*drift;
    }
    result=Result();
    if(seeded) {
      for(unsigned i=0; i<4; ++i)
        for(unsigned j=0; j<3; ++j) {
          result.forces[i][j]=0.17*(i+1)*(j+1);
        }
      for(unsigned i=0; i<3; ++i)
        for(unsigned j=0; j<3; ++j) {
          result.virial[i][j]=0.23*(i+1)-0.31*(j+1);
        }
    }
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

static void compare(const Result& a, const Result& b, const std::string& name) {
  close(a.energy,b.energy,name+" energy");
  for(unsigned i=0; i<4; ++i)
    for(unsigned j=0; j<3; ++j) {
      close(a.forces[i][j],b.forces[i][j],name+" forces");
    }
  for(unsigned i=0; i<3; ++i)
    for(unsigned j=0; j<3; ++j) {
      close(a.virial[i][j],b.virial[i][j],name+" virial");
    }
}
static Result sum(Result a,const Result& b) {
  a.energy+=b.energy;
  for(unsigned i=0; i<4; ++i)
    for(unsigned j=0; j<3; ++j) {
      a.forces[i][j]+=b.forces[i][j];
    }
  for(unsigned i=0; i<3; ++i)
    for(unsigned j=0; j<3; ++j) {
      a.virial[i][j]+=b.virial[i][j];
    }
  return a;
}
static void append(std::vector<std::string>& a,const std::vector<std::string>& b) {
  a.insert(a.end(),b.begin(),b.end());
}
static std::vector<std::string> measured(const std::string& prefix) {
  return {prefix+"p: POSITION ATOM=4 NOPBC",prefix+"h: CELL",
          "RESTRAINT ARG="+prefix+"p.x,"+prefix+"p.y,"+prefix+"p.z,"+prefix+"h.bx,"+prefix+"h.cy AT=0.1,0.2,0.3,0.4,0.5 KAPPA=1,2,3,4,5"};
}
static void rejects(const Positions& x,const Box& box,const std::vector<std::string>& lines,const std::string& text) {
  try {
    evaluate(x,box,lines);
  } catch(const std::exception& e) {
    if(std::string(e.what()).find(text)!=std::string::npos) {
      return;
    }
    throw std::runtime_error("unexpected diagnostic: "+std::string(e.what()));
  }
  throw std::runtime_error("missing diagnostic: "+text);
}
static int scopeChecks() {
  const Positions x{{{{0.2,0.3,0.4}},{{1.3,0.1,0.5}},{{0.4,1.5,0.2}},{{1.7,0.8,1.9}}}};
  const Positions ref{{{{0.5,0.8,0.9}},{{1.8,0.9,0.6}},{{0.8,1.9,0.8}},{{1.5,1.1,2.3}}}};
  const Box box{{{{4.1,0.3,0.2}},{{0.9,4.5,0.6}},{{0.4,0.7,5.2}}}};
  writeReference(ref,"scope-ref.pdb");
  const std::vector<std::pair<std::string,std::string>> modifiers{
    {"FIT_TO_TEMPLATE REFERENCE=scope-ref.pdb TYPE=SIMPLE NOPBC","FIT_TO_TEMPLATE END"},
    {"FIT_TO_TEMPLATE REFERENCE=scope-ref.pdb TYPE=OPTIMAL NOPBC","FIT_TO_TEMPLATE END"},
    {"FIT_TO_TEMPLATE REFERENCE=scope-ref.pdb TYPE=OPTIMAL-FAST NOPBC","FIT_TO_TEMPLATE END"},
    {"RESET_CELL","RESET_CELL END"},
    {"WHOLEMOLECULES ENTITY0=1-4","WHOLEMOLECULES END"},
    {"WRAPAROUND ATOMS=4 AROUND=1","WRAPAROUND END"}};
  for(const auto& modifier:modifiers) {
    Positions input=x;
    if(modifier.first.find("WHOLEMOLECULES")==0 || modifier.first.find("WRAPAROUND")==0) {
      for(unsigned j=0; j<3; ++j) {
        input[3][j]+=box[0][j];
      }
    }
    for(int frames: {
          1,3
        }) {
      auto inside=std::vector<std::string> {modifier.first};
      append(inside,measured("i"));
      auto outside=measured("o");
      auto combined=inside;
      combined.push_back(modifier.second);
      append(combined,outside);
      compare(evaluate(input,box,combined,frames),sum(evaluate(input,box,inside,frames),evaluate(input,box,outside,frames)),modifier.first+" isolation");
      auto restored=std::vector<std::string> {modifier.first,modifier.second};
      append(restored,outside);
      compare(evaluate(input,box,restored,frames),evaluate(input,box,outside,frames),modifier.first+" restoration");
    }
    {
      auto lines=std::vector<std::string> {modifier.first};
      append(lines,measured("i"));
      lines.push_back(modifier.second);
      append(lines,measured("o"));
      derivatives(input,box,lines,modifier.first);
    }
  }
  Positions changed=x;
  Box changedBox=box;
  for(unsigned i=0; i<4; ++i)
    for(unsigned j=0; j<3; ++j) {
      changed[i][j]+=0.03*(i+1)*(j+1);
    }
  changedBox[1][0]+=0.015;
  for(const auto& modifier:modifiers) {
    std::vector<std::string> lines{modifier.first};
    append(lines,measured("i"));
    lines.push_back(modifier.second);
    append(lines,measured("o"));
    compare(evaluate(x,box,lines,4,0.01),evaluate(changed,changedBox,lines),"changing coordinates and box");
    compare(evaluate(x,box,lines,1,0,true),sum(evaluate(x,box,lines),evaluate(x,box, {},1,0,true)),"incoming host forces");
  }
  // Every modifier composition must route middle/outer forces around exactly
  // the transformations that were in effect where each CV was calculated.
  Positions imageInput=x;
  for(unsigned j=0; j<3; ++j) {
    imageInput[3][j]+=box[0][j];
  }
  for(const auto& outer:modifiers)
    for(const auto& inner:modifiers) {
      std::vector<std::string> combined{outer.first,inner.first};
      append(combined,measured("i"));
      combined.push_back(inner.second);
      append(combined,measured("m"));
      combined.push_back(outer.second);
      append(combined,measured("o"));
      std::vector<std::string> inside{outer.first,inner.first};
      append(inside,measured("i"));
      std::vector<std::string> middle{outer.first};
      append(middle,measured("m"));
      const auto expected=sum(sum(evaluate(imageInput,box,inside,3),evaluate(imageInput,box,middle,3)),evaluate(imageInput,box,measured("o"),3));
      compare(evaluate(imageInput,box,combined,3),expected,"modifier composition "+outer.first+" / "+inner.first);
    }
  // Deferred bias on an interior CV, earlier/later CVs, and strict mixed nesting.
  std::vector<std::string> nested{
    "before: POSITION ATOM=4 NOPBC",modifiers[1].first,"fit: POSITION ATOM=4 NOPBC",
    "RESET_CELL","reset: POSITION ATOM=4 NOPBC","RESET_CELL END",
    "fit2: POSITION ATOM=4 NOPBC","FIT_TO_TEMPLATE END","after: POSITION ATOM=4 NOPBC",
    "RESTRAINT ARG=before.x,fit.y,reset.z,fit2.x,after.y AT=0,0,0,0,0 KAPPA=1,2,3,4,5"};
  Result nestedExpected;
  const std::vector<std::string> args{"before.x","fit.y","reset.z","fit2.x","after.y"};
  const std::vector<std::vector<std::string>> contexts{
    {},{modifiers[1].first},{modifiers[1].first,"RESET_CELL"},{modifiers[1].first},{}};
  const std::vector<std::string> labels{"before","fit","reset","fit2","after"};
  for(unsigned i=0; i<args.size(); ++i) {
    auto oracle=contexts[i];
    oracle.push_back(labels[i]+": POSITION ATOM=4 NOPBC");
    oracle.push_back("RESTRAINT ARG="+args[i]+" AT=0 KAPPA="+std::to_string(i+1));
    nestedExpected=sum(nestedExpected,evaluate(x,box,oracle,3));
  }
  compare(evaluate(x,box,nested,3),nestedExpected,"mixed nesting");
  derivatives(x,box,nested,"mixed nesting");
  auto sameType=std::vector<std::string> {modifiers[0].first,modifiers[1].first};
  append(sameType,measured("i"));
  sameType.push_back(modifiers[1].second);
  append(sameType,measured("m"));
  sameType.push_back(modifiers[0].second);
  append(sameType,measured("o"));
  derivatives(x,box,sameType,"same-type nesting");
  std::vector<std::string> deferred{modifiers[1].first,"i: POSITION ATOM=4 NOPBC",modifiers[1].second,
                                    "RESTRAINT ARG=i.x,i.y AT=0,0 KAPPA=2,3"};
  derivatives(x,box,deferred,"deferred bias");
  // Earlier and scope-local virtual atoms; later modifiers must ignore expired ones.
  const std::vector<std::string> vprefix{"old: COM ATOMS=1,2 NOPBC",modifiers[1].first,
                                         "v: COM ATOMS=3,4 NOPBC","g: GHOST ATOMS=1,2,3 COORDINATES=0.2,0.3,0.4",
                                         "vpos: POSITION ATOM=v NOPBC","gpos: POSITION ATOM=g NOPBC",modifiers[1].second};
  auto vatoms=vprefix;
  append(vatoms, {"outside: POSITION ATOM=old NOPBC","RESTRAINT ARG=vpos.x,gpos.y,outside.z AT=0,0,0 KAPPA=1,2,3"});
  derivatives(x,box,vatoms,"virtual atoms");
  auto beforeLater=vatoms;
  beforeLater.insert(beforeLater.end()-1,modifiers[0].first);
  compare(evaluate(x,box,beforeLater),evaluate(x,box,vatoms),"expired virtual atom bypass");
  auto directVirtual=vprefix;
  directVirtual.push_back("RESTRAINT ARG=v.x,g.y AT=0,0 KAPPA=1,2");
  auto directLater=directVirtual;
  directLater.insert(directLater.end()-1,modifiers[0].first);
  compare(evaluate(x,box,directLater),evaluate(x,box,directVirtual),"expired direct virtual outputs");
  rejects(x,box, {"FIT_TO_TEMPLATE END"},"unmatched");
  rejects(x,box, {"WHOLEMOLECULES ENTITY0=1-4",modifiers[0].first,"WHOLEMOLECULES END"},"expected FIT_TO_TEMPLATE END");
  rejects(x,box, {modifiers[0].first,"FIT_TO_TEMPLATE END REFERENCE=scope-ref.pdb"},"REFERENCE");
  for(const std::string atom: {
        "v","5","@allatoms"
      }) {
    rejects(x,box, {modifiers[0].first,"v: COM ATOMS=1,2",modifiers[0].second,"p: COM ATOMS="+atom},"closed FIT_TO_TEMPLATE");
  }
  rejects(x,box, {modifiers[0].first,"v: COM ATOMS=1,2","grp: GROUP ATOMS=v",modifiers[0].second,"p: POSITION ATOM=grp"},"closed FIT_TO_TEMPLATE");
  // INCLUDE and incremental input share the same stack.
  std::ofstream("scope-include.dat")<<"FIT_TO_TEMPLATE END\n";
  std::vector<std::string> included{modifiers[1].first};
  append(included,measured("i"));
  included.push_back("INCLUDE FILE=scope-include.dat");
  append(included,measured("o"));
  std::vector<std::string> explicitEnd{modifiers[1].first};
  append(explicitEnd,measured("i"));
  explicitEnd.push_back(modifiers[1].second);
  append(explicitEnd,measured("o"));
  compare(evaluate(x,box,included),evaluate(x,box,explicitEnd),"INCLUDE closure");
  // END inherits scheduling: skipped steps do not consume a prior snapshot.
  auto stride=std::vector<std::string> {"FIT_TO_TEMPLATE REFERENCE=scope-ref.pdb TYPE=OPTIMAL STRIDE=2 NOPBC"};
  append(stride,measured("i"));
  stride.push_back("FIT_TO_TEMPLATE END");
  append(stride,measured("o"));
  auto unmodified=measured("i");
  append(unmodified,measured("o"));
  compare(evaluate(x,box,stride,2),evaluate(x,box,unmodified,2),"skipped stride");
  auto active=std::vector<std::string> {modifiers[1].first};
  append(active,measured("i"));
  active.push_back(modifiers[1].second);
  append(active,measured("o"));
  compare(evaluate(x,box,stride,3),evaluate(x,box,active,3),"active stride");
  auto forced=stride;
  forced.push_back("PRINT ARG=@0 FILE=activation STRIDE=1");
  // The first action receives the automatic label @0. Reading its output
  // activates it even on a skipped pilot step; END must be activated with it.
  compare(evaluate(x,box,forced,2),evaluate(x,box,active,2),"dependency-activated scope");
  // Scope stacks belong to individual kernels, and clear drops all pointers.
  {
    std::unique_ptr<FILE,decltype(&std::fclose)> log(std::tmpfile(),std::fclose);
    PLMD::Plumed first,second;
    int natoms=4;
    for(auto* kernel: {
          &first,&second
        }) {
      kernel->cmd("setNatoms",&natoms);
      kernel->cmd("setLog",log.get());
      kernel->cmd("init");
    }
    first.cmd("readInputLine",modifiers[0].first.c_str());
    first.cmd("readInputLine","local: COM ATOMS=1,2");
    bool rejected=false;
    try {
      second.cmd("readInputLine","FIT_TO_TEMPLATE END");
    } catch(const std::exception&) {
      rejected=true;
    }
    if(!rejected) {
      throw std::runtime_error("scope leaked between kernels");
    }
    first.cmd("readInputLine","FIT_TO_TEMPLATE END");
    first.cmd("clear");
    rejected=false;
    try {
      first.cmd("readInputLine","FIT_TO_TEMPLATE END");
    } catch(const std::exception&) {
      rejected=true;
    }
    if(!rejected) {
      throw std::runtime_error("scope survived clear");
    }
    first.cmd("readInputLine",modifiers[0].first.c_str());
    first.cmd("readInputLine","FIT_TO_TEMPLATE END");
    first.cmd("readInputLine","local: COM ATOMS=1,2");
    first.cmd("readInputLine","outside: POSITION ATOM=4 NOPBC");
    first.cmd("readInputLine","RESTRAINT ARG=outside.x AT=0 KAPPA=1");
    Positions resetPositions=x;
    Box resetBox=box;
    Result resetResult;
    std::array<double,4> masses{{1,2,3,4}};
    int step=0;
    first.cmd("setStep",&step);
    first.cmd("setPositions",&resetPositions[0][0]);
    first.cmd("setMasses",masses.data());
    first.cmd("setBox",&resetBox[0][0]);
    first.cmd("setForces",&resetResult.forces[0][0]);
    first.cmd("setVirial",&resetResult.virial[0][0]);
    first.cmd("calc");
    first.cmd("getBias",&resetResult.energy);
    compare(resetResult,evaluate(x,box, {"outside: POSITION ATOM=4 NOPBC","RESTRAINT ARG=outside.x AT=0 KAPPA=1"}),"calculation after clear");
  }
  std::ofstream("output")<<"Coordinate scope checks passed\n";
  return 0;
}
#endif
