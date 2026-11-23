#!/usr/bin/env bash
set -euo pipefail
# Conda relocates this embedded host-prefix path into each installed environment.
# PLUMED_KERNEL remains an explicit user override of the default.
export CXXFLAGS="${CXXFLAGS:-} -D__PLUMED_DEFAULT_KERNEL=$PREFIX/lib/libplumedKernel$SHLIB_EXT"
options=()
case "$target_platform" in
  linux-64|osx-64) options+=(-DGMX_SIMD=SSE2) ;;
  linux-aarch64|osx-arm64) options+=(-DGMX_SIMD=ARM_NEON_ASIMD) ;;
  *) echo "Unsupported platform: $target_platform" >&2; exit 1 ;;
esac
if [[ $GROMACS_MPI == openmpi ]]; then
  options+=(-DGMX_MPI=ON -DGMX_THREAD_MPI=OFF
    "-DMPI_C_COMPILER=$PREFIX/bin/mpicc" "-DMPI_CXX_COMPILER=$PREFIX/bin/mpicxx")
else
  options+=(-DGMX_MPI=OFF -DGMX_THREAD_MPI=ON)
fi
if [[ $GROMACS_GPU == cuda ]]; then
  options+=(-DGMX_GPU=CUDA -DCMAKE_CUDA_ARCHITECTURES='75-real;80-real;86-real;89-real;90')
else
  options+=(-DGMX_GPU=OFF)
fi
if [[ $(uname) == Darwin ]]; then
  export CXXFLAGS="${CXXFLAGS:-} -D_LIBCPP_DISABLE_AVAILABILITY"
fi
marker=$(python -c 'import json; d=json.load(open("plumed-provenance.json")); print("PLUMED_"+d["snapshot_date"]+"_g"+d["plumed_commit"][:12])')
cmake -S . -B build -G Ninja ${CMAKE_ARGS:-} \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  -DGMX_VERSION_STRING_OF_FORK="$marker" \
  -DGMX_OPENMP=ON -DGMX_USE_PLUMED=ON -DGMX_BUILD_OWN_FFTW=OFF \
  -DGMX_FFT_LIBRARY=fftw3 -DGMX_USE_HWLOC=ON -DGMX_PREFER_STATIC_LIBS=OFF \
  -DBUILD_SHARED_LIBS=ON -DGMXAPI=OFF -DBUILD_TESTING=OFF \
  -DGMX_BUILD_UNITTESTS=OFF -DGMX_BUILD_HELP=OFF -DREGRESSIONTEST_DOWNLOAD=OFF \
  -DGMX_USE_RDTSCP=OFF "${options[@]}"
cmake --build build -j"${CPU_COUNT:-4}"
cmake --install build
mkdir -p "$PREFIX/share/gromacs-plumed"
cp plumed-provenance.json "$PREFIX/share/gromacs-plumed/provenance.json"
cp applied-plumed.patch "$PREFIX/share/gromacs-plumed/"
