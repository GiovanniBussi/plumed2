#!/usr/bin/env bash
set -euo pipefail
export CXXFLAGS="${CXXFLAGS:-} -O2"
if [[ $(uname) == Darwin ]]; then
  export CXXFLAGS="$CXXFLAGS -D_LIBCPP_DISABLE_AVAILABILITY -Xpreprocessor -fopenmp"
  export LDFLAGS="${LDFLAGS:-} -L$PREFIX/lib -lomp"
else
  export CXXFLAGS="$CXXFLAGS -fopenmp"
  export LDFLAGS="${LDFLAGS:-} -fopenmp"
  export LIBS="-lpthread -lrt"
fi
export CPPFLAGS="${CPPFLAGS:-} -D__PLUMED_DEFAULT_KERNEL=$PREFIX/lib/libplumedKernel$SHLIB_EXT"
./configure --prefix="$PREFIX" --disable-mpi --disable-libsearch --disable-python \
  --disable-doc --disable-static-patch --disable-static-archive --disable-basic-warnings \
  --disable-dependency-tracking \
  --enable-modules=none:cltools:colvar:bias:generic:setup:function:vatom
make -j"${CPU_COUNT:-4}"
make install
mkdir -p "$PREFIX/share/plumed-gromacs-build"
cp conda/gromacs/manifest.json "$PREFIX/share/plumed-gromacs-build/provenance.json"
