#!/bin/sh
set -eu

module_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mpich_cc=${MPICH_CC_WRAPPER:-/Users/bussi/miniconda3/envs/mpich/bin/mpicc}
mpich_cxx=${MPICH_CXX_WRAPPER:-/Users/bussi/miniconda3/envs/mpich/bin/mpicxx}
ompi_cc=${OMPI_CC_WRAPPER:-/Users/bussi/miniconda3/envs/openmpi/bin/mpicc}
ompi_cxx=${OMPI_CXX_WRAPPER:-/Users/bussi/miniconda3/envs/openmpi/bin/mpicxx}
clang_c=${CLANG_C:-/usr/bin/clang}
clang_cxx=${CLANG_CXX:-/usr/bin/clang++}
tmp=$(mktemp -d "${TMPDIR:-/tmp}/mpiw-native-adapter.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

for provider in mpich openmpi; do
  case $provider in
    mpich) cc=$mpich_cc; cxx=$mpich_cxx; export MPICH_CC=$clang_c MPICH_CXX=$clang_cxx ;;
    openmpi) cc=$ompi_cc; cxx=$ompi_cxx; export OMPI_CC=$clang_c OMPI_CXX=$clang_cxx ;;
  esac
  extra_cflags=
  if [ "$provider" = mpich ]; then extra_cflags=-DPLUMED_MPIW_EXPECT_NONNULL_STATUS_IGNORE=1; fi
  for capacity in default 16; do
    capacity_flag=
    if [ "$capacity" = 16 ]; then capacity_flag=-DPLUMED_MPIW_HANDLE_CAPACITY=16; fi
    "$cc" -std=c99 -Wall -Wextra -Werror $extra_cflags $capacity_flag -I"$module_dir" -x c \
      "$module_dir/test-native-adapter.c.in" -o "$tmp/test-$provider-$capacity-c"
    "$tmp/test-$provider-$capacity-c"
    "$cc" -x c -std=c99 -Wall -Wextra -Werror $capacity_flag -DPLUMED_MPIW_NATIVE_ADAPTER=1 -I"$module_dir" -c \
      "$module_dir/mpiwrap_implementation.cpp" -o "$tmp/adapter-$provider-$capacity-c99.o"
    "$cxx" -std=c++98 -Wall -Wextra -Werror $capacity_flag -DPLUMED_MPIW_NATIVE_ADAPTER=1 -I"$module_dir" -c \
      "$module_dir/mpiwrap_implementation.cpp" -o "$tmp/adapter-$provider-$capacity-cxx98.o"
  done
done
