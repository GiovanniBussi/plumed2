#!/usr/bin/env bash
# Run outside the agent sandbox: real MPI needs process-manager/socket access.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")" && pwd)
PROVIDERS=()
THREAD_ONLY=no
JOBS=${JOBS:-4}
RESULTS=
SERIAL_BUILD=
CONDA_BASE=${CONDA_BASE:-${HOME}/miniconda3}
MPICH_ENV=${MPICH_ENV:-${CONDA_BASE}/envs/mpich}
OPENMPI_ENV=${OPENMPI_ENV:-${CONDA_BASE}/envs/openmpi}

usage() {
  cat <<'HELP'
Usage: ./run-mpi-tests.sh [--mpich] [--openmpi] [--thread-only] [--jobs N]
                        [--results DIRECTORY]
                        [--injection-only EXISTING_SERIAL_BUILD]

With no provider options, test both MPICH and Open MPI. Each provider gets a
fresh local clone, a default-module direct-MPI build, and the full make check
suite. A separate serial/internal-thread-MPI build is tested once, then used
for host-injection and environment-provider loading checks with each selected
MPI installation. The serial stage also runs standalone launcher, generated
tMPI injection, and provider-registry checks (CMake/CTest required). Use --thread-only to test internal thread-MPI without Conda
or an installed real MPI. No network clone, source reset, installation, or
push is performed.

The script tests committed HEAD. Commit intended changes before running it.
All clones and logs are retained in the printed results directory.
Use --injection-only with an existing serial build directory to rerun just
the injection/provider-loading checks without rebuilding or repeating the full suites.

Environment overrides:
  CONDA_BASE    Conda installation (default: $HOME/miniconda3)
  MPICH_ENV     Conda env name or path (default: $CONDA_BASE/envs/mpich)
  OPENMPI_ENV   Conda env name or path (default: $CONDA_BASE/envs/openmpi)
  JOBS          Build parallelism (default: 4)
  MPICH_CC / MPICH_CXX / OMPI_CC / OMPI_CXX
                Native compilers used by the MPI compiler wrappers
HELP
}

while (($#)); do
  case $1 in
    --thread-only) THREAD_ONLY=yes; shift ;;
    --mpich) PROVIDERS+=(mpich); shift ;;
    --openmpi) PROVIDERS+=(openmpi); shift ;;
    --jobs|--results|--injection-only)
      if (($# < 2)); then usage >&2; exit 2; fi
      case $1 in
        --jobs) JOBS=$2 ;;
        --results) RESULTS=$2 ;;
        --injection-only) SERIAL_BUILD=$(cd "$2" && pwd) ;;
      esac
      shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) usage >&2; exit 2 ;;
  esac
done
if ! [[ $JOBS =~ ^[1-9][0-9]*$ ]]; then
  echo '--jobs must be a positive integer' >&2
  exit 2
