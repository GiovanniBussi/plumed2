#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
tmpdir=$(mktemp -d "${TMPDIR:-/tmp}/mpiw-registry.XXXXXX")
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pthread \
  -I"$repo_root/src/mpiwrap" \
  -include "$repo_root/src/mpiwrap/mpiwrap-pointers.h" \
  -x c++ "$repo_root/src/mpiwrap/provider-pointer-definitions.inc" \
  -x c++ "$repo_root/src/mpiwrap/provider-registry.cpp.inc" \
  -x c++ "$repo_root/src/mpiwrap/test-provider-registry.cpp.in" \
  -o "$tmpdir/test-provider-registry"
"$tmpdir/test-provider-registry"
echo "provider registry validation and typed restoration passed"
