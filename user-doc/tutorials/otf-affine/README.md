# PLUMED reproduction for the Supporting Information

This directory supplements **Simultaneous on-the-fly refinement of structural
ensembles and forward models** (Di Vora, Gilardoni and Bussi). The manuscript's
original calculations remain those produced with Davide's tool. This package
provides an independent PLUMED/simplemd reproduction and material for a
PLUMED-NEST deposit.

Use PLUMED **2.11.0-dev at 422109dc3e9aa3aa72a55f326dee473231a1039f** or a
compatible later build, with `bias`, `colvar`, `function`, `generic`, `setup`,
`cltools` and (for SAXS) `isdb` available. A C++ compiler and development headers
are required for `LOAD FILE=MaxEntAffine.cpp`. The kernel is unmodified.
This source uses the new checkpoint API: compatibility with PLUMED 2.10 is
not claimed. Binary extensions must be rebuilt against the selected kernel.

## Run a single full reproduction

From a writable copy of this directory:

```sh
plumed mklib SimpleMDInit.cpp
cp plumed-coordination.dat plumed.dat
plumed --load ./SimpleMDInit.$(plumed info --soext) simplemd < simplemd.in > run.log
```

Use `plumed-saxs.dat` instead for SAXS. Each input runs equilibration, joint
learning and frozen production in one simplemd process, preserving velocities
and thermostat state between phases. The two routes use the same MAXENT
replacement, physical model and original target values. `LOAD` automatically
compiles the source and overrides the built-in MAXENT registration for this
PLUMED instance. Look for `LOAD-able affine MAXENT implementation` in the log.
The separate C++ class name prevents symbol clashes with the built-in action.

`initial.xyz` is Davide's `generate_chain(10,1)`: ten beads along z in a
100 x 100 x 100 periodic box. Numerical lengths correspond to nm for PLUMED;
SAXS QVALUE is in inverse angstroms and is therefore q/10. COORDINATION counts
distinct pairs; SAXS outputs are explicitly converted with `I0=50*S-5`.
No self term or factor of two is passed to refinement. Whole-chain distances
are valid because the chain extent is much smaller than half the box.
`SimpleMDInit.cpp` supplies a loadable copy of simplemd with the initial
polymer-force evaluation included, matching Davide. It passes **kBT=0.722**, without interpreting 0.722 as kelvin.

The run has 3,010,000 steps, dt=0.001, temperature=0.722, friction=1, all-pair
LJ interactions shifted at 2.5, FENE k=30/R0=1.5, and seed 0. Evolution
lasts 2,000 reduced time units after 10 units of equilibration; production
lasts 1,000. Parameters are separately averaged at every learning step over
the second half of evolution and frozen at those averages. Output stride is
100 steps (0.1 time units); it does not set the learning/averaging frequency.
`energies.dat` contains the engine's LJ energy, excluding PLUMED potentials;
add the printed FENE and refinement bias when assessing potential energies.

## Checks and the complete set of runs

Python analysis requires numpy, scipy and matplotlib. Optional comparison
against the original implementation additionally requires numba and its
original repository. Commands below use a `plumed` executable specified by
absolute path so that changing into run directories does not alter resolution.

```sh
python validate.py --plumed /path/to/plumed --output validation
python check_original.py --plumed /path/to/plumed --original /path/to/MD_refinement_on-the-fly --output original-check
python workflow.py --plumed /path/to/plumed --output runs --jobs 4
python analyze.py --runs runs --output runs/results
```

The complete workflow runs prior, reference and joint protocols, each for
three independent seeds (0, -12345, -23456) and both observable routes. `--quick` on workflow.py exercises
short protocols (not a scientific reproduction). Single runs can be generated
or executed with benchmark.py; use `--help` for arguments. Prior and reference
runs use the same equilibration and 1,000-unit production as the manuscript;
only the reference has the eight `5*cos(angle)` terms.

`targets-original.dat` contains the exact targets stored with Davide's joint
benchmark dataset; `provenance.json` identifies the source and checksum.
Independent PLUMED reference trajectories produce `targets-plumed.dat` during
analysis. This file is kept separate: replacing the original targets silently
would change the direct comparison. A supplementary joint run against these
newly generated targets can be requested with benchmark.py's `--target`.

## MAXENT extension and original-code conventions

`AFFINE` enables a shared scale A and offset B for any scalar ARG list. At
alpha=gamma=1 and Gaussian sigma=1, the updates are simultaneous explicit
Euler steps using the same old lambda/A/B and the current raw observables:

```
lambda += KAPPA/(1+clock/TAU) * (A*x+B-target-lambda*SIGMA^2)
A -= AFFINE_KAPPA_SCALE/(1+clock/AFFINE_TAU_SCALE) * (sum(lambda_old*x)+AFFINE_PRIOR/A_old)
B -= AFFINE_KAPPA_OFFSET/(1+clock/AFFINE_TAU_OFFSET) * sum(lambda_old)
```

