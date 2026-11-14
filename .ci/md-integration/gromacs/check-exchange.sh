#!/usr/bin/env bash
# Independent small-system exchange/MCMC oracles, using the existing MD build.
set -euo pipefail
: "${BACKEND:?set BACKEND to tmpi, openmpi or mpich}"
gmx=${1:?Usage: check-exchange.sh GMX KERNEL RESULTS}
kernel=${2:?set the serial PLUMED kernel}
results=${3:?set a fresh results directory}
suite=$(cd "$(dirname "$0")/exchange" && pwd)
export PLUMED_KERNEL="$kernel"
unset PLUMED_MPI_WRAPPER PLUMED_MPI_WRAPPER_FORCE PLUMED_HREX
if [[ "$BACKEND" == tmpi ]]; then
  python "$suite/validate_mpi_modes.py" --gmx "$gmx" --kernel "$kernel" --output "$results/tmpi"
else
  : "${MD_MPIEXEC:?set the matching host MPI launcher}"
  launcher="$MD_MPIEXEC -n {n}"
  if [[ ${MD_LAUNCHER_KIND:-$BACKEND} == openmpi ]]; then
    launcher="$MD_MPIEXEC --oversubscribe --bind-to none -n {n}"
  fi
  # Local sandbox validation can specify explicit localhost slots.
  launcher=${MD_EXCHANGE_LAUNCHER:-$launcher}
  python "$suite/../check-provider-input.py" "$(dirname "$(dirname "$gmx")")" "$results/provider-input" "$kernel"
  python "$suite/validate.py" --gmx "$gmx" --output "$results/exchange" \
    --launcher "$launcher" --hrex --filter n2-r1-t0- \
    --mdrun-args '-nb cpu -pme cpu -update cpu'
  python "$suite/validate.py" --gmx "$gmx" --output "$results/exchange-two-ranks" \
    --launcher "$launcher" --hrex --filter n2-r2-t0-energy \
    --mdrun-args '-nb cpu -pme cpu -update cpu'
  python "$suite/validate_lambda.py" --gmx "$gmx" --kernel "$kernel" \
    --output "$results/lambda" --launcher "$launcher" --quick
  for simulator in modular legacy; do
    extra=(--quick)
    if [[ "$simulator" == legacy ]]; then extra+=(--legacy); fi
    python "$suite/validate_expanded.py" --gmx "$gmx" --kernel "$kernel" \
      --output "$results/expanded-$simulator" --launcher "$launcher" "${extra[@]}"
  done
fi
