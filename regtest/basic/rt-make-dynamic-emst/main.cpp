#include "plumed/tools/DynamicTree.h"
#include "plumed/tools/Pbc.h"
#include "plumed/wrapper/Plumed.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>
#include <tuple>

using namespace PLMD;
using Pair=std::pair<unsigned,unsigned>;
void require(bool ok,const char* message) {
  if(!ok) {
    throw std::runtime_error(message);
  }
}

// Independent complete-graph Kruskal oracle. No spatial pruning, displacement
// certificate, or production builder code is used to select its edges.
std::set<Pair> oracle(const std::vector<Vector>& x,const Pbc& pbc) {
  std::vector<std::tuple<double,unsigned,unsigned>> edges;
  for(unsigned i=0; i<x.size(); ++i)
    for(unsigned j=i+1; j<x.size(); ++j) {
      edges.emplace_back(pbc.distance(x[i],x[j]).modulo2(),i,j);
    }
  std::sort(edges.begin(),edges.end());
  std::vector<unsigned> component(x.size());
  std::iota(component.begin(),component.end(),0);
  std::set<Pair> result;
  for(const auto& e:edges) {
    const unsigned i=std::get<1>(e),j=std::get<2>(e);
    if(component[i]==component[j]) {
      continue;
    }
    const unsigned old=component[j],replacement=component[i];
    for(auto& c:component) {
      if(c==old) {
        c=replacement;
      }
    }
    result.emplace(i,j);
    if(result.size()+1==x.size()) {
      break;
    }
  }
  return result;
}

std::set<Pair> actual(const DynamicTree& tree,unsigned n) {
  const auto& parents=tree.getParents();
  const auto& children=tree.getChildren();
  require(parents.size()==(n ? n-1 : 0) && children.size()==parents.size(),"edge count");
  std::vector<bool> seen(n,false);
  if(n) {
    seen[0]=true;
  }
  std::set<Pair> result;
  for(unsigned j=0; j<parents.size(); ++j) {
    const unsigned p=parents[j],c=children[j];
    require(p<n && c<n && seen[p] && !seen[c],"parent-before-child traversal");
    seen[c]=true;
    result.emplace(std::min(p,c),std::max(p,c));
  }
  return result;
}

void check(DynamicTree& tree,const std::vector<Vector>& x,const Pbc& pbc,double buffer=0.4) {
  tree.update(x,pbc,buffer);
  require(actual(tree,x.size())==oracle(x,pbc),"MST differs from complete graph");
}

struct Simulation {
  Plumed p;
  std::vector<Vector> forces;
  std::vector<double> masses;
  Tensor virial;
  int step=0;
  Simulation(unsigned n,const std::string& lines):forces(n),masses(n,1) {
    double dt=0.01;
    p.cmd("setNatoms",&n);
    p.cmd("setTimestep",&dt);
    p.cmd("setLogFile","simulation.log");
    p.cmd("init");
    std::size_t begin=0;
    while(begin<lines.size()) {
      const auto end=lines.find('\n',begin);
      p.cmd("readInputLine",lines.substr(begin,end-begin).c_str());
      if(end==std::string::npos) {
        break;
      }
      begin=end+1;
    }
  }
  double sample(std::vector<Vector> x,Tensor box) {
    std::fill(forces.begin(),forces.end(),Vector());
    virial=Tensor();
    p.cmd("setStep",&step);
    ++step;
    p.cmd("setPositions",&x[0][0]);
    p.cmd("setMasses",masses.data());
    p.cmd("setForces",&forces[0][0]);
    p.cmd("setBox",&box(0,0));
    p.cmd("setVirial",&virial(0,0));
    p.cmd("calc");
    double bias=0;
    p.cmd("getBias",&bias);
    return bias;
  }
};