`KAPPA`, `AFFINE_KAPPA_SCALE` and `AFFINE_KAPPA_OFFSET` include dt. Thus the
benchmark values are 0.0002, 0.00001 and 0.000002, respectively. TAU values are
50 reduced time units. `AFFINE_PRIOR=1` multiplies log(A); that penalty is not
a coordinate potential. Bias energy is kBT*sum(lambda*(A*x+B)), with raw-ARG
force -kBT*A*lambda. B contributes energy and learning residuals but no direct
coordinate force. Work records the complete before/after bias difference,
including A and B, at fixed coordinates.

Additional optional keywords: AFFINE_SCALE (default 1), AFFINE_OFFSET (0),
AFFINE_LAMBDA (zero vector), AFFINE_TAU_SCALE/OFFSET (50), and AFFINE_START
(equilibration duration; default 0). Zero A/B learning increments keep those
parameters fixed. TSTART and TEND specify the averaging window relative to
AFFINE_START. The action currently supports Gaussian EQUAL restraints, PACE=1,
alpha=1, one replica, unit application weight and no REWEIGHT/NO_BROADCAST.
Unsupported combinations fail explicitly. Legacy MAXENT behavior is retained
when AFFINE is absent; legacy ALPHA retains its Laplace-error meaning.

The user explicitly requested following Davide's code when it disagrees with
the manuscript or initial handoff. The reproduction therefore uses:

- Original loop index `istep=PLUMED_step-1`, including equilibration in the
  decay clock. Learning is active for `istep>nequilib` and
  `istep<=nequilib+nonthefly`.
- Old-state simultaneous gradients, followed by reflection `A=-A` if the
  Euler proposal is negative. A zero/nonfinite singular state is diagnosed.
- Post-update parameters for the force at the current coordinates. The affine
  branch updates once in calculate(), guarded by MD step; repeated evaluations
  at that same step reuse the updated state. Legacy updates remain in update().
- Davide's ran1/shuffle RNG, rather than current PLUMED's xoshiro RNG.
  Original positive seeds all initialize the same stream; independent runs
  therefore use zero and distinct negative seeds.
- Post-update averages for `istep>nequilib+nonthefly/2` through the inclusive
  upper bound. Freeze occurs at the first strictly later loop index. Davide's
  forward averaging sums are initialized with A0/B0, so the frozen values
  include an initial-value/Nwindow contribution (1e-6 for A in this benchmark).
  This source behavior is preserved for learned parameters. Explicit zero-rate
  A/B parameters stay fixed, including at freezing.
- Davide's original FENE conditional: return zero for r>=R0. The CUSTOM
  expression preserves this behavior using a strict branch, rather than
  silently imposing an infinite wall. Recorded bond lengths remain below R0.
- The log(A) prior hard-coded in the historical benchmark implementation and
  available as `_grad_fm_reg_fun_default` in the current original code.
  The current original constructor's unrelated default is L2 regularization.

The stock simplemd first half-kick omits the initial PLUMED polymer forces.
`SimpleMDInit.cpp` corrects this with a PLUMED evaluation at step zero before
integration, matching Davide's initialization. It overrides the simplemd CLI
tool when loaded with `--load`; the kernel remains unmodified. This additional
source is compiled automatically by the Python runner, or explicitly with
`plumed mklib SimpleMDInit.cpp` for the standalone command above. A deterministic
one-step NVE test verifies the initial LJ+FENE displacement to numerical precision.

## Restart and portability

The replacement stores A, B, sums, averaging count, frozen status, learning
origin and calculation guard in the PLUMED checkpoint, alongside legacy state.
Use CHECKPOINT before the observable actions. Text-only LAGMULT restart is
rejected in AFFINE mode because it cannot restore A/B and averages exactly.
Driver split-run tests inside averaging and after freezing reproduce the
uninterrupted output bitwise. **Stock simplemd does not checkpoint velocities
and RNG state**, so exact MD continuation is not promised by an XYZ restart.
The provided full protocol avoids this problem by using a single process.
Multi-replica refinement is explicitly unsupported; external-MPI execution
has not been validated for this package.

The kernel was built with Apple clang, serial, no OpenMP or external library
search; isdb was enabled. Results, validation manifests, and SI materials are
provided with the deposit. Long stochastic runs are separate from quick tests.
A PLUMED-master integration is a possible follow-up; the deposit does not
require it. The LOAD-able implementation is LGPL-3.0-or-later, retaining the
PLUMED source copyright and license header.

The compact public deposit archive is stored on the project branch at
`artifacts/otf-affine-reproduction.zip`. `nest-submission.json` contains the
submission metadata; REPORT.md and SI.tex contain the results and limitations.
No curated NEST identifier has been assigned.
