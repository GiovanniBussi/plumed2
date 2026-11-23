/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2026 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   This file is part of plumed, version 2.
   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public License
   along with plumed. If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#include "DynamicTree.h"
#include "Pbc.h"
#include "Exception.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <tuple>

namespace PLMD {
namespace {
// All pruning/certification inequalities use lengths, not squared lengths.
double roundoff(const std::vector<Vector>& positions,const Tensor& box) {
  double scale=1;
  for(const auto& x:positions)
    for(unsigned k=0; k<3; ++k) {
      scale=std::max(scale,std::fabs(x[k]));
    }
  for(unsigned j=0; j<3; ++j)
    for(unsigned k=0; k<3; ++k) {
      scale=std::max(scale,std::fabs(box(j,k)));
    }
  return 1024*std::numeric_limits<double>::epsilon()*scale;
}
struct Components {
  std::vector<unsigned> parent,size;
  explicit Components(unsigned n):parent(n),size(n,1) {
    std::iota(parent.begin(),parent.end(),0);
  }
  unsigned root(unsigned i) {
    while(parent[i]!=i) {
      parent[i]=parent[parent[i]];
      i=parent[i];
    }
    return i;
  }
  bool join(unsigned i,unsigned j) {
    i=root(i);
    j=root(j);
    if(i==j) {
      return false;
    }
    if(size[i]<size[j]) {
      std::swap(i,j);
    }
    parent[j]=i;
    size[i]+=size[j];
    return true;
  }
};
}

bool DynamicTree::Edge::operator<(const Edge& other) const {
  return std::tie(weight,first,second)<std::tie(other.weight,other.first,other.second);
}

double DynamicTree::denseTree(const std::vector<Vector>& positions,const Pbc& pbc) {
  const unsigned n=positions.size();
  parents.clear();
  children.clear();
  if(n<2) {
    return 0;
  }
  std::vector<unsigned char> visited(n,0);
  std::vector<Edge> best(n,Edge{0,0,std::numeric_limits<double>::infinity()});
  unsigned selected=0;
  double longest=0;
  for(unsigned step=0; step<n; ++step) {
    visited[selected]=1;
    unsigned next=n;
    for(unsigned j=0; j<n; ++j) {
      if(visited[j]) {
        continue;
      }
      const unsigned first=std::min(selected,j),second=std::max(selected,j);
      const Edge edge{first,second,pbc.distance(positions[first],positions[second]).modulo2()};
      plumed_assert(std::isfinite(edge.weight)) << "DYNAMIC_EMST distance is not representable";
      if(edge<best[j]) {
        best[j]=edge;
      }
      if(next==n || best[j]<best[next]) {
        next=j;
      }
    }
    if(next<n) {
      const auto& e=best[next];
      parents.push_back(e.first==next ? e.second : e.first);
      children.push_back(next);
      longest=std::max(longest,e.weight);
      selected=next;
    }
  }
  return std::sqrt(longest);
}

void DynamicTree::orient(unsigned n,const std::vector<Edge>& selected) {
  std::vector<std::size_t> offsets(n+1,0);
  for(const auto& e:selected) {
    ++offsets[e.first+1];
    ++offsets[e.second+1];
  }
  std::partial_sum(offsets.begin(),offsets.end(),offsets.begin());
  auto cursor=offsets;
  std::vector<unsigned> adjacent(2*selected.size());
  for(const auto& e:selected) {
    adjacent[cursor[e.first]++]=e.second;
    adjacent[cursor[e.second]++]=e.first;
  }
  parents.clear();
  children.clear();
  std::vector<unsigned> queue(1,0);
  std::vector<unsigned char> seen(n,0);
  seen[0]=1;
  for(std::size_t j=0; j<queue.size(); ++j) {
    const unsigned p=queue[j];
    for(std::size_t k=offsets[p]; k<offsets[p+1]; ++k) {
      const unsigned c=adjacent[k];
      if(seen[c]) {
        continue;
      }
      seen[c]=1;
      parents.push_back(p);
      children.push_back(c);
      queue.push_back(c);
    }
  }
}

bool DynamicTree::sparseTree(const std::vector<Vector>& positions,const Pbc& pbc,double& longest) {
  for(auto& e:edges) {
    e.weight=pbc.distance(positions[e.first],positions[e.second]).modulo2();
    plumed_assert(std::isfinite(e.weight)) << "DYNAMIC_EMST distance is not representable";
  }
  std::sort(edges.begin(),edges.end());
  Components components(positions.size());
  std::vector<Edge> selected;
  longest=0;
  for(const auto& e:edges) {
    if(components.join(e.first,e.second)) {
      selected.push_back(e);
      longest=e.weight;
      if(selected.size()+1==positions.size()) {
        break;
      }
    }
  }
  if(selected.size()+1!=positions.size()) {
    return false;
  }
  longest=std::sqrt(longest);
  orient(positions.size(),selected);
  return true;
}

unsigned DynamicTree::buildNodes(unsigned begin,unsigned end,std::vector<unsigned>& order,
                                 const std::vector<Vector>& positions,const std::vector<Vector>& split,
                                 const Pbc& pbc,std::vector<Node>& nodes) {
  const unsigned index=nodes.size();
  Node node{begin,end,order[begin]};
  Vector lower=split[order[begin]],upper=lower;
  for(unsigned j=begin; j<end; ++j) {
    node.radius=std::max(node.radius,pbc.distance(positions[node.center],positions[order[j]]).modulo());
    for(unsigned k=0; k<3; ++k) {
      lower[k]=std::min(lower[k],split[order[j]][k]);
      upper[k]=std::max(upper[k],split[order[j]][k]);
    }
  }
  nodes.push_back(node);
  if(end-begin>16) {
    unsigned axis=0;
    for(unsigned k=1; k<3; ++k) {
      if(upper[k]-lower[k]>upper[axis]-lower[axis]) {
        axis=k;
      }
    }
    const unsigned middle=begin+(end-begin)/2;
    std::nth_element(order.begin()+begin,order.begin()+middle,order.begin()+end,
    [&](unsigned a,unsigned b) {
      return split[a][axis]!=split[b][axis] ? split[a][axis]<split[b][axis] : a<b;
    });
    node.left=buildNodes(begin,middle,order,positions,split,pbc,nodes);
    node.right=buildNodes(middle,end,order,positions,split,pbc,nodes);
    nodes[index]=node;
  }
  return index;
}

bool DynamicTree::findEdges(unsigned index,unsigned atom,const std::vector<unsigned>& order,
                            const std::vector<Node>& nodes,const std::vector<Vector>& positions,
                            const Pbc& pbc,double padding,std::size_t limit) {
  const auto& node=nodes[index];
  // Periodic distance is a metric: d(query,center)-node.radius is a lower
  // bound even across a wrapping boundary or in a triclinic cell. Cartesian
  // coordinates above determine partitioning only, never periodic pruning.
  if(pbc.distance(positions[atom],positions[node.center]).modulo()>radius+node.radius+padding) {
    return true;
  }
  if(node.left) {
    return findEdges(node.left,atom,order,nodes,positions,pbc,padding,limit)
           && findEdges(node.right,atom,order,nodes,positions,pbc,padding,limit);
  }
  for(unsigned j=node.begin; j<node.end; ++j) {
    const unsigned other=order[j];
    if(other<=atom) {
      continue;
    }
    const double weight=pbc.distance(positions[atom],positions[other]).modulo2();
    plumed_assert(std::isfinite(weight)) << "DYNAMIC_EMST distance is not representable";
    if(std::sqrt(weight)<=radius+padding) {
      if(edges.size()==limit) {
        return false;
      }
      edges.push_back(Edge{atom,other,weight});
    }
  }
  return true;
}

void DynamicTree::rebuild(const std::vector<Vector>& positions,const Pbc& pbc,double buffer) {
  ++rebuilds;
  reference=positions;
  box=pbc.getBox();
  initialized=true;
  dense=false;
  const unsigned n=positions.size();
  std::vector<unsigned> order(n);
  std::iota(order.begin(),order.end(),0);
  std::vector<Vector> split(n);
  double extent=0;
  for(unsigned j=0; j<n; ++j) {
    split[j]=pbc.distance(positions[0],positions[j]);
    extent=std::max(extent,split[j].modulo());
  }
  std::vector<Node> nodes;
  nodes.reserve(2*n);
  buildNodes(0,n,order,positions,split,pbc,nodes);
  const double padding=roundoff(positions,box);
  radius=std::max(buffer,extent/std::cbrt(static_cast<double>(n)));
  // Bound memory for highly clustered groups or widely separated components.
  // Dense Prim still needs only O(N) storage and computes each pair once.
  const std::size_t limit=std::max<std::size_t>(4096,32*static_cast<std::size_t>(n));
  for(;;) {
    edges.clear();
    bool bounded=true;
    for(unsigned j=0; j<n && bounded; ++j) {
      bounded=findEdges(0,j,order,nodes,positions,pbc,padding,limit);
    }
    if(!bounded) {
      edges.clear();
      dense=true;
      denseTree(positions,pbc);
      return;
    }
    double longest=0;
    if(sparseTree(positions,pbc,longest)) {
      // Add a full buffer above the connecting bottleneck, even if the initial
      // radius happened to connect with almost no margin.
      if(radius!=longest+buffer) {
        radius=longest+buffer;
        continue;
      }
      complete=edges.size()==static_cast<std::size_t>(n)*(n-1)/2;
      return;
    }
    radius*=2;
    plumed_assert(std::isfinite(radius)) << "DYNAMIC_EMST search radius is not representable";
  }
}

void DynamicTree::update(const std::vector<Vector>& positions,const Pbc& pbc,double buffer) {
  plumed_assert(std::isfinite(buffer) && buffer>0) << "EMST_BUFFER must be finite and positive";
  plumed_assert(positions.size()<=std::numeric_limits<unsigned>::max());
  for(const auto& x:positions)
    for(unsigned k=0; k<3; ++k) {
      plumed_assert(std::isfinite(x[k])) << "DYNAMIC_EMST requires finite coordinates";
    }
  if(positions.size()<2) {
    parents.clear();
    children.clear();
    edges.clear();
    reference.clear();
    initialized=false;
    return;
  }
  bool sameBox=initialized;
  for(unsigned j=0; j<3; ++j)
    for(unsigned k=0; k<3; ++k) {
      sameBox=sameBox && box(j,k)==pbc.getBox()(j,k);
    }
  if(!sameBox || positions.size()!=reference.size()) {
    rebuild(positions,pbc,buffer);
    return;
  }
  if(dense) {
    denseTree(positions,pbc);
    return;
  }
  double displacement=0;
  for(unsigned j=0; j<positions.size(); ++j) {
    displacement=std::max(displacement,pbc.distance(reference[j],positions[j]).modulo());
  }
  double longest=0;
  const bool connected=sparseTree(positions,pbc,longest);
  // Omitted distances were >radius at build and can fall by at most 2*d.
  // If the current tree's bottleneck is smaller, no omitted edge improves it.
  const double padding=roundoff(positions,box)+roundoff(reference,box);
  if(!connected || (!complete && !(longest+2*displacement+padding<radius))) {
    rebuild(positions,pbc,buffer);
  }
}
}
