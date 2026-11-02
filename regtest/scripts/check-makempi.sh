#!/usr/bin/env bash
# Exercise argument boundaries, installation independence, and failed publication.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
TASK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/plumed-makempi-test.XXXXXX")
trap 'rm -rf "$TASK_DIR"' EXIT
export PLUMED_ROOT="$TASK_DIR/installed tree"
mkdir -p "$PLUMED_ROOT/src/mpiwrap" "$TASK_DIR/compiler dir" "$TASK_DIR/output dir"
for source in mpiwrap_implementation.cpp mpiwrap_implementation.cpp.inc provider-abi.h provider-callbacks.h; do
  cp "$ROOT/src/mpiwrap/$source" "$PLUMED_ROOT/src/mpiwrap/"
done
export ARGUMENT_LOG="$TASK_DIR/arguments"
cat > "$TASK_DIR/compiler dir/mpicc" <<'COMPILER'
#!/usr/bin/env bash
set -euo pipefail
printf '%s\n' "$@" > "$ARGUMENT_LOG"
output=
while (($#)); do
  if [[ "$1" == -o ]]; then output=$2; shift; fi
  shift
done
if [[ "${FAIL_COMPILE:-no}" == yes ]]; then exit 2; fi
printf 'new provider\n' > "$output"
COMPILER
chmod +x "$TASK_DIR/compiler dir/mpicc"
output="$TASK_DIR/output dir/provider.so"
printf 'previous provider\n' > "$output"
if FAIL_COMPILE=yes bash "$ROOT/scripts/makempi.sh" \
  --compiler "$TASK_DIR/compiler dir/mpicc" --out "$output" 2>/dev/null; then
  echo 'makempi accepted a failed compiler' >&2; exit 1
fi
if [[ $(cat "$output") != 'previous provider' ]]; then
  echo 'makempi damaged the previous provider on failure' >&2; exit 1
fi
literal_flag='-DVALUE=$(never-run-this)'
bash "$ROOT/scripts/makempi.sh" --compiler "$TASK_DIR/compiler dir/mpicc" \
  --out "$output" --cflag "$literal_flag" --ldflag '-L/path with spaces'
if [[ $(cat "$output") != 'new provider' ]]; then
  echo 'makempi did not publish the provider' >&2; exit 1
fi
for expected in "$literal_flag" '-L/path with spaces' '-std=c++17' \
  "$PLUMED_ROOT/src/mpiwrap/mpiwrap_implementation.cpp"; do
  if ! grep -Fx -- "$expected" "$ARGUMENT_LOG" >/dev/null; then
    echo "makempi changed argument: $expected" >&2; exit 1
  fi
done
if bash "$ROOT/scripts/makempi.sh" --compiler relative/mpicc --out "$output" 2>/dev/null; then
  echo 'makempi accepted a relative compiler path' >&2; exit 1
fi
if bash "$ROOT/scripts/makempi.sh" --compiler 2>/dev/null; then
  echo 'makempi accepted a missing option argument' >&2; exit 1
fi
shopt -s nullglob
temporary=("$TASK_DIR/output dir"/.plumed-makempi.*)
if ((${#temporary[@]})); then
  echo 'makempi left temporary build directories behind' >&2; exit 1
fi
echo 'makempi arguments, installed sources, cleanup, and publication passed'
