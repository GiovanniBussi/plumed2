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
  std::vector<unsigned>& parent;
  std::vector<unsigned>& size;
  Components(unsigned n,std::vector<unsigned>& p,std::vector<unsigned>& s):parent(p),size(s) {
    parent.resize(n);
    size.assign(n,1);
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
  parentOf.assign(n,n);
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
      parentOf[next]=parents.back();
      longest=std::max(longest,e.weight);
      selected=next;
    }
  }
  return std::sqrt(longest);
}

void DynamicTree::orient(unsigned n,const std::vector<Edge>& selected) {
  // A rooted tree has a unique parent for each atom. If its undirected edges
  // did not change, the previous parent-before-child ordering remains valid.
  if(parentOf.size()==n && parents.size()+1==n &&
  std::all_of(selected.begin(),selected.end(),[&](const Edge& e) {
  return parentOf[e.first]
           ==e.second || parentOf[e.second]==e.first;
  })) {
    return;
  }
  parentOf.assign(n,n);
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
      parentOf[c]=p;
      queue.push_back(c);
    }
  }
}

void DynamicTree::sortEdges() {
  if(edgesSorted) {
    // Consecutive frames often have few inversions. Bound insertion work so
    // arbitrary motion cannot turn sorting into a quadratic operation. The
    // partially repaired vector still contains every edge when falling back.
    std::size_t work=0;
    const std::size_t budget=2*edges.size();
    for(std::size_t i=1; i<edges.size(); ++i) {
      if(!(edges[i]<edges[i-1])) {
        continue;
      }
      const Edge edge=edges[i];
      std::size_t j=i;
      while(j>0 && edge<edges[j-1]) {
        edges[j]=edges[j-1];
        --j;
        if(++work>budget) {
          edges[j]=edge;
          std::sort(edges.begin(),edges.end());
          return;
        }
      }
      edges[j]=edge;
    }
  } else {
    std::sort(edges.begin(),edges.end());
  }
  edgesSorted=true;
}

bool DynamicTree::sparseTree(const std::vector<Vector>& positions,const Pbc& pbc,double& longest,bool refresh) {
  if(refresh) {
    for(auto& e:edges) {
      e.weight=pbc.distance(positions[e.first],positions[e.second]).modulo2();
      plumed_assert(std::isfinite(e.weight)) << "DYNAMIC_EMST distance is not representable";
    }
  }
  sortEdges();
  Components components(positions.size(),componentParent,componentSize);
  selectedEdges.clear();
  longest=0;
  for(const auto& e:edges) {
    if(components.join(e.first,e.second)) {
      selectedEdges.push_back(e);
      longest=e.weight;
      if(selectedEdges.size()+1==positions.size()) {
        break;
      }
    }
  }
  if(selectedEdges.size()+1!=positions.size()) {
    return false;
  }
  longest=std::sqrt(longest);
  orient(positions.size(),selectedEdges);
  return true;
}

