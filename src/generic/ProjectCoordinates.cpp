/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2026 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   This file is part of plumed, version 2.
   plumed is free software: you can redistribute it and/or modify it under
   the terms of the GNU Lesser General Public License as published by the
   Free Software Foundation, either version 3 of the License, or (at your
   option) any later version. See <http://www.gnu.org/licenses/>.
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#include "core/ActionAtomistic.h"
#include "core/ActionPilot.h"
#include "core/ActionRegister.h"
#include "core/ActionSet.h"
#include "core/PbcAction.h"
#include "core/PlumedMain.h"
#include "tools/Pbc.h"

#include <array>
#include <cmath>

namespace PLMD {
namespace generic {

//+PLUMEDOC GENERIC PROJECT_COORDINATES
/*
Project the stored coordinates onto a lattice plane or axis.

Write each position as $r=s_a a+s_b b+s_c c$, where $a,b,c$ are the current
cell vectors. With PLANE=ab this action replaces it with $r'=s_a a+s_b b$.
PLANE=ac and PLANE=bc retain the other pairs of fractional coordinates.
Alternatively, AXIS=a, AXIS=b or AXIS=c retains just one fractional coordinate.
For a tilted cell this is an oblique, rather than an orthogonal, projection.
The direction being removed follows the instantaneous cell.

The output box keeps the selected lattice vectors exactly unchanged. For
PLANE=ab, both $a$ and $b$ keep their original lengths and mutual angle; only
$c$ is replaced by a nonzero auxiliary vector perpendicular to their plane.
The coordinate projection sends the original $c$ to zero; the auxiliary vector
is a separate construction that keeps the output box nonsingular. For AXIS=a,
$a$ is unchanged, and the other two box vectors are replaced by nonzero
auxiliary vectors perpendicular to $a$ and to each other. Analogous rules apply
to the other selectors.

These auxiliary directions cannot shorten distances between projected positions,
so ordinary minimum-image distances give the distances in the retained periodic
lattice. The auxiliary cell volume and auxiliary directions do not describe a
physical projected system.

Projected PBC distances are invariant under independent shifts of input atoms
by any original lattice vector. For PLANE=ab, shifting an atom by
$n_a a+n_b b+n_c c$, with integer coefficients, changes its projected position
only by $n_a a+n_b b$. This is a lattice vector of the output box, so PBC
distances are unchanged. The same argument applies to the other selectors.
Thus these periodic distances do not require making molecules whole before
projection. CVs that use a whole structure without PBC may still require it.

This operation affects all real atoms and all virtual atoms defined before it.
Forces and cell derivatives are propagated to the original system, including
the box dependence of the projection and the auxiliary cell. No coordinate
snapshots are stored. A nondegenerate three-dimensional input cell is required.

!!! caution "modifies stored positions and cell"

    All subsequent atomistic actions use the projected coordinates and auxiliary
    cell. The operation is performed every step and has no END option. Define
    CVs requiring the original coordinates before PROJECT_COORDINATES.

Reference PDBs must be projected by the user using the same lattice convention
and the intended reference cell. Occupancy and beta weights keep their usual
meanings. For example, use TYPE=SIMPLE for RMSD between projected structures
when only translation should be removed. An optimal three-dimensional fit of
collinear structures is degenerate; this is particularly relevant to AXIS.
Virtual atoms defined afterwards can introduce positions outside the retained
subspace (for example a FIXEDATOM); ordinary three-dimensional distances to such
points need not have the interpretation of distances in the projected lattice.

## Examples

Calculate membrane-plane coordination while retaining a CV using full coordinates:

```plumed
full: DISTANCE ATOMS=1,2
PROJECT_COORDINATES PLANE=ab
inplane: COORDINATION GROUPA=1-10 GROUPB=11-20 R_0=0.3
PRINT ARG=full,inplane FILE=COLVAR
```

For a rectangular box, retaining the c axis computes distances using z alone:

```plumed
PROJECT_COORDINATES AXIS=c
height: DISTANCE ATOMS=1,2
PRINT ARG=height FILE=COLVAR
```
*/
//+ENDPLUMEDOC

class ProjectCoordinates : public ActionPilot, public ActionAtomistic {
  std::array<bool,3> keep{{false,false,false}};
  Tensor projector;
  Tensor auxiliaryInverse;
  std::array<Tensor,9> cellCorrection;
  PbcAction* pbcAction;
  Value* boxValue;

