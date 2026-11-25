#!/bin/sh
set -eu
if [ "$#" -lt 2 ]; then
  echo 'usage: check-provider.sh MPICC MPIEXEC [MPIEXEC_OPTIONS...]' >&2
  exit 2
fi
mpicc=$1
mpiexec=$2
shift 2
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$here/../../.." && pwd)
tmp=$(mktemp -d "${TMPDIR:-/tmp}/mpiw-long-long.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
"$mpicc" -std=c99 -I"$root/src" -c "$here/native-host.c" -o "$tmp/host.o"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pthread -D__PLUMED_MPIWRAP \
  -I"$root/src" -iquote "$root/src/mpiwrap" \
  "$root/src/mpiwrap/mpiwrap.cpp" "$root/src/mpiwrap/loader.cpp" \
  "$root/src/mpiwrap/fallbacks.cpp" "$root/src/mpiwrap/launcher.cpp" \
  "$root/src/mpiwrap/tmpi.cpp" "$here/provider-check.cpp" "$tmp/host.o" \
  $("$mpicc" --showme:link) -o "$tmp/probe"
for ranks in 1 2 4; do
  "$mpiexec" "$@" -n "$ranks" "$tmp/probe"
  PLUMED_TEST_MISSING_LONG_LONG=1 "$mpiexec" "$@" -n "$ranks" "$tmp/probe"
done
