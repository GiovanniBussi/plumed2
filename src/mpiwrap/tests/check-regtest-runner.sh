#!/usr/bin/env bash
set -euo pipefail

repo=$(cd "$(dirname "$0")/../../.." && pwd)
tmp=$(mktemp -d "${TMPDIR:-/private/tmp}/mpiwrap-regtest.XXXXXX")
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/regtest/scripts" "$tmp/regtest/case/deep"
cp "$repo/regtest/scripts/run" "$tmp/regtest/scripts/run"
cp "$repo/regtest/scripts/fixzeroes.sh" "$tmp/regtest/scripts/fixzeroes.sh"
mkdir -p "$tmp/bin"
cat > "$tmp/bin/plumed" <<'EOF'
#!/usr/bin/env bash
if [[ "$*" = "--no-mpi info --root" ]]; then
  echo "$TEST_REPO_ROOT"
  exit 0
fi
if [[ "$*" = "--no-mpi config has real_mpi" ]]; then
  if [[ ${TEST_OLD_CONFIG:-no} = yes ]]; then
    echo "real_mpi not found"
    exit 1
  fi
  if [[ ${TEST_NATIVE_MPI:-no} = yes ]]; then
    echo "real_mpi on"
    exit 0
  fi
  echo "real_mpi off"
  exit 1
fi
if [[ "$*" = "--no-mpi config -q has mpi" ]]; then
  [[ ${TEST_NATIVE_MPI:-no} = yes ]]
  exit
fi
if [[ "$*" = "--no-mpi config -q mpiexec" || "$*" = "--no-mpi config mpiexec" ]]; then
  [[ ${TEST_NATIVE_MPI:-no} = yes ]] || exit 1
  echo "$TEST_CONFIG_MPIRUN"
  exit 0
fi
if [[ "$*" = "--has-thread-mpi" || "$*" = "--no-mpi --has-thread-mpi" ]]; then
  [[ ${TEST_THREAD_MPI:-no} = yes ]]
  exit
fi
if [[ "$*" = "--is-installed" || "$*" = "--no-mpi --is-installed" ]]; then
  exit 1
fi
if [[ " $* " = *" driver "* ]]; then
  printf 'PLUMED %s\n' "$*" >> "$TEST_LAUNCH_LOG"
  exit 0
fi
echo "unexpected fake plumed arguments: $*" >&2
exit 2
EOF
cat > "$tmp/bin/fake-mpiexec" <<'EOF'
#!/usr/bin/env bash
printf 'MPI %s\n' "$*" >> "$TEST_LAUNCH_LOG"
[[ ${1:-} = -np && $# -ge 4 ]] || exit 2
shift 2
exec "$@"
EOF
cat > "$tmp/bin/config-mpiexec" <<'EOF'
#!/usr/bin/env bash
printf 'CONFIG %s\n' "$*" >> "$TEST_LAUNCH_LOG"
[[ ${1:-} = -np && $# -ge 4 ]] || exit 2
shift 2
exec "$@"
EOF
chmod +x "$tmp/bin/plumed" "$tmp/bin/fake-mpiexec" "$tmp/bin/config-mpiexec"

run_case() {
  local name=$1 mpi=$2 threads=$3 per_test=$4 chdir=$5 override=$6 expect=$7 old=$8
  local dir="$tmp/regtest/case/deep/$name"
  mkdir -p "$dir"
  cat > "$dir/config" <<EOF
type=driver
mpiprocs=3
arg=--test-ranks\ 3
plumed_thread_mpi=$per_test
plumed_chdir=$chdir
EOF
  : > "$TEST_LAUNCH_LOG"
  (cd "$dir" && TEST_NATIVE_MPI="$mpi" TEST_THREAD_MPI="$threads" TEST_OLD_CONFIG="$old" \
    TEST_REPO_ROOT="$repo" TEST_CONFIG_MPIRUN="$tmp/bin/config-mpiexec" \
    TEST_LAUNCH_LOG="$TEST_LAUNCH_LOG" PATH="$tmp/bin:$PATH" \
    PLUMED_PREPEND_PATH="$tmp/bin" PLUMED_MPIRUN="$override" \
    "$tmp/regtest/scripts/run" >run.out 2>run.err)
  if [[ -n $expect ]]; then
    grep -q "$expect" "$dir/report.txt"
  fi
}

check_log() {
  local pattern=$1
  if ! grep -q "$pattern" "$TEST_LAUNCH_LOG"; then
    cat "$TEST_LAUNCH_LOG" >&2
    return 1
  fi
}

export TEST_LAUNCH_LOG="$tmp/launch.log"
run_case internal no yes auto no "" "Using PLUMED internal thread-MPI with 3 ranks" no
check_log 'PLUMED --tmpi-threads 3 driver --test-ranks 3'

run_case native yes yes auto no "" "" no
check_log '^CONFIG -np 3 '
check_log '^PLUMED driver --test-ranks 3$'

run_case old_native yes no auto no "" "" yes
check_log '^CONFIG -np 3 '
check_log '^PLUMED driver --test-ranks 3$'

run_case native_override yes yes auto no "$tmp/bin/fake-mpiexec" "" no
check_log '^MPI -np 3 '
if grep -q '^CONFIG ' "$TEST_LAUNCH_LOG"; then
  echo "explicit PLUMED_MPIRUN was ignored" >&2
  exit 1
fi

run_case old no no auto no "" "MPI NOT INSTALLED" no
test ! -s "$TEST_LAUNCH_LOG"

run_case explicit no yes auto no "$tmp/bin/fake-mpiexec" "MPI NOT INSTALLED" no
test ! -s "$TEST_LAUNCH_LOG"

run_case chdir no yes no yes "" "CHDIR is unsupported with thread-MPI" no
test ! -s "$TEST_LAUNCH_LOG"

echo "regtest runner selection passed (internal, native, old-native, old-serial, override, CHDIR skip)"
