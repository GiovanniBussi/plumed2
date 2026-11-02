#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: check-capacity-install.sh MPICH_MPICXX OPENMPI_MPICXX" >&2
  exit 2
fi
mpich_mpicxx=$1
openmpi_mpicxx=$2
script_dir=$(cd "$(dirname "$0")" && pwd)
repo_root=$(cd "$script_dir/../.." && pwd)
if [[ ! -r "$repo_root/Makefile.conf" ]]; then
  echo "check-capacity-install.sh: configure the source tree first" >&2
  exit 2
fi
tmp=$(mktemp -d "${TMPDIR:-/tmp}/mpiw-capacity-install.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

# Exercise the configured header through the real install-headers target.
capacity_header="$repo_root/mpiwrap-capacity-installed.h"
if [[ ! -r "$capacity_header" ]] || \
    ! grep -q '^#define PLUMED_MPIW_HANDLE_CAPACITY 24$' "$capacity_header" || \
    ! grep -q '^#define PLUMED_MPIW_STATUS_CAPACITY 192$' "$capacity_header"; then
  echo "check-capacity-install.sh: configure with handle capacity 24 and status capacity 192 first" >&2
  exit 2
fi
make -C "$repo_root/src/lib" install-headers \
  DESTDIR="$tmp/stage" includedir=/usr/include program_name=plumed \
  DIRS=../mpiwrap

include_dir="$tmp/stage/usr/include"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror \
  -I"$include_dir" -fsyntax-only -x c++ \
  "$script_dir/test-capacity-frontend.cpp.in"

MPICH_CXX="${MPICH_CXX:-c++}" "$mpich_mpicxx" -std=c++17 \
  -D__PLUMED_HAS_MPI -DGMX_THREAD_MPI=0 \
  -I"$include_dir" -fsyntax-only -x c++ \
  "$script_dir/test-capacity-native.cpp.in"
OMPI_CXX="${OMPI_CXX:-c++}" "$openmpi_mpicxx" -std=c++17 \
  -D__PLUMED_HAS_MPI -DGMX_THREAD_MPI=0 \
  -I"$include_dir" -fsyntax-only -x c++ \
  "$script_dir/test-capacity-native.cpp.in"

echo "configured public headers preserve 24-byte handles and 192-byte status storage in wrapper and native-MPI builds"