  // Gram-Schmidt and its analytic directional derivative. Retained vectors
  // form the first part of the basis but are left unchanged in the output cell.
  Tensor makeCell(const Tensor& box, const Tensor& tangent, Tensor& derivative) const;
public:
  static void registerKeywords(Keywords& keys);
  explicit ProjectCoordinates(const ActionOptions&);
  void calculate() override;
  void apply() override;
};

PLUMED_REGISTER_ACTION(ProjectCoordinates,"PROJECT_COORDINATES")

void ProjectCoordinates::registerKeywords(Keywords& keys) {
  Action::registerKeywords(keys);
  ActionAtomistic::registerKeywords(keys);
  keys.add("optional","PLANE","the lattice plane to retain: ab, ac or bc");
  keys.add("optional","AXIS","the lattice axis to retain: a, b or c; mutually exclusive with PLANE");
}

ProjectCoordinates::ProjectCoordinates(const ActionOptions& ao):
  Action(ao), ActionPilot(ao), ActionAtomistic(ao) {
  std::string plane,axis;
  parse("PLANE",plane);
  parse("AXIS",axis);
  if(plane.empty()==axis.empty()) {
    error("specify exactly one of PLANE=ab/ac/bc and AXIS=a/b/c");
  }
  if(!plane.empty() && plane!="ab" && plane!="ac" && plane!="bc") {
    error("PLANE must be ab, ac or bc");
  }
  if(!axis.empty() && axis!="a" && axis!="b" && axis!="c") {
    error("AXIS must be a, b or c");
  }
  for(const char c : plane+axis) {
    keep[c-'a']=true;
  }
  setStride(1);
  setGlobalCoordinateModifier();
  pbcAction=plumed.getActionSet().selectWithLabel<PbcAction*>("Box");
  if(!pbcAction) {
    error("a simulation cell is required");
  }
  boxValue=pbcAction->copyOutput(0);
  // This adds the box dependency, without asking the host to communicate atoms
  // that no downstream CV uses. The global helpers also cover virtual atoms.
  requestAtoms({});
  doNotRetrieve();
  log.printf("  retaining lattice %s %s\n",plane.empty() ? "axis" : "plane",
             (plane+axis).c_str());
}

Tensor ProjectCoordinates::makeCell(const Tensor& box, const Tensor& tangent, Tensor& derivative) const {
  Tensor result;
  std::array<Vector,3> basis,dbasis;
  unsigned count=0;
  for(unsigned pass=0; pass<2; ++pass) {
    for(unsigned i=0; i<3; ++i) {
      if(keep[i]!=(pass==0)) {
        continue;
      }
      Vector v=box.getRow(i),dv=tangent.getRow(i);
      for(unsigned k=0; k<count; ++k) {
        const double projection=dotProduct(v,basis[k]);
        const double dprojection=dotProduct(dv,basis[k])+dotProduct(v,dbasis[k]);
        dv-=dprojection*basis[k]+projection*dbasis[k];
        v-=projection*basis[k];
      }
      for(unsigned j=0; j<3; ++j) {
        result(i,j)=keep[i] ? box(i,j) : v[j];
        derivative(i,j)=keep[i] ? tangent(i,j) : dv[j];
      }
      const double length=modulo(v);
      basis[count]=v/length;
      dbasis[count]=(dv-basis[count]*dotProduct(basis[count],dv))/length;
      ++count;
    }
  }
  return result;
}

void ProjectCoordinates::calculate() {
  const Tensor box=pbcAction->getPbc().getBox();
  double scale=1;
  for(unsigned i=0; i<3; ++i) {
    scale*=modulo(box.getRow(i));
  }
  const double det=determinant(box);
  if(!std::isfinite(scale) || !std::isfinite(det) || scale==0 || std::fabs(det)<1e-12*scale || !pbcAction->getPbc().isSet()) {
    error("PROJECT_COORDINATES requires a finite, nondegenerate three-dimensional cell");
  }
  Tensor mask;
  for(unsigned i=0; i<3; ++i) {
    mask(i,i)=keep[i] ? 1 : 0;
  }
  projector=matmul(transpose(box),matmul(mask,transpose(inverse(box))));
  Tensor derivative;
  const Tensor auxiliary=makeCell(box,Tensor(),derivative);
  auxiliaryInverse=inverse(auxiliary);
  for(unsigned i=0; i<3; ++i) {
    for(unsigned j=0; j<3; ++j) {
      Tensor tangent;
      for(unsigned k=0; k<3; ++k) {
        tangent(k,j)=box(k,i);
      }
      makeCell(box,tangent,derivative);
      for(unsigned k=0; k<3; ++k) {
        derivative(k,j)-=auxiliary(k,i);
      }
      cellCorrection[3*i+j]=derivative;
    }
  }
  transformAllGlobalPositions(projector);
  pbcAction->getPbc().setBox(auxiliary);
}

void ProjectCoordinates::apply() {
  Tensor virial=getVirial();
  // At fixed fractional coordinates Q(h)r transforms affinely under a strain.
  // Only the nonaffine change of the auxiliary cell needs a virial correction.
  // Its retained rows are zero. On the other rows, h_aux^{-T} annihilates every
  // projected position: the coordinate moment in the incoming virial therefore
  // drops out, leaving exactly the explicit auxiliary-cell gradient below.
  const Tensor cellGradient=matmul(transpose(auxiliaryInverse),virial);
  for(unsigned i=0; i<3; ++i) {
    for(unsigned j=0; j<3; ++j) {
      for(unsigned k=0; k<3; ++k) {
        for(unsigned l=0; l<3; ++l) {
          virial(i,j)+=cellGradient(k,l)*cellCorrection[3*i+j](k,l);
        }
      }
    }
  }
  rotateForcesOnAllAtoms(transpose(projector));
  boxValue->clearInputForce();
  for(unsigned i=0; i<3; ++i) {
    for(unsigned j=0; j<3; ++j) {
      boxValue->addForce(3*i+j,virial(i,j));
    }
  }
}

}
}
