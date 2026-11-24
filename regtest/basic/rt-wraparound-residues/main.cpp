#include "plumed/core/PlumedMain.h"
#include "plumed/tools/Vector.h"
#include "plumed/tools/Tensor.h"
#include "plumed/tools/Pbc.h"
#include <array>
#include <cstdio>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

using namespace PLMD;
void require(bool ok,const std::string& message) {
  if(!ok) {
    throw std::runtime_error(message);
  }
}
const std::vector<double> rx= {0,0.1,0.4,0.8,0.9,1,1.4,1.8,1.9,2.2,2.8,2.9,3.2,3.6};
void reference() {
  FILE* f=std::fopen("reference.pdb","w");
  require(f,"reference output");
  for(unsigned i=0; i<rx.size(); ++i) {
    if(i==10) {
      std::fprintf(f,"TER\n");
    }
    const char chain=i<7 ? 'A' : 'B';
    const char insertion=i>=3 && i<7 ? 'A' : ' ';
    std::fprintf(f,"ATOM  %5u  C   ALA %c%4d%c   %8.3f%8.3f%8.3f  1.00  1.00           C\n",
                 i+1,chain,i==13 ? 2 : 1,insertion,10*rx[i],0.0,0.0);
  }
  std::fprintf(f,"END\n");
  std::fclose(f);
}
struct Context {
  FILE* log=std::tmpfile();
  std::unique_ptr<PlumedMain> p=std::make_unique<PlumedMain>();
  Context(const std::string& flags="WHOLE SMALL_RESIDUES") {
    require(log,"temporary log");
    int precision=sizeof(double),n=18;
    double dt=1;
    p->cmd("setRealPrecision",&precision);
    p->cmd("setNatoms",&n);
    p->cmd("setTimestep",&dt);
    p->cmd("setLog",log);
    p->cmd("init");
    if(flags!="NONE") {
      line("MOLINFO STRUCTURE=reference.pdb PYTHON_BIN=no "+flags);
    }
  }
  void line(const std::string& input) {
    std::istringstream lines(input);
    std::string line;
    while(std::getline(lines,line)) {
      p->cmd("readInputLine",line.c_str());
    }
  }
  ~Context() {
    p.reset();
    std::fclose(log);
  }
};
using Positions=std::array<Vector,18>;
Positions calculate(const std::string& input,const Positions& start,const Tensor& box,int step=0) {
  std::remove("wrapped.xyz");
  {
    Context c;
    c.line(input);
    c.line("DUMPATOMS ATOMS=1-18 FILE=wrapped.xyz PRECISION=12");
    auto positions=start;
    Tensor cell=box,virial;
    std::array<double,18> masses{},charges{};
    masses.fill(1);
    Positions forces{};
    c.p->cmd("setStep",&step);
    c.p->cmd("setBox",cell.data());
    c.p->cmd("setPositions",&positions[0][0]);
    c.p->cmd("setMasses",masses.data());
    c.p->cmd("setCharges",charges.data());
    c.p->cmd("setForces",&forces[0][0]);
    c.p->cmd("setVirial",virial.data());
    c.p->cmd("calc");
  }
  std::ifstream f("wrapped.xyz");
  std::string line,label;
  std::getline(f,line);
  require(line=="18","DUMPATOMS atom count");
  std::getline(f,line);
  Positions result;
  for(auto& x:result) {
    f>>label>>x[0]>>x[1]>>x[2];
    require(bool(f),"DUMPATOMS coordinates");
  }
  return result;
}
void same(const Positions& a,const Positions& b,const std::string& message) {
  for(unsigned i=0; i<a.size(); ++i) {
    require((a[i]-b[i]).modulo()<1e-10,message+" atom "+std::to_string(i+1));
  }
}
void rejects(const std::string& flags,const std::string& input,const std::string& diagnostic) {
  bool rejected=false;
  try {
    Context c(flags);
    c.line(input);
  } catch(const std::exception& e) {
    rejected=std::string(e.what()).find(diagnostic)!=std::string::npos;
  }
  require(rejected,"expected diagnostic: "+diagnostic);
}
int main() {
  std::ofstream out("output");
  reference();
  for(bool skew: {
        false,true
      }) {
    const Tensor box=skew ? Tensor(4,0,0,1,4,0,-0.5,0.8,4) : Tensor(4,0,0,0,4,0,0,0,4);
    Positions start{};
    for(unsigned i=0; i<start.size(); ++i) {
      start[i]=Vector(i<rx.size() ? rx[i] : 0,0.02*i,0);
      start[i]+=matmul(Vector(int(i%5)-2,int(i%3)-1,int(i%7)-3),box);
    }
    // Deliberately change current geometry: the anchor remains reference-selected.
    start[0][0]+=1.9;
    start[1][0]+=1.95;
    start[2][0]+=1.4;
    const std::string grouped="WRAPAROUND ATOMS=1-14 AROUND=17,18 GROUP_BY_RESIDUE";
    const auto result=calculate(grouped,start,box);
    const auto explicitGroups=calculate(
                                "WRAPAROUND ATOMS=2,1,3 AROUND=17,18 GROUPBY=3\n"
                                "WRAPAROUND ATOMS=6,4,5,7 AROUND=17,18 GROUPBY=4\n"
                                "WRAPAROUND ATOMS=9,8,10,12,11,13 AROUND=17,18 GROUPBY=3\n"
                                "WRAPAROUND ATOMS=14 AROUND=17,18",start,box);
    same(result,explicitGroups,"mixed groups and full residue identities");
    const auto firstAnchors=calculate("WRAPAROUND ATOMS=1-3 AROUND=17,18 GROUPBY=3",start,box);
    require((result[1]-firstAnchors[1]).modulo()>1,"central anchor differs observably from legacy first anchor");
    for(unsigned i=14; i<18; ++i) {
      require((result[i]-start[i]).modulo()<1e-10,"unselected atoms unchanged");
    }
    same(calculate("WRAPAROUND ATOMS=1,3,1,4,7 AROUND=17 GROUP_BY_RESIDUE GROUPBY=1",start,box),
         calculate("WRAPAROUND ATOMS=1,3,4,7 AROUND=17 GROUPBY=2",start,box),
         "partial selections choose closest selected atom, stable duplicate removal");
    same(calculate("WRAPAROUND ATOMS=1,4,2,5,3,7 AROUND=17,18 GROUP_BY_RESIDUE",start,box),
         calculate("WRAPAROUND ATOMS=1,4,2,5,3,7 AROUND=17,18",start,box),
         "noncontiguous residue runs stay separate");
    same(calculate("WRAPAROUND ATOMS=1-14 AROUND=17,18,17,18,17 GROUP_BY_RESIDUE PAIR",start,box),
         calculate("WRAPAROUND ATOMS=2,1,3 AROUND=17 GROUPBY=3 PAIR\n"
                   "WRAPAROUND ATOMS=6,4,5,7 AROUND=18 GROUPBY=4 PAIR\n"
                   "WRAPAROUND ATOMS=9,8,10 AROUND=17 GROUPBY=3 PAIR\n"
                   "WRAPAROUND ATOMS=12,11,13 AROUND=18 GROUPBY=3 PAIR\n"
                   "WRAPAROUND ATOMS=14 AROUND=17 PAIR",start,box),"PAIR per residue run");
    same(calculate(grouped+" STRIDE=2",start,box,1),start,"STRIDE respected");
    same(calculate("WRAPAROUND ATOMS=1-14 AROUND=1-14 GROUP_BY_RESIDUE",start,box),
         calculate("WRAPAROUND ATOMS=2,1,3 AROUND=1-14 GROUPBY=3\n"
                   "WRAPAROUND ATOMS=6,4,5,7 AROUND=1-14 GROUPBY=4\n"
                   "WRAPAROUND ATOMS=9,8,10 AROUND=1-14 GROUPBY=3\n"
                   "WRAPAROUND ATOMS=12,11,13 AROUND=1-14 GROUPBY=3\n"
                   "WRAPAROUND ATOMS=14 AROUND=1-14",start,box),"overlapping reference snapshot and self anchors");
    same(calculate("c: COM ATOMS=17,18 NOPBC\nWRAPAROUND ATOMS=1-3 AROUND=c GROUP_BY_RESIDUE",start,box),
         calculate("c: COM ATOMS=17,18 NOPBC\nWRAPAROUND ATOMS=2,1,3 AROUND=c GROUPBY=3",start,box),"virtual COM reference");
    same(calculate("WRAPAROUND AROUND=17 GROUP_BY_RESIDUE",start,box),start,"empty selection");
  }
  out<<"mixed residues, central static anchors, partial selections and identities PASS\n";
  out<<"rectangular/skew cells, arbitrary images, duplicates and contiguous runs PASS\n";
  out<<"PAIR, virtual COM, overlap, unselected atoms, STRIDE and DUMPATOMS PASS\n";
  const std::string command="WRAPAROUND ATOMS=1-3 AROUND=17 GROUP_BY_RESIDUE";
  for(const std::string flags: {
        "NONE","","WHOLE"
      }) {
    rejects(flags,command,"requires a preceding MOLINFO");
  }
  rejects("WHOLE SMALL_RESIDUES",command+" GROUPBY=3","cannot be combined");
  rejects("WHOLE SMALL_RESIDUES","WRAPAROUND ATOMS=1-3 AROUND=17,18 GROUP_BY_RESIDUE PAIR","one AROUND entry");
  rejects("WHOLE SMALL_RESIDUES","WRAPAROUND ATOMS=15 AROUND=17 GROUP_BY_RESIDUE","atom 15 is missing");
  rejects("WHOLE SMALL_RESIDUES","c: COM ATOMS=1-3\nWRAPAROUND ATOMS=c AROUND=17 GROUP_BY_RESIDUE","is missing");
  rejects("WHOLE SMALL_RESIDUES","WRAPAROUND ATOMS=1-3 AROUND=17,18 GROUPBY=3 PAIR","#ATOMS = #AROUND");
  out<<"missing declarations/metadata, conflicting GROUPBY and PAIR counts PASS\n";
}
