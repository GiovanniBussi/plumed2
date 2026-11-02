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
    "$cc" -std=c99 -Wall -Wextra -Werror $extra_cflags $capacity_flag -x c \
      "$module_dir/test-native-adapter.c.in" -o "$tmp/test-$provider-$capacity-c"
    "$tmp/test-$provider-$capacity-c"
    "$cc" -x c -std=c99 -Wall -Wextra -Werror $capacity_flag -DPLUMED_MPIW_NATIVE_ADAPTER=1 -c \
      "$module_dir/mpiwrap_implementation.cpp" -o "$tmp/adapter-$provider-$capacity-c99.o"
    "$cxx" -std=c++98 -Wall -Wextra -Werror $capacity_flag -DPLUMED_MPIW_NATIVE_ADAPTER=1 -c \
      "$module_dir/mpiwrap_implementation.cpp" -o "$tmp/adapter-$provider-$capacity-cxx98.o"
    for host_language in c cxx98; do
      case $host_language in
        c) host_compiler=$cc; host_standard=c99; host_input_language=c ;;
        cxx98) host_compiler=$cxx; host_standard=c++98; host_input_language=c++ ;;
      esac
      "$host_compiler" -std="$host_standard" -Wall -Wextra -Werror $capacity_flag \
        -D__PLUMED_WRAPPER_IMPLEMENTATION=1 -DPLUMED_MPIW_HOST_ADAPTER=1 \
        -D__PLUMED_HAS_DLOPEN=1 -iquote "$module_dir" -x "$host_input_language" -c \
        "$module_dir/test-host-header-adapter.c.in" \
        -o "$tmp/host-$provider-$capacity-$host_language.o"
    done
  done
done

if [ -n "${GMX_THREAD_MPI_INCLUDE:-}" ]; then
  for host_language in c cxx98; do
    case $host_language in
      c) host_compiler=$clang_c; host_standard=c99; host_input_language=c ;;
      cxx98) host_compiler=$clang_cxx; host_standard=c++98; host_input_language=c++ ;;
    esac
    "$host_compiler" -std="$host_standard" -Wall -Wextra -Werror \
      -DGMX_THREAD_MPI=1 -DPLUMED_MPIW_GROMACS_VERSION=20260000 \
      -DPLUMED_MPIW_TEST_THREAD_MPI=1 \
      -DSIZEOF_LONG_LONG_INT=8 -D__PLUMED_WRAPPER_IMPLEMENTATION=1 \
      -DPLUMED_MPIW_HOST_ADAPTER=1 -D__PLUMED_HAS_DLOPEN=1 \
      -I"$GMX_THREAD_MPI_INCLUDE" -iquote "$module_dir" -x "$host_input_language" -c \
      "$module_dir/test-host-header-adapter.c.in" \
      -o "$tmp/host-thread-mpi-$host_language.o"
  done
fi
