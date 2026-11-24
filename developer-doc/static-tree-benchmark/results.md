# Static reconstruction startup measurements

Measured 2026-11-24 on macOS arm64 with Apple Clang 17, `-O3 -std=c++17`, native
MPI and OpenMP disabled. Both kernels use the same configure flags and module
selection (`reset`). The unmodified upstream kernel was saved before editing.

Baseline: official master `832f9c9a154e074ed707f0cd320c284589564004`.
Implementation: `12c58e0c1` (source commit; this report adds no production changes).
See [manifest.json](manifest.json) for exact source/kernel/data hashes, configuration
and selection counts, [trials.csv](trials.csv) for every trial, and
[summary.csv](summary.csv) for medians and phase times.

Data are [RCSB 1S72](https://www.rcsb.org/structure/1S72), the large ribosomal
subunit previously used for ERMSD optimization. The retained structure has 99,039
atoms in 14,705 residue groups, including ions and waters. Standard A/C/G/U RNA
accounts for 61,512 atoms in 2,871 residues. The reference contains all atoms even
when the action selects only triplets; residue centers therefore use the full
reference residue. The normalized PDB uses `ATOM_ID=order` to avoid TER serial gaps.

## Startup, including reference and metadata preparation

Median of three fresh processes per entry, milliseconds. Mode order rotates
between trials. No other local project build/test jobs run during timing;
other host load is uncontrolled. Startup includes MOLINFO parsing, action creation,
residue metadata when enabled, and the first calculation where local trees are
built. Kernel loading, coordinate-file parsing by the runner, and process teardown
are outside this timer and recorded separately. This measures startup, not MD
throughput or a universal speedup.

| Selection | Actions | Upstream | Cache, full tree | Cache + SMALL_RESIDUES | Upstream / residue mode |
| --- | ---: | ---: | ---: | ---: | ---: |
| RNA ring triplets: 8,613 atoms / 2,871 residues | 1 | 252.04 | 250.71 | 174.71 | 1.44x |
| All standard RNA: 61,512 atoms / 2,871 residues | 1 | 5,465.15 | 5,457.87 | 178.80 | 30.57x |
| Full structure: 99,039 atoms / 14,705 residues | 1 | 13,758.55 | 13,736.22 | 400.77 | 34.33x |
| Identical RNA ring-triplet requests | 4 | 589.18 | 245.05 | 171.27 | 3.44x |
| Different ring/ribose triplet requests | 3 | 478.24 | 470.02 | 186.00 | 2.57x |
| One atom per reference residue: 14,705 atoms | 1 | 470.20 | 472.21 | 368.40 | 1.28x |

For RNA, the three upstream startup times span 5,457–5,587 ms, versus 177.9–180.5 ms
with residue mode. For the complete structure they span 13,707–13,769 ms versus
398.7–404.1 ms. Read the raw trials rather than interpreting the last printed
decimal as measurement precision.

Caching alone saves repeated construction: four identical requests have a
median first-calculation cost of 458.41 ms upstream and 116.20 ms with the
shared full tree. Overall startup improves by 2.40x because reference reading
and other setup costs remain. Distinct requests do not share a tree. Single
requests retain essentially the baseline full-tree cost when the residue flag
is off; the smaller differences in that column do not establish a general gain.

Residue grouping changes the scaling of tree construction. The first-calculation
cost for the RNA selection falls from 5,331.05 ms to 12.51 ms, while setup rises
from 134.10 to 166.18 ms because residue centers are prepared. Consequently its
startup gain is about 31x, rather than the much larger tree-phase ratio. For the
small triplet selection, reference parsing already dominates residue-mode startup.

The one-atom-per-residue control has no geometric reduction: R=N. Its improvement
comes from the reduced builder's different dense Prim implementation, which
updates only unvisited vertices and stores parents, rather than fewer vertices.
It should not be interpreted as a residue-count scaling gain. That implementation
is not substituted into the default full-tree path, preserving its existing
selection and tie behavior.

## Correctness and intended contract

Every mode/trial must produce the same finite gyration values within 1e-11
relative/absolute tolerance. All 54 timed executions pass. The 100 nm cubic
benchmark box leaves the deposited structure unwrapped; it avoids introducing
an image ambiguity into startup measurements. Nontrivial independent atom image
shifts are tested separately in the regression, in rectangular and skew cells.

`MOLINFO WHOLE SMALL_RESIDUES` is opt-in. It asserts that residues remain compact
enough for direct minimum-image links to a selected representative. The expanded
tree is generally not the full selected-atom EMST. A material represented as one
extended residue must not enable this flag; ordinary `MOLINFO WHOLE` retains the
full-tree policy. No simulation atoms outside the request are added. Cached
topologies are immutable and scoped to one MOLINFO reference and ordered request.

Validation of the measured implementation:

* Full native suite: 688 pass, 512 not applicable, zero errors.
* New regression checks explicit representative/star edges, full-residue centers
  for partial selections, chain/insertion-code/TER identity, original anchoring,
  parent-before-child order, duplicate/coincident/missing-reference inputs,
  invalid flags, cache sharing/isolation/expiry/pruning and unchanged material defaults.
* Actual local GYRATION_SCALAR actions and WHOLEMOLECULES entities share topology;
  reconstructed values, atomic forces and virial match the legacy path in the fixture.
* ASan/UBSan pass with all four changed implementation units and the fixture
  instrumented. Other linked kernel units are uninstrumented.
* Source/style checks and Ruff pass. Hosted CI is tracked separately; these
  local results do not assert that a fork or upstream CI run has completed.

See [README.md](README.md) for commands and timing boundaries. No downloaded PDB,
binary kernel, or generated trajectory is required in the repository.
