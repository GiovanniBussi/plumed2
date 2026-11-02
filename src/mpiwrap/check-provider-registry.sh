#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
tmpdir=$(mktemp -d "${TMPDIR:-/tmp}/mpiw-registry.XXXXXX")
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

for capacity in default 16; do
  capacity_flag=
  if [ "$capacity" = 16 ]; then capacity_flag=-DPLUMED_MPIW_HANDLE_CAPACITY=16; fi
  "${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pthread $capacity_flag \
    -I"$repo_root/src/mpiwrap" \
    -include "$repo_root/src/mpiwrap/mpiwrap-pointers.h" \
    -x c++ "$repo_root/src/mpiwrap/provider-pointer-definitions.inc" \
    -x c++ "$repo_root/src/mpiwrap/provider-registry.cpp.inc" \
    -x c++ "$repo_root/src/mpiwrap/test-provider-registry.cpp.in" \
    -o "$tmpdir/test-provider-registry-$capacity"
  "$tmpdir/test-provider-registry-$capacity"
done
echo "provider registry validation and typed restoration passed with default and 16-byte handle storage"
