# Dynamic minimum spanning trees

`DYNAMIC_EMST` reconstructs an action's selected atoms from their current
minimum-image distances. It does not require a PDB reference or `MOLINFO`.
The first selected atom anchors the reconstructed image. Input and output atom
order is preserved.

For example, write a reconstructed trajectory every 100 steps:

```plumed
DUMPATOMS ATOMS=1-1000 FILE=whole.xyz STRIDE=100 DYNAMIC_EMST
```

Or reconstruct two entities in the shared coordinates, before subsequent actions:

```plumed
WHOLEMOLECULES ENTITY0=1-100 ENTITY1=101-200 DYNAMIC_EMST EMST_BUFFER=0.2
```

For other atomistic actions the reconstruction changes only that action's local
copy of the coordinates. For example:

```plumed
g: GYRATION_SCALAR ATOMS=1-100 DYNAMIC_EMST
PRINT ARG=g FILE=COLVAR
```

Use the concrete atomistic action (such as `GYRATION_SCALAR`) when a shortcut's
syntax does not expose these keywords.

Reconstruction is performed when the action retrieves its coordinates, even if
it would not normally call `makeWhole()`. `NOPBC`, where available, controls the
subsequent calculation; it does not disable explicitly requested `DYNAMIC_EMST`
reconstruction. Actions that do not retrieve atom coordinates cannot use this
local reconstruction. `WHOLEMOLECULES` handles its entities separately.

The minimum spanning tree (MST) is recalculated at every evaluation of the action,
using periodic distances in the current cell. This is independent of whether
the candidate graph needs rebuilding. There is no fixed tree-update interval.
The tree selected for an evaluation is kept during numerical differentiation.
Equal-weight edges are ordered deterministically by their local endpoint indices.

`EMST_BUFFER` is a positive length in the units selected by `UNITS` (default
0.2, or 0.2 nm with the default units). It controls candidate graph reuse, not
accuracy. A larger buffer can reduce rebuild frequency while increasing work
and memory per evaluation. No distance cutoff approximation is introduced.

At a graph rebuild, a periodic spatial search collects all pairs within a radius
large enough to connect the group, then uses the tree's longest edge plus the
buffer as the radius. During reuse, candidate distances are updated and their
MST is found. If its longest edge is `L` and the largest periodic displacement
since rebuilding is `d`, reuse is certified only when
`L + 2*d < radius`, with an additional floating point safety margin. Omitted
edges cannot be shorter than any tree edge under this condition. A complete
candidate graph needs no displacement certificate. Changing the atom selection
invalidates the graph; cell changes use the additional bound below.

Orthorhombic cells use periodic cell lists, with each neighboring cell visited
once even for very small grids. Triclinic cells use periodic metric tree bounds.
Radius growth retains already discovered pairs. Edge sorting exploits the
previous ordering when motion is small, with a bounded amount of repair work
before falling back to a full sort. These changes preserve the exact edge and
tie comparisons.

Small cell changes can also reuse the graph. If the reference cell deforms by
`F`, a conservative lower bound on its length contraction is
`c = 1 - ||F-I||_F`. Measure `d` after subtracting that affine deformation,
optionally also removing a common translation. Reuse requires
`L + 2*d < c*radius`, with numerical margins. Large or uncertifiable cell changes
rebuild the graph. A complete candidate graph is valid for any cell.

If too many candidate edges would be stored, construction falls back to dense
Prim with linear auxiliary memory and quadratic distance work. That fallback
is retried after sufficient motion, or a cell or selection change. Large gaps between clusters and
coincident coordinates can make the graph dense. Performance depends on geometry
and the buffer; graph reuse does not guarantee linear running time.

The MST describes geometric proximity rather than chemical bonds. For ambiguous,
dissociating or periodically percolating groups, changing the tree can change
the reconstructed images and produce discontinuities in derived quantities.
Choose entities accordingly, especially when applying a bias. Static
reference-based reconstruction remains the default when `DYNAMIC_EMST` is absent.
