/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2021-2023 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed.org for more information.

   This file is part of plumed, version 2.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

#include "Tree.h"
#include "Tools.h"
#include "Vector.h"
#include "AtomNumber.h"
#include "core/GenericMolInfo.h"
#include <vector>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <cmath>

namespace PLMD {

Tree::Tree(GenericMolInfo* moldat) {
// initialize class
  moldat_ = moldat;
// check if molinfo present
  if(!moldat_) {
    plumed_merror("MOLINFO DATA not found");
  }
// check if reference structure is whole
  if(!moldat_->isWhole()) {
    plumed_merror("Check that reference structure in PDB file is not broken by pbc and set WHOLE in MOLINFO line");
  }
}

void Tree::buildTree(const std::vector<AtomNumber> & atoms) {
  if(!atoms.empty() && !moldat_->hasSmallResidues()) {
    buildAtomTree(atoms);
    return;
  }
  root_.clear();
  tree_.clear();
  root_indexes_.clear();
  tree_indexes_.clear();
  if(!atoms.empty()) {
    buildResidueTree(atoms);
  }
}

void Tree::buildAtomTree(const std::vector<AtomNumber> & atoms) {
  // Implementation inspired from:
  // https://mayanknatani.wordpress.com/2013/05/31/euclidean-minimummaximum-spanning-tree-emst/
  //
  // clear root_ vector
  root_.clear();
  tree_.clear();
  root_indexes_.clear();
  tree_indexes_.clear();
  std::vector<Vector> positions;

  // remove atoms not in PDB file
  std::vector<unsigned> addtotree, addtoroot;
  std::vector<unsigned> newatoms;
  newatoms.reserve(atoms.size());
  positions.reserve(atoms.size());

  tree_.reserve(atoms.size());
  root_.reserve(atoms.size());
  tree_indexes_.reserve(atoms.size());
  root_indexes_.reserve(atoms.size());
  if(!moldat_->checkForAtom(atoms[0])) {
    plumed_merror("The first atom in the list should be present in the PDB file");
  }
  // store first atom
  newatoms.push_back(0);
  positions.push_back(moldat_->getPosition(atoms[0]));
  for(unsigned i=1; i<atoms.size(); ++i) {
    if(!moldat_->checkForAtom(atoms[i])) {
      // store this atom for later
      addtotree.push_back(i);
      // along with its root (the previous atom)
      addtoroot.push_back(i-1);
    } else {
      newatoms.push_back(i);
      positions.push_back(moldat_->getPosition(atoms[i]));
    }
  }

  // start EMST
  std::vector<bool> intree(newatoms.size(), false);
  std::vector<double> mindist(newatoms.size(), std::numeric_limits<double>::max());
  // initialize tree with first atom
  mindist[0] = 0.0;
  // loops
  for(unsigned i=0; i<newatoms.size(); ++i) {
    int selected_vertex = -1;
    for(unsigned j=0; j<newatoms.size(); ++j) {
      if( !intree[j] && (selected_vertex==-1 || mindist[j] < mindist[selected_vertex]) ) {
        selected_vertex = j;
      }
    }
    // add to tree
    plumed_assert(selected_vertex>=0);
    tree_indexes_.push_back(newatoms[selected_vertex]);
    tree_.push_back(atoms[newatoms[selected_vertex]]);
    intree[selected_vertex] = true;
    // update distances
    double minroot = std::numeric_limits<double>::max();
    int iroot = -1;
    for(unsigned j=0; j<newatoms.size(); ++j) {
      double dist = delta(positions[selected_vertex], positions[j]).modulo2();
      if(dist < mindist[j]) {
        mindist[j] = dist;
      }
      if(dist < minroot && intree[j] && dist>0.0) {
        minroot = dist;
        iroot = j;
      }
    }
    // add to root vector
    if(iroot>=0) {
      root_.push_back(atoms[newatoms[iroot]]);
      root_indexes_.push_back(newatoms[iroot]);
    }
  }

  // now re-add atoms not present in the PDB
  for(unsigned i=0; i<addtotree.size(); ++i) {
    tree_.push_back(atoms[addtotree[i]]);
    tree_indexes_.push_back(addtotree[i]);
    root_.push_back(atoms[addtoroot[i]]);
    root_indexes_.push_back(addtoroot[i]);
  }
}

void Tree::buildResidueTree(const std::vector<AtomNumber>& atoms) {
  const unsigned n=atoms.size();
  plumed_massert(moldat_->checkForAtom(atoms[0]),"The first atom in the list should be present in the PDB file");
  std::unordered_map<unsigned,unsigned> groups;
  std::vector<unsigned> group(n,n),representative;
  std::vector<double> centralDistance;
  for(unsigned i=0; i<n; ++i) {
    if(!moldat_->checkForAtom(atoms[i])) {
      continue;
    }
    const unsigned residue=moldat_->getReconstructionResidue(atoms[i]);
    const auto entry=groups.emplace(residue,representative.size());
    if(entry.second) {
      representative.push_back(i);
      centralDistance.push_back(std::numeric_limits<double>::infinity());
    }
    const unsigned g=group[i]=entry.first->second;
    const double distance=delta(moldat_->getPosition(atoms[i]),moldat_->getReconstructionCenter(residue)).modulo2();
    plumed_massert(std::isfinite(distance),"SMALL_RESIDUES reference distances must be finite");
    if(distance<centralDistance[g] || (distance==centralDistance[g] && atoms[i]<atoms[representative[g]])) {
      centralDistance[g]=distance;
      representative[g]=i;
    }
  }

  // Dense Prim on the actual selected representative coordinates. Distinct
  // coincident representatives are valid vertices and may have zero-length links.
  const unsigned r=representative.size();
  std::vector<Vector> positions(r);
  for(unsigned i=0; i<r; ++i) {
    positions[i]=moldat_->getPosition(atoms[representative[i]]);
  }
  std::vector<unsigned> parent(n,n),bestParent(r,r);
  std::vector<double> best(r,std::numeric_limits<double>::infinity());
  std::vector<unsigned char> visited(r,0);
  unsigned selected=0;
  for(unsigned step=0; step<r; ++step) {
    visited[selected]=1;
    unsigned next=r;
    for(unsigned j=0; j<r; ++j) {
      if(visited[j]) {
        continue;
      }
      const double distance=delta(positions[selected],positions[j]).modulo2();
      plumed_massert(std::isfinite(distance),"SMALL_RESIDUES reference distances must be finite");
      if(distance<best[j]) {
        best[j]=distance;
        bestParent[j]=selected;
      }
      if(next==r || best[j]<best[next]) {
        next=j;
      }
    }
    if(next<r) {
      parent[representative[next]]=representative[bestParent[next]];
      selected=next;
    }
  }
  for(unsigned i=0; i<n; ++i) {
    if(group[i]==n) {
      // Preserve the existing fallback for atoms absent from the reference.
      parent[i]=i-1;
    } else if(i!=representative[group[i]]) {
      parent[i]=representative[group[i]];
    }
  }

  // The first requested atom may not be its residue's representative. Re-root
  // the expanded tree there, retaining the original image anchor and local IDs.
  std::vector<unsigned> offsets(n+1,0);
  for(unsigned i=0; i<n; ++i) {
    if(parent[i]!=n) {
      ++offsets[i+1];
      ++offsets[parent[i]+1];
    }
  }
  std::partial_sum(offsets.begin(),offsets.end(),offsets.begin());
  auto cursor=offsets;
  std::vector<unsigned> adjacent(offsets.back());
  for(unsigned i=0; i<n; ++i) {
    if(parent[i]!=n) {
      adjacent[cursor[i]++]=parent[i];
      adjacent[cursor[parent[i]]++]=i;
    }
  }
  std::vector<unsigned> queue(1,0);
  std::vector<unsigned char> seen(n,0);
  seen[0]=1;
  tree_.push_back(atoms[0]);
  tree_indexes_.push_back(0);
  for(std::size_t j=0; j<queue.size(); ++j) {
    const unsigned p=queue[j];
    for(unsigned k=offsets[p]; k<offsets[p+1]; ++k) {
      const unsigned c=adjacent[k];
      if(seen[c]) {
        continue;
      }
      seen[c]=1;
      queue.push_back(c);
      root_.push_back(atoms[p]);
      root_indexes_.push_back(p);
      tree_.push_back(atoms[c]);
      tree_indexes_.push_back(c);
    }
  }
  plumed_assert(queue.size()==n);
}

const std::vector<AtomNumber> & Tree::getTree(const std::vector<AtomNumber> & atoms) {
  buildTree(atoms);
  return tree_;
}

const std::vector<AtomNumber> & Tree::getTree() const noexcept {
  return tree_;
}

const std::vector<AtomNumber> & Tree::getRoot() const noexcept {
  return root_;
}

const std::vector<unsigned> & Tree::getTreeIndexes() const noexcept {
  return tree_indexes_;
}

const std::vector<unsigned> & Tree::getRootIndexes() const noexcept {
  return root_indexes_;
}


}
