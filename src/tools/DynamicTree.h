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
#ifndef __PLUMED_tools_DynamicTree_h
#define __PLUMED_tools_DynamicTree_h

#include "Vector.h"
#include "Tensor.h"
#include <cstddef>
#include <vector>

namespace PLMD {
class Pbc;

/// Exact periodic MST of current coordinates, with a buffered candidate graph.
/// The graph is reused only while omitted edges are provably longer than the
/// current tree's longest edge. Box changes always invalidate the graph.
/// Edges are ordered parent-before-child, anchored at local atom zero.
class DynamicTree {
  struct Edge {
    unsigned first,second;
    double weight;
    bool operator<(const Edge& other) const;
  };
  struct Node {
    unsigned begin,end,center,left=0,right=0;
    double radius=0;
  };
  std::vector<Edge> edges;
  std::vector<Vector> reference;
  Tensor box;
  double radius=0;
  bool initialized=false,complete=false,dense=false;
  std::size_t rebuilds=0;
  std::vector<unsigned> parents,children;
  double denseTree(const std::vector<Vector>& positions,const Pbc& pbc);
  bool sparseTree(const std::vector<Vector>& positions,const Pbc& pbc,double& longest);
  void orient(unsigned n,const std::vector<Edge>& selected);
  void rebuild(const std::vector<Vector>& positions,const Pbc& pbc,double buffer);
  unsigned buildNodes(unsigned begin,unsigned end,std::vector<unsigned>& order,
                      const std::vector<Vector>& positions,const std::vector<Vector>& split,
                      const Pbc& pbc,std::vector<Node>& nodes);
  bool findEdges(unsigned node,unsigned atom,const std::vector<unsigned>& order,
                 const std::vector<Node>& nodes,const std::vector<Vector>& positions,
                 const Pbc& pbc,double padding,std::size_t limit);
public:
  /// Update the tree using current periodic distances. buffer is a positive
  /// length, not a displacement trigger or an approximation tolerance.
  void update(const std::vector<Vector>& positions,const Pbc& pbc,double buffer);
  const std::vector<unsigned>& getParents() const {
    return parents;
  }
  const std::vector<unsigned>& getChildren() const {
    return children;
  }
  /// Diagnostics for validation and profiling.
  std::size_t getRebuildCount() const {
    return rebuilds;
  }
  std::size_t getCandidateCount() const {
    return edges.size();
  }
  bool usesDenseTree() const {
    return dense;
  }
};
}
#endif
