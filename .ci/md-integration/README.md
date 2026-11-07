# MD integration CI

The `MD integration` workflow builds serial PLUMED once on Ubuntu 24.04 and
macOS 15 (Apple Silicon). Each installation is shared as an artifact with three
GROMACS jobs on the same OS. Both PLUMED and GROMACS have OpenMP enabled.
Artifacts preserve installed symlinks and permissions and use the same absolute
installation prefix in producer and consumer jobs. They are not shared between
operating systems or architectures.
Linux jobs add the private installation's `lib` directory to `LD_LIBRARY_PATH`
before using the installed executable, including after downloading the artifact.

GROMACS is pinned to 2026.4, commit
`6c69f720aa7378fddfee67f99d51aff21ccfb0a2`. The installed PLUMED patcher applies,
reverts and reapplies the distributed `gromacs-2026.0` patch. The revert must
restore the tracked GROMACS sources exactly and remove the top-level wrapper
files. The patched build uses the installed current `Plumed.h`. GROMACS's bundled
wrapper remains untouched. Each job installs/selects its own host MPI:

- GROMACS thread-MPI;
- Open MPI;
- MPICH.

Ubuntu 24.04's packaged MPICH uses external PMIx and cannot form a shared MPI
world with its bundled Hydra launcher ([MPICH issue #7064](https://github.com/pmodels/mpich/issues/7064)).
The Linux MPICH job therefore uses Open MPI's PMIx-capable launcher, while
GROMACS and the injected provider still compile/link against MPICH. UCX uses
TCP, shared memory and self transports, avoiding unavailable hosted-runner RDMA
devices. This reuses the workaround already validated in PLUMED's injection
tests. On macOS, MPICH uses its own launcher. Every real-MPI job first runs the
existing native MPI probe with four ranks, without GROMACS or PLUMED, so a
broken MPI installation fails before the expensive GROMACS build.

The workflow is CPU-only and uses GROMACS's bundled FFTPACK to avoid installing
an FFTW package that can bring in a different MPI. This input uses cutoff
electrostatics, so FFT performance is irrelevant. Full GROMACS compilation is
intentional; GPU builds and GROMACS's own regression suite are outside this test.

## Before merging upstream

**The current branch deliberately runs MD builds on every push to
`mpiw-md-integration` and suppresses the usual CI for pushes to that branch.**
This is temporary, to iterate quickly in the GiovanniBussi test fork.

Before merging:

1. Remove the branch-specific `push` trigger from `md-integration.yml`, leaving
   `workflow_dispatch`. MD builds must remain optional.
2. Remove the matching temporary `branches-ignore` entry from `ci.yml`, restoring
   ordinary CI on every branch. The existing jobs themselves are unchanged.
3. Confirm a manual MD workflow run and the normal CI both pass.

The reusable `md-gromacs.yml` is called by the parent after the PLUMED artifacts
are ready. Further engines can have separate reusable workflows sharing those
artifacts. LAMMPS and AMBER injection/header edits should be local to their test
scripts, rather than new entries in the distributed `plumed patch` collection.
They are not implemented here.

## Numerical and injection checks

`gromacs/` contains the same 5832-atom argon fixture used for the local GROMACS
integration checks, with its topology, a 20-step MD input, PLUMED input, and a
portable Python runner. No precompiled TPR is checked in: each backend runs
`grompp` itself. Outputs remain in the requested results directory, including
the exact input files, command logs, trajectories and `summary.json`.

For every backend the runner tests 1, 2 and 4 ranks with one OpenMP thread each,
then **4 ranks with 2 OpenMP threads per rank**. The hybrid case explicitly sets
`-ntomp 2`, `OMP_NUM_THREADS=2` and `PLUMED_NUM_THREADS=2`; it is not left to CPU
count heuristics. The runner verifies the rank/thread counts in GROMACS's log
and that PLUMED receives those counts. GROMACS owns rank startup and MPI
initialization; PLUMED's internal thread-MPI launcher is not used.
The Open MPI launcher receives `--oversubscribe --bind-to none` explicitly, so
four ranks and hybrid threads can run on smaller hosted runners without relying
on version-dependent MPI environment settings. Other backends receive no Open
MPI-specific launcher options unless explicitly using Open MPI's launcher for
the PMIx workaround above.

Every biased run sets `PLUMED_MPI_WRAPPER` to a nonexistent path, disables forced
provider selection and sets `MPICC`/`MPICXX` to `/usr/bin/false`. Success therefore
requires the injected host implementation to take precedence over discovery;
the test cannot silently pass by compiling or loading another provider.

Checks cover all 21 output frames:

- finite output and complete, ordered time steps;
- analytical harmonic bias `50 * (distance - 0.3)^2` and generalized force
  `-100 * (distance - 0.3)`;
- rank/hybrid consistency of distances, bias, potential energy and positions;
- actual atomic force increments, from the first trajectory frame compared
  with a separate four-rank simulation without PLUMED.

The existing numerical tolerances from the local tests are retained: `2e-8`
for analytical bias/generalized force, `2e-6` nm for distance, `2e-4` kJ/mol for
bias, `2e-5` nm for positions, and `5e-6` relative to the initial potential energy
for energy reductions. The atomic-force comparison allows `0.02` kJ/mol/nm for
the text precision of `gmx dump`. Differences are reported rather than compared
against platform-specific reference files. Logs/results are uploaded even on
failure, with a seven-day retention period.

## Running locally

Use a serial PLUMED kernel and a GROMACS executable patched/built with the
current header. The output directory must not exist yet. From the PLUMED root:

```sh
python3 .ci/md-integration/gromacs/check.py --backend tmpi \
  --gmx /path/to/gromacs-build/bin/gmx \
  --kernel /path/to/libplumedKernel.dylib --results /tmp/gromacs-tmpi-results
```

For Open MPI or MPICH, select the corresponding backend, use `gmx_mpi`, and add
`--launcher /path/to/the/matching/mpiexec`. Ensure the MPI's runtime libraries
and process-manager helpers are accessible in the environment. On Linux use
`libplumedKernel.so`. `--check-only` rechecks an existing result directory without
rerunning the simulations (it still invokes `gmx dump` to read stored forces).
When launching Ubuntu's packaged MPICH with Open MPI, also pass
`--launcher-kind openmpi` and set `UCX_TLS=tcp,sm,self`, as the workflow does.

`build-gromacs.sh SOURCE BUILD` builds an already patched tree. Set `BACKEND`
to `tmpi`, `openmpi` or `mpich`, and optionally `CC`, `CXX`, `MD_BUILD_JOBS`.
Real MPI additionally requires `MD_MPICC` and `MD_MPICXX`; Apple Clang OpenMP
requires `MD_LIBOMP` pointing to a prefix containing `include/omp.h` and
`lib/libomp.dylib`. `setup-gromacs.sh` is the GitHub-runner dependency setup and
writes selected paths to `GITHUB_ENV`/`GITHUB_PATH`.
