#include "plumed/core/PlumedMain.h"
#include "plumed/core/GenericMolInfo.h"
#include "plumed/core/ActionSet.h"
#include "plumed/core/ActionWithValue.h"
#include "plumed/core/Value.h"
#include "plumed/tools/Tree.h"
#include "plumed/tools/Pbc.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <set>
#include <stdexcept>

using namespace PLMD;
using Link=std::pair<unsigned,unsigned>;
void require(bool condition,const char* message) {
  if(!condition) {
    throw std::runtime_error(message);
  }
}
const std::vector<Vector> reference= {
  {-0.2,0,0},{0,0,0},{0.1,0,0},
  {0.7,0.2,0},{0.8,0.2,0},{0.95,0.2,0},
  {1.6,0.1,0},{1.8,0.1,0},{2.1,0.1,0},
  {2.7,-0.1,0},{2.8,-0.1,0},{3,-0.1,0},
  {3.4,0.4,0},{3.4,0.4,0}
};
void writeReference(const char* name,bool material=false) {
  FILE* file=std::fopen(name,"w");
  require(file,"reference file");
  for(unsigned i=0; i<(material ? 12u : reference.size()); ++i) {
    if(!material && i==9) {
      std::fprintf(file,"TER\n");
    }
    const char chain=material ? 'M' : i<6 ? 'A' : i<12 ? 'B' : 'C';
    const char insertion=!material && i>=3 && i<6 ? 'A' : ' ';
    const int residue=material || i<12 ? 1 : i-10;
    const Vector x=material ? Vector(0.3*i,0,0) : reference[i];
    std::fprintf(file,"ATOM  %5u  C   ALA %c%4d%c   %8.3f%8.3f%8.3f  1.00  1.00           C\n",
                 i+1,chain,residue,insertion,10*x[0],10*x[1],10*x[2]);
  }
  std::fprintf(file,"END\n");
  std::fclose(file);
}
struct Context {
  FILE* log=std::tmpfile();
  std::unique_ptr<PlumedMain> p=std::make_unique<PlumedMain>();
  Context() {
    require(log,"log file");
    int precision=sizeof(double),n=16;
    double timestep=1;
    p->cmd("setRealPrecision",&precision);
    p->cmd("setNatoms",&n);
    p->cmd("setTimestep",&timestep);
    p->cmd("setLog",log);
    p->cmd("init");
  }
  ~Context() {
    p.reset();
    std::fclose(log);
  }
  GenericMolInfo* info(const std::string& flags="WHOLE SMALL_RESIDUES",
                       const std::string& file="reference.pdb") {
    const std::string input="MOLINFO STRUCTURE="+file+" PYTHON_BIN=no "+flags;
    p->cmd("readInputLine",input.c_str());
    return p->getActionSet().select<GenericMolInfo*>().back();
  }
};
std::vector<AtomNumber> atoms(std::initializer_list<unsigned> serials) {
  std::vector<AtomNumber> result;
  for(unsigned i:serials) {
    result.push_back(AtomNumber::serial(i));
  }
  return result;
}
std::set<Link> links(const Tree& tree,const std::vector<AtomNumber>& selection) {
  const auto& children=tree.getTreeIndexes();
  const auto& parents=tree.getRootIndexes();
  require(children.size()==selection.size(),"one traversal entry per requested atom");
  require(parents.size()+(selection.empty()?0:1)==children.size(),"tree edge count");
  std::set<Link> result;
  std::vector<bool> visited(selection.size(),false);
  if(!selection.empty()) {
    require(children[0]==0,"original first atom remains anchor");
    visited[0]=true;
  }
  for(unsigned j=0; j<parents.size(); ++j) {
    const unsigned p=parents[j],c=children[j+1];
    require(p<selection.size() && c<selection.size() && visited[p] && !visited[c],"parent-before-child traversal");
    visited[c]=true;
    unsigned a=selection[p].serial(),b=selection[c].serial();
    result.emplace(std::min(a,b),std::max(a,b));
  }
  return result;
}
void geometry(const Tree& tree,const std::vector<AtomNumber>& selection) {
  for(bool skew: {
        false,true
      }) {
    const Tensor box=skew ? Tensor(4.5,0,0,0.6,4.5,0,-0.3,0.2,4.5) : Tensor(4.5,0,0,0,4.5,0,0,0,4.5);
    Pbc pbc;
    pbc.setBox(box);
    std::vector<Vector> x;
    for(unsigned j=0; j<selection.size(); ++j) {
      const Vector image(static_cast<int>(j%5)-2,static_cast<int>(j%3)-1,static_cast<int>(j%7)-3);
      x.push_back(reference[selection[j].index()]+matmul(image,box));
    }
    const Vector shift=x[0]-reference[selection[0].index()];
    pbc.makeWhole(x,tree.getRootIndexes().size(),tree.getRootIndexes().data(),tree.getTreeIndexes().data()+1);
    for(unsigned j=0; j<x.size(); ++j) {
      require((x[j]-reference[selection[j].index()]-shift).modulo()<1e-11,"whole coordinates in rectangular/skew cell");
    }
  }
}
struct Result {
  double value;
  std::array<double,48> forces;
  std::array<double,9> virial;
};
Result calculate(bool small,bool global) {
  Context c;
  auto* info=c.info(small ? "WHOLE SMALL_RESIDUES" : "WHOLE");
  if(global) {
    c.p->cmd("readInputLine","WHOLEMOLECULES ENTITY0=1-12 ENTITY1=1-12");
  }
  c.p->cmd("readInputLine","g1: GYRATION_SCALAR ATOMS=1-12");
  c.p->cmd("readInputLine","g2: GYRATION_SCALAR ATOMS=1-12");
  c.p->cmd("readInputLine","RESTRAINT ARG=g1,g2 AT=0.8,0.9 KAPPA=1,2");
  std::vector<AtomNumber> selection;
  for(unsigned i=1; i<=12; ++i) {
    selection.push_back(AtomNumber::serial(i));
  }
  std::array<double,48> positions{};
  std::array<double,16> masses,charges{};
  masses.fill(1);
  for(unsigned i=0; i<14; ++i) {
    for(unsigned k=0; k<3; ++k) {
      positions[3*i+k]=reference[i][k];
    }
    positions[3*i]+=4.5*(static_cast<int>(i%5)-2);
  }
  std::array<double,9> box{4.5,0,0,0,4.5,0,0,0,4.5};
  Result result{};
  int step=0;
  c.p->cmd("setStep",&step);
  c.p->cmd("setBox",box.data());
  c.p->cmd("setPositions",positions.data());
  c.p->cmd("setMasses",masses.data());
  c.p->cmd("setCharges",charges.data());
  c.p->cmd("setForces",result.forces.data());
  c.p->cmd("setVirial",result.virial.data());
  c.p->cmd("calc");
  auto cached=info->getReconstructionTree(selection);
  require(cached.use_count()>= (global ? 5 : 3),"local actions and global entities share topology");
  result.value=c.p->getActionSet().selectWithLabel<ActionWithValue*>("g1")->copyOutput(0)->get();
  return result;
}
int main() {
  writeReference("reference.pdb");
  writeReference("material.pdb",true);
  std::ofstream out("output");
  Context c;
  auto* info=c.info();
  const auto selection=atoms({1,2,3,4,5,6,7,8,9,10,11,12});
  auto full=info->getReconstructionTree(selection);
  require(links(*full,selection)==std::set<Link> {{1,2},{2,3},{4,5},{5,6},{7,8},{8,9},{10,11},{11,12},{2,5},{5,8},{8,11}},"residue star links and representative MST");
  for(const auto& subset: {
        atoms({1,3,4,6,7,9,10,12})
        ,atoms({12,1,6,7,4,3,10,9})
      }) {
    auto tree=info->getReconstructionTree(subset);
    require(links(*tree,subset)==std::set<Link> {{1,3},{4,6},{7,9},{10,12},{3,4},{4,7},{7,10}},"representative chosen from selected atoms using full-residue center");
    geometry(*tree,subset);
  }
  const auto coincident=atoms({13,14});
  require(links(*info->getReconstructionTree(coincident),coincident)==std::set<Link> {{13,14}},"coincident representatives");
  const auto duplicates=atoms({1,1,2});
  links(*info->getReconstructionTree(duplicates),duplicates);
  info->getReconstructionTree({});
  info->getReconstructionTree(atoms({1}));
  geometry(*full,selection);
  out<<"residue identity, selected representatives, anchor and traversal PASS\n";

  require(info->getReconstructionTree(selection)==full,"cache hit");
  const auto original=full->getTreeIndexes();
  auto reordered=selection;
  std::reverse(reordered.begin(),reordered.end());
  require(info->getReconstructionTree(reordered)!=full,"ordered selection cache key");
  require(full->getTreeIndexes()==original,"cached tree is immutable");
  const auto temporary=atoms({1,4});
  std::weak_ptr<const Tree> released=info->getReconstructionTree(temporary);
  require(released.expired(),"cache does not own unused trees");
  for(unsigned i=0; i<50; ++i) {
    info->getReconstructionTree(std::vector<AtomNumber>(i+1,AtomNumber::serial(1)));
  }
  require(info->getReconstructionTree(selection)==full,"pruning preserves live entries");
  auto* other=c.info();
  require(other->getReconstructionTree(selection)!=full,"different MOLINFO references have separate caches");
  out<<"shared immutable trees, request/reference isolation and expiry PASS\n";

  auto* material=c.info("WHOLE","material.pdb");
  require(!material->hasSmallResidues(),"residue grouping is opt-in");
  std::set<Link> chain;
  for(unsigned i=1; i<12; ++i) {
    chain.emplace(i,i+1);
  }
  require(links(*material->getReconstructionTree(selection),selection)==chain,"single-residue material retains full EMST");
  const auto missing=atoms({1,15,16,2,4});
  const auto missingLinks=links(*info->getReconstructionTree(missing),missing);
  require(missingLinks.count({1,15}) && missingLinks.count({15,16}),"missing-reference previous-atom links");
  out<<"default single-residue material and missing-reference links PASS\n";

  const auto legacy=calculate(false,false);
  for(bool global: {
        false,true
      }) {
    const auto result=calculate(true,global);
    require(std::fabs(result.value-legacy.value)<1e-12,"local/global reconstructed CV");
    for(unsigned i=0; i<result.forces.size(); ++i) {
      require(std::isfinite(result.forces[i]) && std::fabs(result.forces[i]-legacy.forces[i])<1e-11,"local/global forces");
    }
    for(unsigned i=0; i<result.virial.size(); ++i) {
      require(std::isfinite(result.virial[i]) && std::fabs(result.virial[i]-legacy.virial[i])<1e-11,"local/global virial");
    }
  }
  out<<"periodic reconstruction, local/global actions, forces and virial PASS\n";
  bool rejected=false;
  try {
    Context invalid;
    invalid.info("SMALL_RESIDUES");
  } catch(const std::exception& e) {
    rejected=std::string(e.what()).find("SMALL_RESIDUES requires WHOLE")!=std::string::npos;
  }
  require(rejected,"SMALL_RESIDUES without WHOLE rejected");
  rejected=false;
  try {
    info->getReconstructionTree(atoms({15,1}));
  } catch(const std::exception&) {
    rejected=true;
  }
  require(rejected,"missing first reference atom rejected");
  out<<"invalid SMALL_RESIDUES input PASS\n";
}
