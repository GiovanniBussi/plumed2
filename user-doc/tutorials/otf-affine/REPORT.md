# on-the-fly-forward-models: reproduction and validation report

Completed local implementation and source-faithful scientific validation,
19 November 2026. Davide's manuscript calculations and figures are unchanged.
The package provides two primary PLUMED inputs (COORDINATION and SAXS),
simplemd parameters, initial coordinates, exact original target data, a
LOAD-able MAXENT replacement, a small loadable simplemd replacement,
validation/analysis scripts, SI figures and NEST submission metadata.

## Scientific results

The main set contains 18 full runs: two observable routes × prior/reference/
joint protocols × three independent seeds (0, -12345, -23456). The joint
protocol uses 10 time units equilibration, 2,000 learning and 1,000 frozen
production, dt=0.001; prior/reference production lasts 1,000 after equilibration.
Both routes share the exact target spectrum stored with Davide's
`posterior_0.2_0.01_0.002_50000.pkl`. No parameters were tuned to obtain agreement.

Rg below is the mean (not the mode quoted in the manuscript). Uncertainty is
the standard error across three independent runs. Per-run 50-time-unit block
errors, spectra, frozen parameters and stationarity residuals are in
`results/runs-summary.json`. The two routes' prior/reference runs share RNG
streams and are not six independent samples.

| route | ensemble | mean Rg ± SE | mean temperature | mean seconds/run |
| --- | --- | ---: | ---: | ---: |
| coordination | prior | 1.14157 ± 0.00330 | 0.72617 | 19.4 |
| coordination | reference | 2.35455 ± 0.01766 | 0.72448 | 29.7 |
| coordination | joint | 2.32736 ± 0.00663 | 0.71810 | 122.0 |
| saxs | prior | 1.14157 ± 0.00330 | 0.72617 | 17.6 |
| saxs | reference | 2.35455 ± 0.01766 | 0.72448 | 23.5 |
| saxs | joint | 2.32908 ± 0.00419 | 0.72202 | 78.2 |

Frozen A ranges from 1.5374443 to 1.5383614; B ranges from -0.2765011
to -0.2520833. Both routes recover extended ensembles and closely reproduce
the target spectrum. The joint spectrum's RMS discrepancy from the target,
averaged over seeds, is 0.246 (COORDINATION) / 0.236 (SAXS), compared with
12.427 for the unrefined prior. A finite mismatch is expected with regularization.

The difference between route-averaged Rg is 0.00173 (0.22 combined SE).
The maximum spectral difference is 0.03599; across q, all differences are
below 0.33 combined SE. Central-angle means differ by 0.00032 rad (0.11 SE).
SAXS is faster in these local runs. Timings include LOAD compilation, use up
to four concurrent simulations, and are not isolated hardware benchmarks.

All recorded observables/parameters are finite. The largest sampled bond
length is 1.17961, below R0=1.5. Production A, B and every lambda are exactly
constant in the recorded output. Mean temperatures span approximately
0.718–0.726 across ensemble/route means, close to the prescribed 0.722.

### Optimization limitation

The prescribed finite learning protocol does **not** satisfy all stationarity
conditions within sampling error. Maximum absolute inner residual
`mean(A*I0+B)-target-lambda` ranges from 0.025 to 0.278 among main runs;
some components are statistically significant. Outer A residuals range
approximately -0.315 to 0.048, and `sum(lambda)` remains 0.663–0.711.
Thus B still has a slow negative drift. These are measured finite-optimization
limits of the unchanged source protocol, not a claim of complete minimax
convergence. No learning-rate or objective adjustment was made to hide them.
The SI should describe qualitative/statistical reproduction and this limitation.

Two additional full joint runs (one per route, seed 0) use newly generated
PLUMED-reference targets. Their separate summaries and targets are included
in `generated-target-results/`; they do not replace the original-target set.

## Validation actually completed

- All ten baseline intensities on three configurations agree with an independent
  Python pair sum: maximum error 2.85e-14. COORDINATION and converted SAXS
  combined forces differ by at most 7.11e-15.
- FENE and angular energies and their combined forces, plus the affine bias,
  agree with independent coordinate finite differences: max error 4.20e-8.
  Every q derivative is separately checked in both routes; max error <2e-8.
- Shifted LJ energy/force checks cover distances 1.1, 2.2 and 2.6. One-step NVE
  displacement agrees to numerical precision; energy differences are within
  the engine output's six-decimal precision.
