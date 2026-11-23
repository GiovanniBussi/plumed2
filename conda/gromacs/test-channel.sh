#!/usr/bin/env bash
set -euo pipefail
suite="$GITHUB_WORKSPACE/.ci/md-integration/gromacs/exchange"
results="$RUNNER_TEMP/channel-results"
mkdir -p "$results"
kernel="$CONDA_PREFIX/lib/libplumedKernel.so"
if [[ $(uname) == Darwin ]]; then kernel="$CONDA_PREFIX/lib/libplumedKernel.dylib"; fi
export PLUMED_KERNEL="$kernel" OMP_DYNAMIC=FALSE
unset PLUMED_MPI_WRAPPER PLUMED_MPI_WRAPPER_FORCE
gmx="$CONDA_PREFIX/bin/gmx"
if [[ $GROMACS_MPI == openmpi ]]; then gmx="$CONDA_PREFIX/bin/gmx_mpi"; fi
"$gmx" --version > "$results/version.txt" 2>&1
python "$GITHUB_WORKSPACE/conda/gromacs/test-provenance.py" "$CONDA_PREFIX" "$results/version.txt"
if [[ $GROMACS_MPI == tmpi ]]; then
  python "$suite/validate_mpi_modes.py" --gmx "$gmx" --kernel "$kernel" --output "$results/tmpi"
else
  launcher="$CONDA_PREFIX/bin/mpiexec --oversubscribe --bind-to none -n {n}"
  python "$suite/validate_checkpoint.py" --gmx "$gmx" --kernel "$kernel" \
    --launcher "$launcher" --case ordinary --output "$results/checkpoint"
fi
