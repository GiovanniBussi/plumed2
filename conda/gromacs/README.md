# GROMACS Conda snapshots

After integration, select **Actions → Publish GROMACS Conda test packages →
Run workflow**. The default builds and uploads all ten variants. Clear **upload**
to build downloadable artifacts only. Uploads require a manual run on upstream
master and `CONDA_UPLOAD_TOKEN`; branch and fork builds cannot publish.

The workflow builds GROMACS 2026.4 with `plumed patch -p --runtime`, using exactly
the triggered PLUMED checkout. It freezes that checkout's full SHA and one UTC
snapshot date before starting the platform builds. No later fetch of master is
used for patching. `config.json` pins the GROMACS archive checksum and version.

| Platform | CPU | CUDA 12.9 |
|---|---|---|
| Linux x86-64 | thread-MPI, Open MPI | thread-MPI, Open MPI |
| Linux ARM64 | thread-MPI, Open MPI | |
| Intel macOS | thread-MPI, Open MPI | |
| Apple Silicon | thread-MPI, Open MPI | |

All builds use mixed precision, OpenMP, conda-forge toolchains, FFTW and hwloc.
CPU builds use SSE2 on x86 and NEON on ARM to avoid requiring the builder's CPU.
CUDA builds target NVIDIA SM 75/80/86/89/90 and are tested on their CPU path.
GPU numerical execution is **not** validated by this workflow.

Package identity includes GROMACS version, date, PLUMED short SHA, MPI/GPU variant
and a unique run/attempt build revision. For example, version
`2026.4.post20261124` and build
`openmpi_cuda_plumed_g0123456789ab_hHASH_REVISION`. Full provenance, source
checksums, the applied patch digest and the tested runtime source are installed
in `share/gromacs-plumed/provenance.json`; `gmx --version` reports the PLUMED
snapshot date and short SHA in the fork marker. Dates identify snapshot creation;
the PLUMED commit timestamp is recorded separately. Runtime kernel selection
does not change this patch-source identity.

Each platform first builds a small serial PLUMED patcher/test kernel with no
external optional scientific libraries and no MPI add-on. It includes the
cltools and basic action modules required by numerical tests, not all optional
modules, Python wrappers or plugins. These `gromacs_test_...` packages are
temporary CI artifacts and are **never uploaded**. GROMACS owns MPI and injects
it into the runtime kernel.

Installed GROMACS depends on the full `plumed` package, at least as recent as the
snapshot date, from `plumed/label/test`. Before an upstream publication, every
variant must also pass its numerical/exchange/restart tests against a full
runtime resolved from that channel. If no sufficiently recent compatible full
PLUMED snapshot exists, publish one using the separate PLUMED publication
workflow, then start a fresh GROMACS publication run. The GROMACS workflow does
not replace channel PLUMED with its minimal test kernel.

All builds and mandatory tests must pass before upload. The uploader verifies
the complete ten-package manifest, uploads only those archives, and refuses to
overwrite different existing bytes. Identical already-uploaded test packages
are skipped on an upload retry. Upload is not transactional: a network failure
can leave a partial set, recorded in job logs. Retrying just the upload uses
the existing artifacts; rebuilding uses a new run/attempt revision.

Example installation (choose the displayed date and desired build):

```sh
conda create -n gmx -c plumed/label/test -c conda-forge \
  'gromacs-plumed=2026.4.post20261124=openmpi_cpu_*'
```

The temporary fork-only push trigger lives on the separate
`agents/conda-gromacs/build-test` branch. Do not merge that trigger upstream.
Its package and minimal-runtime artifacts permit installation testing before
the manual entry exists on the default branch. Install both artifact archives
with conda-forge dependencies if testing before a matching full PLUMED snapshot
has been published.
