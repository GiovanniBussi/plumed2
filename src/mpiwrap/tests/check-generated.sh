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
repo_root=$(CDPATH= cd -- "$script_dir/../../.." && pwd)
generated="$repo_root/src/mpiwrap/generated"

python3 "$repo_root/src/mpiwrap/tools/generate.py" --check

"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -I"$generated" \
  -fsyntax-only "$generated/wrapper-header-check.cpp"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -I"$generated" \
  -fsyntax-only -x c "$generated/provider-callbacks.h"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -I"$generated" \
  -I"$tmpi_include" -fsyntax-only "$generated/thread-mpi-symbol-check.cpp"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -I"$generated" \
  -I"$tmpi_include" -fsyntax-only "$generated/thread-mpi-entry-check.cpp"

MPICH_CXX="${MPICH_CXX:-c++}" "$mpich_mpicxx" -std=c++17 -D__PLUMED_HAS_MPI \
  -I"$generated" -fsyntax-only "$generated/native-header-check.cpp"
MPICH_CXX="${MPICH_CXX:-c++}" "$mpich_mpicxx" -std=c++17 -I"$generated" \
  -fsyntax-only "$generated/native-entry-check.cpp"
MPICH_CXX="${MPICH_CXX:-c++}" "$mpich_mpicxx" -std=c++17 \
  -fsyntax-only "$generated/native-signature-check.cpp"
OMPI_CXX="${OMPI_CXX:-c++}" "$openmpi_mpicxx" -std=c++17 -D__PLUMED_HAS_MPI \
  -I"$generated" -fsyntax-only "$generated/native-header-check.cpp"
OMPI_CXX="${OMPI_CXX:-c++}" "$openmpi_mpicxx" -std=c++17 -I"$generated" \
  -fsyntax-only "$generated/native-entry-check.cpp"
OMPI_CXX="${OMPI_CXX:-c++}" "$openmpi_mpicxx" -std=c++17 \
  -fsyntax-only "$generated/native-signature-check.cpp"

echo "generated interface compiled for provider mode, pinned tMPI, MPICH, and Open MPI"
