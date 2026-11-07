#!/usr/bin/env bash
# Build only the existing sander MPI executable, using the selected host MPI.
set -euo pipefail

source_dir=${1:?Usage: build-amber.sh SOURCE BUILD}
build_dir=${2:?Usage: build-amber.sh SOURCE BUILD}
: "${MD_MPICC:?Set MD_MPICC to the selected MPI C wrapper}"
: "${MD_MPICXX:?Set MD_MPICXX to the selected MPI C++ wrapper}"
: "${MD_MPIFC:?Set MD_MPIFC to the selected MPI Fortran wrapper}"

# MPI discovery must probe the same compilers that build Amber, including the
# Fortran module ABI. This also supports conda wrappers whose default driver
# names are unavailable outside their original build environment.
export MPICH_CC="${CC:-cc}" MPICH_CXX="${CXX:-c++}" MPICH_FC="${FC:-gfortran}"
export OMPI_CC="$MPICH_CC" OMPI_CXX="$MPICH_CXX" OMPI_FC="$MPICH_FC"

cmake -S "$source_dir" -B "$build_dir" \
  -DCMAKE_C_COMPILER="${CC:-cc}" \
  -DCMAKE_CXX_COMPILER="${CXX:-c++}" \
  -DCMAKE_Fortran_COMPILER="${FC:-gfortran}" \
  -DMPI_C_COMPILER="$MD_MPICC" \
  -DMPI_CXX_COMPILER="$MD_MPICXX" \
  -DMPI_Fortran_COMPILER="$MD_MPIFC" \
  -DCOMPILER=MANUAL -DMPI=TRUE -DCUDA=FALSE \
  -DBUILD_PYTHON=FALSE -DCHECK_UPDATES=FALSE -DINSTALL_TESTS=FALSE \
  -DDOWNLOAD_MINICONDA=FALSE -DFORCE_DISABLE_LIBS=boost \
  -DTRUST_SYSTEM_LIBS=TRUE -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$build_dir/install"
cmake --build "$build_dir" --target sander.MPI --parallel "${MD_BUILD_JOBS:-4}"
