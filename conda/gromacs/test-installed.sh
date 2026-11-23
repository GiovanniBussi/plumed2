#!/usr/bin/env bash
set -euo pipefail
: "${CONDA_PREFIX:?activate the installed test environment}"
: "${GROMACS_MPI:?}"
suite="$GITHUB_WORKSPACE/.ci/md-integration/gromacs"
results="$RUNNER_TEMP/gromacs-results"
kernel="$CONDA_PREFIX/lib/libplumedKernel.so"
if [[ $(uname) == Darwin ]]; then kernel="$CONDA_PREFIX/lib/libplumedKernel.dylib"; fi
export PLUMED_KERNEL="$kernel" OMP_NUM_THREADS=2 OMP_DYNAMIC=FALSE
unset PLUMED_MPI_WRAPPER PLUMED_MPI_WRAPPER_FORCE
gmx="$CONDA_PREFIX/bin/gmx"
arguments=(--backend "$GROMACS_MPI" --kernel "$kernel" --gromacs-source "$GROMACS_SOURCE"
  --results "$results/numerical")
if [[ $GROMACS_MPI == openmpi ]]; then
  gmx="$CONDA_PREFIX/bin/gmx_mpi"
  arguments+=(--launcher "$CONDA_PREFIX/bin/mpiexec" --launcher-kind openmpi)
fi
"$gmx" --version > "$RUNNER_TEMP/gromacs-version.txt" 2>&1
python "$GITHUB_WORKSPACE/conda/gromacs/test-provenance.py" "$CONDA_PREFIX" "$RUNNER_TEMP/gromacs-version.txt"
python "$suite/check.py" --gmx "$gmx" "${arguments[@]}"
# Verify automatic discovery after prefix relocation, with no kernel environment
# variable and no build-prefix library path. A single rank also tests MPI startup.
mkdir -p "$results/discovery"
cp "$suite/plumed.dat" "$results/discovery/"
(
  cd "$results/discovery"
  unset PLUMED_KERNEL LD_LIBRARY_PATH DYLD_LIBRARY_PATH
  extra=(-ntomp 1 -pin off -nb cpu -pme cpu -update cpu)
  if [[ $GROMACS_MPI == tmpi ]]; then extra+=(-ntmpi 1); fi
  "$gmx" mdrun -s "$results/numerical/base.tpr" -plumed plumed.dat \
    -nsteps 1 "${extra[@]}" > run.log 2>&1
)
if [[ $GROMACS_MPI == tmpi ]]; then
  python "$suite/exchange/validate_mpi_modes.py" --gmx "$gmx" --kernel "$kernel" --output "$results/tmpi"
  python "$suite/exchange/validate_expanded.py" --gmx "$gmx" --kernel "$kernel" \
    --backend tmpi --quick --output "$results/expanded"
else
  launcher="$CONDA_PREFIX/bin/mpiexec --oversubscribe --bind-to none -n {n}"
  python "$suite/exchange/validate.py" --gmx "$gmx" --output "$results/exchange" \
    --launcher "$launcher" --hrex --filter n2-r1-t0- --mdrun-args '-nb cpu -pme cpu -update cpu'
  python "$suite/exchange/validate_checkpoint.py" --gmx "$gmx" --kernel "$kernel" \
    --launcher "$launcher" --output "$results/checkpoint"
  python "$suite/exchange/validate_rlist.py" --gmx "$gmx" --kernel "$kernel" \
    --launcher "$launcher" --output "$results/hrex-rlist"
  python "$suite/exchange/validate_expanded.py" --gmx "$gmx" --kernel "$kernel" \
    --launcher "$launcher" --quick --output "$results/expanded"
fi
