Dynamic periodic MST regression
===============================

The complete-graph Kruskal oracle independently selects the expected edge set.
Both algorithms use Pbc for distances; this test validates MST selection,
periodic spatial pruning and displacement certification rather than revalidating
Pbc's minimum-image implementation.

400-point orthorhombic and triclinic clouds exercise sparse graphs, eight frames
of reuse, independent atom image changes, displacement-triggered rebuilding,
cell changes and selection-size changes. The oracle checks every frame and the
test checks graph rebuild counters and parent-before-child traversal.

Small tied/coincident groups check deterministic edges and an actual topology
change without candidate rebuilding. 600 coincident points trigger the bounded
memory dense fallback, also checked on a subsequent frame. Empty, singleton,
nonperiodic, nonfinite and invalid-buffer cases are included.

Public MD-interface calculations test GYRATION without a reference, analytic
versus numerical atomic forces and virial, shared WHOLEMOLECULES reconstruction,
independent entities, and DUMPATOMS at stride two. A simultaneous unmodified dump
checks that local reconstruction does not change the shared coordinates.

The fixture requires no native MPI or OpenMP. The normal hosted build matrix
also runs it with other compilers and kernel configurations. Timing is not a
test assertion.
