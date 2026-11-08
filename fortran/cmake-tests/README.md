# Consumer build checks

Use CMake 3.16 or newer and a configured/built PLUMED tree or installation:

```sh
cmake -S fortran/cmake-tests -B /path/to/consumer-build \
  -DPLUMED_CONFIG=/path/to/plumed/src/lib/Plumed.cmake.shared \
  -DCMAKE_Fortran_COMPILER=gfortran
cmake --build /path/to/consumer-build --parallel
ctest --test-dir /path/to/consumer-build --output-on-failure
```

The default builds both modules once and links two applications in a child
build directory. This checks transitive module paths, compilation ordering,
link dependencies, and the raw const-pointer error path. The procedural
application explicitly terminates its command key with `c_null_char`.

Repeat with separate build directories and `-DBINDING_MODULES=LEGACY`,
`-DBINDING_MODULES=F08`, or `-DBINDING_MODULES=DEFAULT` (the helper's default).
Linking configurations `.static`, `.runtime`, and the full `Plumed.cmake` can
also be tested. Runtime linking requires selecting an available kernel, e.g.:

```sh
PLUMED_KERNEL=/path/to/libplumedKernel.so \
  ctest --test-dir /path/to/runtime-consumer-build --output-on-failure
```

Use `.dylib` on macOS. Installed-source checks should also work with a read-only
PLUMED installation: all compiled objects and module files belong to the
consumer's build directory. Use a compatible C++/Fortran runtime combination
when linking a static kernel.

For a static Apple clang kernel with GNU Fortran, the Fortran link driver may
select an incompatible unwinder for C++ exceptions. Exercise the C++ link driver
with `-DTEST_CXX_LINKER=ON -DCMAKE_CXX_COMPILER=clang++`. If that compiler does
not supply a search path for the Fortran runtime, also set `CMAKE_BUILD_RPATH`
to its runtime library directory. This is consumer toolchain configuration;
the helper does not override the application's linker.
