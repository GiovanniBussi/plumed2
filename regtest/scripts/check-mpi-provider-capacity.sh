#!/usr/bin/env bash
# A provider must reject native representations that exceed its configured ABI.
set -euo pipefail
if (($# != 1)); then
  echo 'usage: check-mpi-provider-capacity.sh MPI_CXX_COMPILER' >&2
  exit 2
fi
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
TASK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/plumed-provider-capacity.XXXXXX")
trap 'rm -rf "$TASK_DIR"' EXIT
compiler=$1
arguments=(-x c++ -std=c++98 -DPLUMED_MPIW_NATIVE_ADAPTER \
  -I"$ROOT/src/mpiwrap" -c "$ROOT/src/mpiwrap/mpiwrap_implementation.cpp")
"$compiler" "${arguments[@]}" -o "$TASK_DIR/valid.o"
if "$compiler" "${arguments[@]}" -DPLUMED_MPIW_HANDLE_TYPE=uint8_t \
    -o "$TASK_DIR/invalid.o" > "$TASK_DIR/error.log" 2>&1; then
  echo 'native provider accepted undersized handle storage' >&2; exit 1
fi
if ! grep -F 'storage too small' "$TASK_DIR/error.log" >/dev/null; then
  cat "$TASK_DIR/error.log" >&2
  echo 'native provider failed without a handle-capacity diagnostic' >&2; exit 1
fi
echo 'native provider rejects undersized configured handle storage at build time'
