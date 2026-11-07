#!/usr/bin/env bash
# Build an already patched LAMMPS tree against the serial PLUMED installation.
set -euo pipefail

source_dir=${1:?Usage: build-lammps.sh SOURCE BUILD PLUMED_PREFIX}
build_dir=${2:?Usage: build-lammps.sh SOURCE BUILD PLUMED_PREFIX}
plumed_prefix=${3:?Usage: build-lammps.sh SOURCE BUILD PLUMED_PREFIX}
: "${BACKEND:?Set BACKEND to openmpi or mpich}"
case "$BACKEND" in openmpi|mpich) ;; *) exit 2 ;; esac
options=()
if [[ -n "${MD_LIBOMP:-}" ]]; then
  options+=("-DOpenMP_CXX_FLAGS=-Xpreprocessor -fopenmp -I$MD_LIBOMP/include"
    -DOpenMP_CXX_LIB_NAMES=omp "-DOpenMP_omp_LIBRARY=$MD_LIBOMP/lib/libomp.dylib")
fi

export PKG_CONFIG_PATH="$plumed_prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
cmake -S "$source_dir/cmake" -B "$build_dir" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="${CXX:-c++}" \
  -DBUILD_MPI=ON "-DMPI_CXX_COMPILER=${MD_MPICXX:?Set MD_MPICXX}" \
  -DBUILD_OMP=ON -DPKG_OPENMP=ON -DPKG_PLUMED=ON \
  -DPLUMED_MODE=runtime -DDOWNLOAD_PLUMED=OFF -DDOWNLOAD_POTENTIALS=OFF \
  -DBUILD_SHARED_LIBS=OFF "${options[@]}"
cmake --build "$build_dir" -j "${MD_BUILD_JOBS:-4}"
"$build_dir/lmp" -help
