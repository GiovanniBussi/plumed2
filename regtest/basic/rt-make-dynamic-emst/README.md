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

Additional frames check small affine deformation plus common translation with
no rebuild, large nonaffine changes of edge order, orthorhombic grids with one,
two and many cells, motion-triggered recovery from dense fallback and a thin
line whose initial search radius would otherwise exceed the memory limit.

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

Matched performance comparison (run after building the kernel, with no other
local build or test jobs running):

```
python3 benchmark.py --work /path/inside/project/benchmark \
  --sizes 1000,5000 --profiles orthorhombic,triclinic,deforming \
  --frames 5 --trials 3
```

The helper obtains the baseline implementation from Git (default commit
`cde0962e5`), gives it a separate class name, and compiles both implementations
into one executable using the same Pbc kernel. Every timed frame's selected
edge set is compared outside the timer; a mismatch aborts the run. CSV output
reports median batch time per evaluation, candidate count, actual rebuilds and
dense fallback. Initial construction is excluded from reuse timing; rebuild
timing creates a fresh builder for every frame. These are builder timings,
without coordinate reconstruction or the rest of an MD calculation.

Other profiles are `compact`, `elongated`, and `clusters`; `--buffers` accepts a
comma-separated sweep. Use `--motion` and `--frames` to measure graph lifetime
and amortized cost as well as the nearly stationary case. All profiles are
synthetic; timing is neither a regression assertion nor a guarantee for real
trajectories.