- The FENE branch is finite and exactly zero at/above R0, matching Davide's
  conditional. The initial LJ+FENE half-kick agrees to 1.12e-16 in distance.
- Nonzero lambda/A/B deterministic steps, simultaneous old-state updates,
  reflection of a negative A proposal, fixed A/B rates, work including A/B,
  averaging and freezing pass independent checks.
- Driver checkpoint restarts within averaging and after freezing reproduce
  uninterrupted COLVAR and LAGMULT files bitwise. AFFINE text-only restart is
  rejected. Stock/simplemd XYZ restart cannot restore velocities/RNG exactly;
  full protocols therefore run in one process.
- The loaded replacement with AFFINE disabled matches built-in MAXENT output
  bitwise. Five existing MAXENT regression fixtures pass, comparing 13 reference
  files (Gaussian/Laplace, forces, averaging, file restart and checkpoint restart).
- Direct comparison against Davide's actual numba update function has maximum
  parameter difference 2.23e-16. A 40-step joint MD comparison through all phase
  transitions gives parameter difference 4.17e-17 and coordinate difference
  4.99e-8, entirely within seven-decimal XYZ output rounding.
- Both primary inputs pass the documented NEST parse-only command on the
  pinned compatible PLUMED version. Both C++ replacements compile against a
  fresh project-local installation, and its SAXS joint quick run succeeds.
- Python Ruff lint/format checks pass. C++ is formatted with PLUMED's astyle
  rules. The in-tree plumedcheck module-dependency scanner does not apply to
  standalone LOAD sources outside src/modules; its initial module-path errors
  are recorded, not represented as a passing module scan.

## Source conventions preserved

The user requested following Davide's implementation wherever it disagrees
with the handoff or manuscript. The package preserves the original absolute
loop-index learning clock including equilibration; old-state simultaneous
Euler gradients; negative-A reflection; updating parameters before current
coordinate forces; strict-lower/inclusive-upper post-update averaging; and
freezing on the next loop index. Original forward-parameter sums start from
A0/B0, producing an A0/Nwindow contribution at freezing (1e-6 in this benchmark).
Zero-rate A/B parameters are explicitly held fixed by the extension.

Two current-PLUMED engine differences were removed with `SimpleMDInit.cpp`:
initial polymer forces are included, and the original ran1/shuffle RNG is used
instead of modern PLUMED's xoshiro. Davide maps positive seeds to the same
stream; independent runs therefore use zero/distinct negative seeds. Short
MD is checked directly against his tool, rather than assuming engine equivalence.

The FENE implementation retains his zero branch for r>=1.5. The log(A) prior
was hard-coded in the historical benchmark and is selected explicitly via its
current original-code counterpart. The current original constructor's L2 default
is not used for this historical benchmark. Source target datasets differ;
`targets-original.dat` identifies the exact stored joint-run targets.

The optional manuscript forward-model-only comparison remains outside the
required reproduction. Holding lambda identically zero in the joint equations
does not perform a data-driven fit of A/B; that comparison needs a separate
fitting rule. Neither manuscript nor original sources were edited.

## Versions, reproducibility and publication status

PLUMED baseline: `422109dc3e9aa3aa72a55f326dee473231a1039f` (2.11.0-dev).
Original reference: `4ddf15b808c2602e5b8191a67c2bad01ffa8bd5e` at
https://github.com/DavideDiVora/MD_refinement_on-the-fly . Apple clang 17,
serial/no OpenMP/no external library search; isdb enabled. Python 3.9 with
numpy 2.0.2, scipy 1.13.1, matplotlib 3.9.4 and numba 0.60 for original checks.
No multi-replica or external-MPI validation is claimed.

`README.md` gives exact build/run/analysis commands. `provenance.json`,
run manifests and validation JSON retain seeds, versions, targets and checks.
The compact archive includes source/inputs, scripts, manifests, report and
figures; larger sampled production/learning arrays are retained locally as
`notes/results/samples.npz` (41 MiB) for follow-up analysis. Regeneration from
inputs is supported without those arrays. No machine-specific binary is needed.

NEST metadata and the ZIP are prepared for submission; no curated NEST record
has been assigned or claimed. An automated form submission was rejected with
HTTP 403 because an interactive CAPTCHA is required; the official contribution
form must be completed in a browser. The response is retained in project notes. Publishing the record requires the NEST submission
and curator acceptance. The reproduction is independent of any upstream merge.
The source-faithful action's timing, averaging quirk and limited combinations
warrant separate API review before proposing a general built-in MAXENT extension.
No changes were made to the built-in MAXENT or PLUMED master.