int main() {
  std::ofstream out("output");
  Pbc pbc;
  const Tensor rect(10,0,0,0,10,0,0,0,10);
  const Tensor skew(10,0,0,3,9,0,-2,1,8);
  std::mt19937 rng(1191);
  std::uniform_real_distribution<double> uniform(0,1);
  for(const auto& box: {
        rect,skew
      }) {
    pbc.setBox(box);
    std::vector<Vector> x(400);
    for(auto& v:x) {
      v=Vector(uniform(rng),uniform(rng),uniform(rng));
      v=pbc.scaledToReal(v);
    }
    DynamicTree tree;
    check(tree,x,pbc);
    require(!tree.usesDenseTree(),"random cloud should use candidate graph");
    const auto built=tree.getRebuildCount();
    const auto candidates=tree.getCandidateCount();
    require(candidates<x.size()*(x.size()-1)/2,"graph should be sparse");
    for(unsigned frame=0; frame<8; ++frame) {
      for(auto& v:x) {
        v[0]+=0.0001*(uniform(rng)-0.5);
      }
      check(tree,x,pbc);
      require(tree.getRebuildCount()==built,"small displacements should reuse graph");
    }
    // Independent image changes must neither corrupt the metric nor force a
    // rebuild when the actual periodic displacements remain negligible.
    for(unsigned i=0; i<x.size(); ++i) {
      x[i]+=box.getRow(i%3)*(i%2 ? 2 : -2);
    }
    check(tree,x,pbc);
    require(tree.getRebuildCount()==built,"periodic image changes should reuse graph");
    x[0]+=Vector(3.1,2.7,1.9);
    check(tree,x,pbc);
    require(tree.getRebuildCount()>built,"large displacement must invalidate certificate");
    const auto moved=tree.getRebuildCount();
    Tensor changed=box;
    changed(0,0)+=3.0;
    pbc.setBox(changed);
    check(tree,x,pbc);
    require(tree.getRebuildCount()==moved+1,"cell change must rebuild");
    x.pop_back();
    check(tree,x,pbc);
    require(tree.getRebuildCount()==moved+2,"selection size change must rebuild");
  }
  out<<"sparse exact MST, displacement reuse, wrapping, rebuild and cell changes PASS\n";

  // Affine cell deformation and common translation do not require geometric
  // rebuilding while the conservative omitted-edge certificate remains valid.
  pbc.setBox(skew);
  std::vector<Vector> deforming(400);
  for(auto& v:deforming) {
    v=pbc.scaledToReal(Vector(uniform(rng),uniform(rng),uniform(rng)));
  }
  DynamicTree deformingTree;
  check(deformingTree,deforming,pbc);
  const auto deformationBuilds=deformingTree.getRebuildCount();
  const Tensor deformation(1.0001,0.00005,0,0,0.99995,0.00003,0,0,1.00002);
  for(unsigned frame=0; frame<8; ++frame) {
    pbc.setBox(matmul(pbc.getBox(),deformation));
    for(auto& v:deforming) {
      v=matmul(v,deformation)+Vector(0.1,-0.15,0.05);
      v[0]+=0.0001*(uniform(rng)-0.5);
    }
    check(deformingTree,deforming,pbc);
    require(deformingTree.getRebuildCount()==deformationBuilds,"small affine cell changes should reuse graph");
  }
  // Force large changes of edge order as well as graph validity; bounded sorting
  // must still return exactly the complete-graph result.
  std::shuffle(deforming.begin(),deforming.end(),rng);
  check(deformingTree,deforming,pbc);
  require(deformingTree.getRebuildCount()>deformationBuilds,"nonaffine motion must rebuild");
  for(double length: {
        0.1,1.0,10.0
      }) {
    pbc.setBox(Tensor(length,0,0,0,length,0,0,0,length));
    std::vector<Vector> wrapped(40);
    for(auto& v:wrapped) {
      v=Vector(length*(5*uniform(rng)-2),length*(5*uniform(rng)-2),length*(5*uniform(rng)-2));
    }
    DynamicTree cells;
    check(cells,wrapped,pbc,0.2);
  }
  out<<"cell-list degeneracies, affine deformation and translation reuse PASS\n";

  pbc.setBox(rect);
  std::vector<Vector> tied{{0,0,0},{1,0,0},{0,1,0},{1,1,0},{0,0,0}};
  DynamicTree tree;
  check(tree,tied,pbc,2.0);
  const auto old=actual(tree,tied.size());
  const auto built=tree.getRebuildCount();
  tied[3]=Vector(0.1,0.1,0);
  check(tree,tied,pbc);
  require(actual(tree,tied.size())!=old && tree.getRebuildCount()==built,"MST must change within reused graph");
  check(tree, {},pbc);
  check(tree, {Vector()},pbc);
  check(tree,tied,pbc);
  std::vector<Vector> coincident(600,Vector());
  check(tree,coincident,pbc);
  require(tree.usesDenseTree(),"memory cap must select dense fallback");
  coincident[1]=Vector(0.1,0,0);
  check(tree,coincident,pbc);
  for(unsigned i=0; i<coincident.size(); ++i) {
    coincident[i]=Vector(i%10,(i/10)%10,i/100);
  }
  check(tree,coincident,pbc);
  require(!tree.usesDenseTree(),"sparse geometry should recover after dense fallback");
  Pbc nonperiodic;
  check(tree,tied,nonperiodic);
  std::vector<Vector> lineCloud(600);
  for(unsigned i=0; i<lineCloud.size(); ++i) {
    lineCloud[i]=Vector(i,0,0);
  }
  check(tree,lineCloud,nonperiodic);
  require(!tree.usesDenseTree(),"oversized initial radius should retry at actual bottleneck");
  bool rejected=false;
  try {
    tree.update(tied,pbc,0);
  } catch(const std::exception&) {
    rejected=true;
  }
  require(rejected,"zero buffer must be rejected");
  tied[0][0]=std::numeric_limits<double>::quiet_NaN();
  rejected=false;
  try {
    tree.update(tied,pbc,0.4);
  } catch(const std::exception&) {
    rejected=true;
  }
  require(rejected,"nonfinite coordinates must be rejected");
  out<<"ties, duplicates, topology changes, dense fallback and invalid inputs PASS\n";

  // Deliberately unordered wrapped line, no reference. Its MST reconstructs
  // x=(1,7,3,5), with Rg=sqrt(5). Sequential reconstruction gives a different Rg.
  std::vector<Vector> line{{1,0,0},{-3,0,0},{3,0,0},{-5,0,0}};
  Simulation local(4,"g: GYRATION ATOMS=1-4 DYNAMIC_EMST\nw: BIASVALUE ARG=g");
  require(std::fabs(local.sample(line,rect)-std::sqrt(5.0))<1e-12,"local reconstruction");
  const auto analytic=local.forces;
  const auto analyticVirial=local.virial;
  Simulation numeric(4,"g: GYRATION ATOMS=1-4 DYNAMIC_EMST NUMERICAL_DERIVATIVES\nw: BIASVALUE ARG=g");
  require(std::fabs(numeric.sample(line,rect)-std::sqrt(5.0))<1e-12,"numerical reconstruction");
  for(unsigned i=0; i<line.size(); ++i)
    for(unsigned k=0; k<3; ++k) {
      require(std::fabs(analytic[i][k]-numeric.forces[i][k])<1e-5,"numerical forces");
    }
  for(unsigned j=0; j<3; ++j)
    for(unsigned k=0; k<3; ++k) {
      require(std::fabs(analyticVirial(j,k)-numeric.virial(j,k))<1e-5,"numerical virial");
    }
  Simulation global(4,"WHOLEMOLECULES ENTITY0=1-4 DYNAMIC_EMST\ng: GYRATION ATOMS=1-4 NOPBC\nw: BIASVALUE ARG=g");
  require(std::fabs(global.sample(line,rect)-std::sqrt(5.0))<1e-12,"global reconstruction");
  Simulation entities(4,"WHOLEMOLECULES ENTITY0=1,3 ENTITY1=2,4 DYNAMIC_EMST\ng: GYRATION ATOMS=1,3 NOPBC\nw: BIASVALUE ARG=g");
  require(std::fabs(entities.sample(line,rect)-1.0)<1e-12,"independent entities");
  {
    Simulation dump(4,"DUMPATOMS ATOMS=1-4 FILE=whole.xyz STRIDE=2 DYNAMIC_EMST\nDUMPATOMS ATOMS=1-4 FILE=raw.xyz STRIDE=2");
    dump.sample(line,rect);
    for(auto& v:line) {
      v[0]+=10;
    }
    dump.sample(line,rect);
    dump.sample(line,rect);
  }
  std::ifstream whole("whole.xyz"),raw("raw.xyz");
  const double reconstructed[4]= {1,7,3,5},original[4]= {1,-3,3,-5};
  for(unsigned frame=0; frame<2; ++frame) {
    std::string header;
    for(unsigned j=0; j<2; ++j) {
      std::getline(whole,header);
      std::getline(raw,header);
    }
    for(unsigned i=0; i<4; ++i) {
      std::string name;
      double a,b,c;
      whole>>name>>a>>b>>c;
      require(whole.good() && std::fabs(a-reconstructed[i]-10*frame)<1e-6,"dump reconstruction and stride");
      raw>>name>>a>>b>>c;
      require(raw.good() && std::fabs(a-original[i]-10*frame)<1e-6,"local reconstruction must preserve shared coordinates");
    }
    std::string rest;
    std::getline(whole,rest);
    std::getline(raw,rest);
  }
  out<<"local/global actions, dump stride, numerical forces and virial PASS\n";
}