unsigned DynamicTree::buildNodes(unsigned begin,unsigned end,std::vector<unsigned>& order,
                                 const std::vector<Vector>& positions,const std::vector<Vector>& split,
                                 const Pbc& pbc,std::vector<Node>& nodes) {
  const unsigned index=nodes.size();
  Node node{begin,end,order[begin]};
  Vector lower=split[order[begin]],upper=lower;
  for(unsigned j=begin; j<end; ++j) {
    for(unsigned k=0; k<3; ++k) {
      lower[k]=std::min(lower[k],split[order[j]][k]);
      upper[k]=std::max(upper[k],split[order[j]][k]);
    }
  }
  const Vector middlePoint=0.5*(lower+upper);
  double best=std::numeric_limits<double>::infinity();
  for(unsigned j=begin; j<end; ++j) {
    const double d=(split[order[j]]-middlePoint).modulo2();
    if(d<best) {
      node.center=order[j];
      best=d;
    }
  }
  for(unsigned j=begin; j<end; ++j) {
    node.radius=std::max(node.radius,pbc.distance(positions[node.center],positions[order[j]]).modulo());
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
                            const Pbc& pbc,double padding,std::size_t limit,double previousSquared) {
  const auto& node=nodes[index];
  // Periodic distance is a metric: d(query,center)-node.radius is a lower
  // bound even across a wrapping boundary or in a triclinic cell. Cartesian
  // coordinates above determine partitioning only, never periodic pruning.
  if(pbc.distance(positions[atom],positions[node.center]).modulo()>radius+node.radius+padding) {
    return true;
  }
  if(node.left) {
    return findEdges(node.left,atom,order,nodes,positions,pbc,padding,limit,previousSquared)
           && findEdges(node.right,atom,order,nodes,positions,pbc,padding,limit,previousSquared);
  }
  for(unsigned j=node.begin; j<node.end; ++j) {
    const unsigned other=order[j];
    if(other<=atom) {
      continue;
    }
    const double weight=pbc.distance(positions[atom],positions[other]).modulo2();
    plumed_assert(std::isfinite(weight)) << "DYNAMIC_EMST distance is not representable";
    if(weight>previousSquared && weight<=(radius+padding)*(radius+padding)) {
      if(edges.size()==limit) {
        return false;
      }
      edges.push_back(Edge{atom,other,weight});
    }
  }
  return true;
}

bool DynamicTree::findCellEdges(const std::vector<Vector>& positions,const Pbc& pbc,
                                double padding,std::size_t limit,double previousSquared) {
  const unsigned n=positions.size();
  const double reach=radius+padding;
  const double cutoffSquared=reach*reach;
  unsigned count[3];
  double length[3];
  const unsigned maximum=std::max(1u,static_cast<unsigned>(2*std::cbrt(static_cast<double>(n))));
  std::size_t total=1;
  for(unsigned k=0; k<3; ++k) {
    length[k]=std::fabs(pbc.getBox()(k,k));
    count[k]=static_cast<unsigned>(std::max(1.0,std::min(static_cast<double>(maximum),std::floor(length[k]/reach))));
    total*=count[k];
  }
  std::vector<unsigned> head(total,n),next(n,n);
  auto locate=[&](const Vector& x,unsigned (&cell)[3]) {
    for(unsigned k=0; k<3; ++k) {
      const double scaled=x[k]/length[k];
      const double wrapped=scaled-std::floor(scaled);
      cell[k]=std::min(count[k]-1,static_cast<unsigned>(wrapped*count[k]));
    }
  };
  auto index=[&](unsigned a,unsigned b,unsigned c) {
    return (static_cast<std::size_t>(a)*count[1]+b)*count[2]+c;
  };
  for(unsigned i=0; i<n; ++i) {
    unsigned cell[3];
    locate(positions[i],cell);
    const auto bin=index(cell[0],cell[1],cell[2]);
    next[i]=head[bin];
    head[bin]=i;
  }
  for(unsigned i=0; i<n; ++i) {
    unsigned cell[3],neighbor[3][3],size[3];
    locate(positions[i],cell);
    for(unsigned k=0; k<3; ++k) {
      size[k]=std::min(3u,count[k]);
      // Enumerate each cell only once when there are just one or two cells.
      for(unsigned j=0; j<size[k]; ++j) {
        neighbor[k][j]=(cell[k]+count[k]+j-1)%count[k];
      }
    }
    for(unsigned a=0; a<size[0]; ++a)
      for(unsigned b=0; b<size[1]; ++b)
        for(unsigned c=0; c<size[2]; ++c) {
          for(unsigned j=head[index(neighbor[0][a],neighbor[1][b],neighbor[2][c])]; j<n; j=next[j]) {
            if(j<=i) {
              continue;
            }
            const double weight=pbc.distance(positions[i],positions[j]).modulo2();
            plumed_assert(std::isfinite(weight)) << "DYNAMIC_EMST distance is not representable";
            if(weight>previousSquared && weight<=cutoffSquared) {
              if(edges.size()==limit) {
                return false;
              }
              edges.push_back(Edge{i,j,weight});
            }
          }
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
  edgesSorted=false;
  edges.clear();
  const unsigned n=positions.size();
  std::vector<unsigned> order(n);
  std::iota(order.begin(),order.end(),0);
  std::vector<Vector> split(n);
  double extent=0;
  bool cells=pbc.isOrthorombic();
  for(unsigned j=0; j<n; ++j) {
    split[j]=pbc.distance(positions[0],positions[j]);
    extent=std::max(extent,split[j].modulo());
    for(unsigned k=0; k<3; ++k) {
      // Very large image labels lose precision when mapped into cells. Retain
      // the metric-tree fallback rather than relying on an inaccurate bin.
      cells=cells && std::isfinite(pbc.getBox()(k,k)) && pbc.getBox()(k,k)!=0
            && std::fabs(positions[j][k]/pbc.getBox()(k,k))<1e8;
    }
  }
  std::vector<Node> nodes;
  if(!cells) {
    nodes.reserve(2*n);
    buildNodes(0,n,order,positions,split,pbc,nodes);
  }
  const double padding=roundoff(positions,box);
  radius=std::max(buffer,extent/std::cbrt(static_cast<double>(n)));
  // Bound memory for highly clustered groups or widely separated components.
  // Dense Prim still needs only O(N) storage and computes each pair once.
  const std::size_t limit=std::max<std::size_t>(4096,32*static_cast<std::size_t>(n));
  double previousSquared=-1;
  bool finalRadius=false;
  for(;;) {
    edgesSorted=false;
    bool bounded=true;
    if(cells) {
      bounded=findCellEdges(positions,pbc,padding,limit,previousSquared);
    } else {
      for(unsigned j=0; j<n && bounded; ++j) {
        bounded=findEdges(0,j,order,nodes,positions,pbc,padding,limit,previousSquared);
      }
    }
    if(!bounded) {
      edges.clear();
      const double target=denseTree(positions,pbc)+buffer;
      // An anisotropic cloud can make the initial radius estimate too large.
      // Try the actual bottleneck before committing to the dense fallback.
      if(target<radius) {
        radius=target;
        previousSquared=-1;
        finalRadius=true;
        continue;
      }
      dense=true;
      return;
    }
    if(!finalRadius) {
      Components components(n,componentParent,componentSize);
      unsigned remaining=n;
      for(const auto& e:edges) {
        if(components.join(e.first,e.second)) {
          --remaining;
        }
      }
      if(remaining>1) {
        previousSquared=(radius+padding)*(radius+padding);
        radius*=2;
        plumed_assert(std::isfinite(radius)) << "DYNAMIC_EMST search radius is not representable";
        continue;
      }
    }
    double longest=0;
    if(sparseTree(positions,pbc,longest,false)) {
      // Add a full buffer above the connecting bottleneck, even if the initial
      // radius happened to connect with almost no margin.
      const double target=longest+buffer;
      if(!finalRadius && radius!=target) {
        const double oldRadius=radius;
        radius=target;
        finalRadius=true;
        if(radius<oldRadius) {
          const double cutoff=(radius+padding)*(radius+padding);
          edges.erase(std::remove_if(edges.begin(),edges.end(),[&](const Edge& e) {
            return e.weight>cutoff;
          }),edges.end());
          // Removing long edges preserves both the MST and its ordering.
          complete=edges.size()==static_cast<std::size_t>(n)*(n-1)/2;
          return;
        }
        previousSquared=(oldRadius+padding)*(oldRadius+padding);
        continue;
      }
      complete=edges.size()==static_cast<std::size_t>(n)*(n-1)/2;
      return;
    }
    plumed_merror("DYNAMIC_EMST candidate graph unexpectedly disconnected");
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
  if(!initialized || positions.size()!=reference.size()) {
    rebuild(positions,pbc,buffer);
    return;
  }
  if(complete && !dense) {
    double longest=0;
    sparseTree(positions,pbc,longest);
    return;
  }
  Tensor deformation;
  deformation(0,0)=deformation(1,1)=deformation(2,2)=1;
  double contraction=1;
  if(!sameBox) {
    // In row-vector convention B_current=B_reference*F. For every periodic
    // displacement, |v F| >= (1-||F-I||_F)|v|. Subtract the current-cell
    // nonaffine motion below. No assumption about unchanged image labels is
    // made. The Frobenius bound is cheap and conservative for small cell moves.
    if(!pbc.isSet() || determinant(box)==0) {
      rebuild(positions,pbc,buffer);
      return;
    }
    const Tensor reciprocal=inverse(box);
    deformation=matmul(reciprocal,pbc.getBox());
    double change2=0,reciprocal2=0,current2=0;
    for(unsigned j=0; j<3; ++j)
      for(unsigned k=0; k<3; ++k) {
        const double difference=deformation(j,k)-(j==k ? 1.0 : 0.0);
        change2+=difference*difference;
        reciprocal2+=reciprocal(j,k)*reciprocal(j,k);
        current2+=pbc.getBox()(j,k)*pbc.getBox()(j,k);
      }
    const double error=1024*std::numeric_limits<double>::epsilon()*(1+std::sqrt(reciprocal2)*std::sqrt(current2));
    contraction=1-std::sqrt(change2)-error;
    if(!std::isfinite(contraction) || contraction<=0) {
      rebuild(positions,pbc,buffer);
      return;
    }
  }
  double displacement=0;
  double translatedDisplacement=0;
  const Vector first=sameBox ? reference[0] : matmul(reference[0],deformation);
  const Vector translation=pbc.distance(first,positions[0]);
  for(unsigned j=0; j<positions.size(); ++j) {
    const Vector affine=sameBox ? reference[j] : matmul(reference[j],deformation);
    displacement=std::max(displacement,pbc.distance(affine,positions[j]).modulo());
    translatedDisplacement=std::max(translatedDisplacement,pbc.distance(affine+translation,positions[j]).modulo());
  }
  // Either bound is valid, since a common translation leaves pair distances
  // unchanged. Taking the smaller one avoids rebuilding for rigid translation
  // without letting a particularly mobile anchor worsen the original bound.
  displacement=std::min(displacement,translatedDisplacement);
  if(dense) {
    if(!sameBox || displacement>buffer) {
      rebuild(positions,pbc,buffer);
    } else {
      denseTree(positions,pbc);
    }
    return;
  }
  double longest=0;
  const bool connected=sparseTree(positions,pbc,longest);
  // Omitted distances were >radius at build. Cell deformation contracts them
  // by no more than contraction, and nonaffine motion reduces them by <=2*d.
  // If the current tree's bottleneck is smaller, no omitted edge improves it.
  const double padding=roundoff(positions,box)+roundoff(reference,box);
  if(!connected || !(longest+2*displacement+padding<contraction*radius)) {
    rebuild(positions,pbc,buffer);
  }
}
}