fi
if [[ $THREAD_ONLY == yes ]]; then
  if ((${#PROVIDERS[@]} > 0)) || [[ -n $SERIAL_BUILD ]]; then
    echo '--thread-only cannot be combined with provider options or --injection-only.' >&2
    exit 2
  fi
elif ((${#PROVIDERS[@]} == 0)); then
  PROVIDERS=(mpich openmpi)
fi
if [[ $THREAD_ONLY == no && ! -f $CONDA_BASE/etc/profile.d/conda.sh ]]; then
  echo "Cannot find $CONDA_BASE/etc/profile.d/conda.sh; set CONDA_BASE." >&2
  exit 2
fi
if [[ -z $RESULTS ]]; then
  RESULTS=$(mktemp -d "${TMPDIR:-/tmp}/plumed-mpi-tests.XXXXXX")
else
  mkdir -p "$RESULTS"
  RESULTS=$(cd "$RESULTS" && pwd)
  if [[ -n $(ls -A "$RESULTS") ]]; then
    echo "Results directory must be empty: $RESULTS" >&2
    exit 2
  fi
fi
echo "Source: $ROOT ($(git -C "$ROOT" rev-parse --short HEAD))"
echo "Results and local clones: $RESULTS"
injection_only=no
if [[ -n $SERIAL_BUILD ]]; then
  injection_only=yes
  if [[ ! -f $SERIAL_BUILD/Makefile.conf ]]; then
    echo "No configured serial build at $SERIAL_BUILD" >&2
    exit 2
  fi
else
  SERIAL_BUILD=$RESULTS/serial
fi
if [[ -n $(git -C "$ROOT" status --porcelain) ]]; then
  echo 'Note: working-tree edits and untracked files are excluded; testing committed HEAD.'
fi
unset PLUMED_KERNEL PLUMED_MPI_WRAPPER PLUMED_MPI_WRAPPER_FORCE PLUMED_MPIRUN
unset PLUMED_PROGRAM_NAME PLUMED_CLI_PROGRAM_NAME PLUMED_LIB PLUMED_ROOT
export MPICH_CC=${MPICH_CC:-/usr/bin/clang}
export MPICH_CXX=${MPICH_CXX:-/usr/bin/clang++}
export OMPI_CC=${OMPI_CC:-/usr/bin/clang}
export OMPI_CXX=${OMPI_CXX:-/usr/bin/clang++}

activate_provider() {
  local provider=$1 environment
  if [[ $provider == mpich ]]; then environment=$MPICH_ENV; else environment=$OPENMPI_ENV; fi
  # Conda's shell hooks may reference unset variables.
  set +u
  source "$CONDA_BASE/etc/profile.d/conda.sh"
  conda activate "$environment"
  set -u
  for executable in mpicc mpicxx mpiexec; do
    if [[ ! -x $CONDA_PREFIX/bin/$executable ]]; then
      echo "Missing $executable in activated environment $CONDA_PREFIX" >&2
      return 1
    fi
  done
  echo "Activated $provider: $CONDA_PREFIX"
}

run_serial() (
  set -e
  local build=$RESULTS/serial
  git clone --local --no-hardlinks "$ROOT" "$build" > "$RESULTS/serial-clone.log" 2>&1
  cd "$build"
  CC=/usr/bin/clang CXX=/usr/bin/clang++ ./configure --disable-mpi --enable-thread-mpi > "$RESULTS/serial-configure.log" 2>&1
  grep -q -- '-D__PLUMED_HAS_THREAD_MPI' Makefile.conf
  make -j"$JOBS" > "$RESULTS/serial-build.log" 2>&1
  make check > "$RESULTS/serial-check.log" 2>&1
  cmake -S src/thread_mpi -B "$RESULTS/thread-mpi-runtime" \
    > "$RESULTS/thread-mpi-runtime-configure.log" 2>&1
  cmake --build "$RESULTS/thread-mpi-runtime" -j"$JOBS" \
    > "$RESULTS/thread-mpi-runtime-build.log" 2>&1
  ctest --test-dir "$RESULTS/thread-mpi-runtime" --output-on-failure \
    > "$RESULTS/thread-mpi-runtime-tests.log" 2>&1
  CXX=/usr/bin/clang++ bash src/mpiwrap/check-provider-registry.sh \
    > "$RESULTS/provider-registry.log" 2>&1
  CXX=/usr/bin/clang++ bash src/mpiwrap/check-kernel-lifecycle.sh \
    > "$RESULTS/creation-lifecycle.log" 2>&1
  CXX=/usr/bin/clang++ bash src/mpiwrap/check-regtest-runner.sh \
    > "$RESULTS/runner-selection.log" 2>&1
  CXX=/usr/bin/clang++ bash src/mpiwrap/check-thread-mpi-cli.sh \
    "$build/src/lib/plumed-static" > "$RESULTS/thread-mpi-cli.log" 2>&1
  CXX=/usr/bin/clang++ bash src/mpiwrap/check-thread-mpi-lifecycle.sh \
    "$build/src/lib/install/libplumed.a" > "$RESULTS/thread-mpi-lifecycle.log" 2>&1
)

run_provider() (
  set -e
  local provider=$1 build=$RESULTS/$1
  activate_provider "$provider"
  git clone --local --no-hardlinks "$ROOT" "$build" > "$RESULTS/$provider-clone.log" 2>&1
  cd "$build"
  CC="$CONDA_PREFIX/bin/mpicc" CXX="$CONDA_PREFIX/bin/mpicxx" \
    ./configure --enable-mpi --disable-thread-mpi > "$RESULTS/$provider-configure.log" 2>&1
  # Fail rather than silently validating a serial fallback.
  grep -q -- '-D__PLUMED_HAS_MPI=1' Makefile.conf
  make -j"$JOBS" > "$RESULTS/$provider-build.log" 2>&1
  make check > "$RESULTS/$provider-check.log" 2>&1
)

run_injection() (
  set -e
  local provider=$1 extension kernel adapter
  activate_provider "$provider"
  extension=$(sed -n 's/^SOEXT=//p' "$SERIAL_BUILD/Makefile.conf")
  kernel=$SERIAL_BUILD/src/lib/libplumedKernel.$extension
  adapter=$RESULTS/provider-$provider.$extension
  PLUMED_ROOT="$SERIAL_BUILD" bash "$SERIAL_BUILD/scripts/mkmpi.sh" \
    --compiler "$CONDA_PREFIX/bin/mpicc" --out "$adapter"
  # Use the current checker, including fixes made after the kernel was built.
  "$ROOT/regtest/mpiwrap/check-mpi-host-injection.sh" \
    "$CONDA_PREFIX/bin/mpicxx" "$CONDA_PREFIX/bin/mpiexec" "$kernel" "$adapter"
)

failed=0
set +e
if [[ $injection_only == no ]]; then
  echo 'Building/testing serial PLUMED with internal thread-MPI...'
  run_serial
  serial_status=$?
  printf 'serial-thread-mpi %s\n' "$serial_status" >> "$RESULTS/status.txt"
  if ((serial_status)); then failed=1; fi
fi
for provider in "${PROVIDERS[@]}"; do
  if [[ $injection_only == no ]]; then
    echo "Building/testing direct $provider..."
    run_provider "$provider"
    status=$?
    printf '%s-direct %s\n' "$provider" "$status" >> "$RESULTS/status.txt"
    if ((status)); then failed=1; fi
  fi
  if [[ -f $SERIAL_BUILD/Makefile.conf ]]; then
    echo "Testing $provider injection and environment-provider loading..."
    run_injection "$provider" > "$RESULTS/$provider-injection.log" 2>&1
    status=$?
    printf '%s-injection %s\n' "$provider" "$status" >> "$RESULTS/status.txt"
    if ((status)); then failed=1; fi
  else
    printf '%s-injection SKIPPED-no-serial-build\n' "$provider" >> "$RESULTS/status.txt"
    failed=1
  fi
done
set -e
cat "$RESULTS/status.txt"
echo "Full logs and test reports: $RESULTS"
exit "$failed"
