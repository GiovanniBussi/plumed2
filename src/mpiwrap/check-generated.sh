#!/bin/sh
set -eu

if [ "$#" -ne 3 ]; then
  echo "usage: check-generated.sh MPICH_MPICXX OPENMPI_MPICXX THREAD_MPI_INCLUDE" >&2
  exit 2
fi
mpich_mpicxx=$1
openmpi_mpicxx=$2
tmpi_include=$3
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
generated="$repo_root/src/mpiwrap"

python3 "$repo_root/src/mpiwrap/generate.py" --check

"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -I"$generated" \
  -fsyntax-only -x c++ "$generated/wrapper-header-check.cpp.in"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -I"$generated" \
  -fsyntax-only -x c++ "$generated/callback-bridges.h"
"${CC:-cc}" -std=c99 -Wall -Wextra -Werror -I"$generated" \
  -fsyntax-only -x c "$generated/provider-callbacks.h"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -I"$generated" \
  -I"$tmpi_include" -fsyntax-only -x c++ "$generated/thread-mpi-symbol-check.cpp.in"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -I"$generated" \
  -I"$tmpi_include" -fsyntax-only -x c++ "$generated/thread-mpi-entry-check.cpp.in"

MPICH_CXX="${MPICH_CXX:-c++}" "$mpich_mpicxx" -std=c++17 -D__PLUMED_HAS_MPI \
  -I"$generated" -fsyntax-only -x c++ "$generated/native-header-check.cpp.in"
MPICH_CXX="${MPICH_CXX:-c++}" "$mpich_mpicxx" -std=c++17 -I"$generated" \
  -fsyntax-only -x c++ "$generated/native-entry-check.cpp.in"
MPICH_CXX="${MPICH_CXX:-c++}" "$mpich_mpicxx" -std=c++17 \
  -fsyntax-only -x c++ "$generated/native-signature-check.cpp.in"
OMPI_CXX="${OMPI_CXX:-c++}" "$openmpi_mpicxx" -std=c++17 -D__PLUMED_HAS_MPI \
  -I"$generated" -fsyntax-only -x c++ "$generated/native-header-check.cpp.in"
OMPI_CXX="${OMPI_CXX:-c++}" "$openmpi_mpicxx" -std=c++17 -I"$generated" \
  -fsyntax-only -x c++ "$generated/native-entry-check.cpp.in"
OMPI_CXX="${OMPI_CXX:-c++}" "$openmpi_mpicxx" -std=c++17 \
  -fsyntax-only -x c++ "$generated/native-signature-check.cpp.in"

echo "generated interface compiled for provider mode, pinned tMPI, MPICH, and Open MPI"
