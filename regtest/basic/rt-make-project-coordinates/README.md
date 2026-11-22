# Lattice coordinate projection

`checks.h` drives the real kernel through the public wrapper. It checks all
three lattice planes and axes against an independent periodic-image enumeration
using reciprocal vectors of the original box, without constructing the action's
auxiliary cell. Integer translations by retained and discarded cell vectors
must leave the result unchanged.

Central finite differences check all twelve Cartesian forces and all nine
affine-strain derivatives, including the nonsymmetric virial. The cases cover
coordinate and explicit-cell biases together, a CV computed before projection
but biased afterwards, direct and neighbor-list COORDINATION, RMSD with a
user-prepared PDB, all FIT_TO_TEMPLATE types in both orders, virtual atoms,
successive projections and RESET_CELL. Repeated steps exercise force resets.
Invalid selectors, END, STRIDE and a missing cell must be rejected.

The adjacent MPI fixture reuses the serial evaluator as an oracle and compares
all six projections with two ranks, scattered atoms and changing ownership.
It checks energy, owned atomic forces and every virial component.

For box vectors stored as rows in `h`, the coordinate map is
`Q = transpose(h) D transpose(inverse(h))`, where `D` retains the selected
fractional coordinates. Forces map with `transpose(Q)`. Under simultaneous
affine strain of coordinates and box, projected positions transform affinely.
The auxiliary cell is obtained by orthogonalizing the discarded vectors against
the retained subspace and each other; its retained rows are unchanged.

Let `W` be the incoming virial, `b` the auxiliary cell and `db` its analytic
derivative for one strain component. The correction is the contraction
`(transpose(inverse(b)) W) : (db - db_affine)`. The second factor has zero
retained rows. On discarded rows the first factor contains only explicit-cell
derivatives: the reciprocal discarded vectors annihilate all projected
positions, so their contraction with the coordinate-force moment is zero.
This avoids storing original or projected positions for the reverse pass.

This action intentionally has no closing command. Future restoration work is
tracked separately by the coordinating project agent.
