#!/bin/sh
set -eu

gromacs_repo=${GROMACS_REPO:-/Users/bussi/plumed-codex/gromacs}
work=$(mktemp -d "${TMPDIR:-/private/tmp}/mpiw-layout-probe.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM
repo=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)

cat > "$work/mpi-layout.cpp" <<'EOF'
#include <mpi.h>
#include <cstdio>
namespace PLMD {
int run_probe() {
  char library[MPI_MAX_LIBRARY_VERSION_STRING];
  int library_length = 0;
  MPI_Get_library_version(library, &library_length);
  for (int i = 0; i < library_length; ++i) {
    if (library[i] == '\n' || library[i] == '\r' || library[i] == ',') {
      library[i] = '\0';
      break;
    }
  }
  std::printf("library=%s\n", library);
  std::printf("MPI_VERSION=%d MPI_SUBVERSION=%d\n", MPI_VERSION, MPI_SUBVERSION);
  std::printf("MPI_Comm=%zu/%zu MPI_Group=%zu/%zu MPI_Datatype=%zu/%zu\n",
              sizeof(MPI_Comm), alignof(MPI_Comm), sizeof(MPI_Group), alignof(MPI_Group),
              sizeof(MPI_Datatype), alignof(MPI_Datatype));
  std::printf("MPI_Request=%zu/%zu MPI_Op=%zu/%zu MPI_Errhandler=%zu/%zu MPI_Status=%zu/%zu\n",
              sizeof(MPI_Request), alignof(MPI_Request), sizeof(MPI_Op), alignof(MPI_Op),
              sizeof(MPI_Errhandler), alignof(MPI_Errhandler), sizeof(MPI_Status), alignof(MPI_Status));
  return 0;
}
}
int main() { return PLMD::run_probe(); }
EOF

for provider in mpich openmpi; do
  bindir=/Users/bussi/miniconda3/envs/$provider/bin
  if [ ! -x "$bindir/mpicxx" ]; then
    echo "$provider: unavailable (no executable $bindir/mpicxx)"
    continue
  fi
  case "$provider" in
    mpich) export MPICH_CXX=${MPICH_CXX:-/usr/bin/clang++} ;;
    openmpi) export OMPI_CXX=${OMPI_CXX:-/usr/bin/clang++} ;;
  esac
  echo "[$provider] wrapper=$bindir/mpicxx cxx=${MPICH_CXX:-${OMPI_CXX:-default}}"
  if "$bindir/mpicxx" -std=c++17 -Wall -Wextra "$work/mpi-layout.cpp" -o "$work/mpi-layout-$provider"; then
    "$work/mpi-layout-$provider"
  else
    echo "[$provider] unavailable: wrapper compile/link failed"
  fi
done

${CXX:-c++} -std=c++17 -Wall -Wextra "$repo/src/mpiwrap/development/probe-abi.cpp" \
  -I"$repo/src/mpiwrap/development" -o "$work/probe-abi-cxx"
${CC:-clang} -std=c11 -Wall -Wextra "$repo/src/mpiwrap/development/probe-abi-c.c" \
  -I"$repo/src/mpiwrap/development" -o "$work/probe-abi-c"
"$work/probe-abi-cxx" | sed -n -e '/^status /p' -e '/^descriptor /p' \
  -e '/^function-entry /p' -e '/^metadata-entry /p' -e '/^native-C-int /p' > "$work/layout-cxx.txt"
"$work/probe-abi-c" > "$work/layout-c.txt"
diff -u "$work/layout-c.txt" "$work/layout-cxx.txt"
echo "[provider-abi] C11 and C++17 descriptor layouts match"
"$work/probe-abi-cxx"

tag=v2026.4
commit=6c69f720aa7378fddfee67f99d51aff21ccfb0a2
actual=$(git -C "$gromacs_repo" rev-parse "$tag^{commit}")
test "$actual" = "$commit"
mkdir -p "$work/gromacs"
git -C "$gromacs_repo" archive "$commit" src/external/thread_mpi/include | tar -x -C "$work/gromacs"
cat > "$work/tmpi-signature.cpp" <<'EOF'
#include "thread_mpi/tmpi.h"
#include "thread_mpi/mpi_bindings.h"
#include <cstdio>
#include <type_traits>
static_assert(std::is_same<decltype(&tMPI_Error_string),
              int (*)(int, char*, int*)>::value, "unexpected tMPI_Error_string signature");
static_assert(std::is_same<tMPI_Errhandler_fn, void (*)(tMPI_Comm*, int*)>::value,
              "unexpected tMPI_Errhandler_fn signature");
static_assert(std::is_same<decltype(&tMPI_Create_errhandler),
              int (*)(tMPI_Errhandler_fn*, tMPI_Errhandler*)>::value,
              "unexpected tMPI_Create_errhandler signature");
static_assert(std::is_same<decltype(&tMPI_Init),
              int (*)(int*, char***, int (*)(int, char**))>::value,
              "tMPI_Init is not the standard two-argument MPI_Init");
static_assert(std::is_same<decltype(&tMPI_Init_fn),
              int (*)(int, int, tMPI_Affinity_strategy,
                      void (*)(const void*), const void*)>::value,
              "unexpected tMPI_Init_fn signature");
static_assert(std::is_same<decltype(&tMPI_Sendrecv),
              int (*)(const void*, int, tMPI_Datatype, int, int, void*, int,
                      tMPI_Datatype, int, int, tMPI_Comm, tMPI_Status*)>::value,
              "unexpected tMPI_Sendrecv signature");
#ifdef MPI_Allgather
#error "The pinned tMPI compatibility header unexpectedly defines MPI_Allgather"
#endif
#ifdef MPI_Allgatherv
#error "The pinned tMPI compatibility header unexpectedly defines MPI_Allgatherv"
#endif
#ifdef MPI_Comm_f2c
#error "The pinned tMPI compatibility header unexpectedly defines MPI_Comm_f2c"
#endif
namespace PLMD {
int run_probe() {
  std::printf("tMPI_Comm=%zu/%zu tMPI_Group=%zu/%zu tMPI_Datatype=%zu/%zu\n",
              sizeof(tMPI_Comm), alignof(tMPI_Comm), sizeof(tMPI_Group), alignof(tMPI_Group),
              sizeof(tMPI_Datatype), alignof(tMPI_Datatype));
  std::printf("tMPI_Request=%zu/%zu tMPI_Op=%zu/%zu tMPI_Errhandler=%zu/%zu tMPI_Status=%zu/%zu\n",
              sizeof(tMPI_Request), alignof(tMPI_Request), sizeof(tMPI_Op), alignof(tMPI_Op),
              sizeof(tMPI_Errhandler), alignof(tMPI_Errhandler), sizeof(tMPI_Status), alignof(tMPI_Status));
  return 0;
}
}
int main() { return PLMD::run_probe(); }
EOF
echo "[gromacs-tmpi] tag=$tag commit=$actual"
if ${CXX:-c++} -std=c++17 -Wall -Wextra -fsyntax-only \
   -I"$work/gromacs/src/external/thread_mpi/include" "$work/tmpi-signature.cpp" &&
   ${CXX:-c++} -std=c++17 -Wall -Wextra \
   -I"$work/gromacs/src/external/thread_mpi/include" "$work/tmpi-signature.cpp" \
   -o "$work/tmpi-layout"; then
  echo "[gromacs-tmpi] verified Error_string and Create_errhandler callback declarations"
  "$work/tmpi-layout"
else
  echo "[gromacs-tmpi] unavailable: pinned header signature/layout probe failed"
fi
