# Embedded thread-MPI

This module contains the namespaced GROMACS 2026.4 thread-MPI import pinned in
[`UPSTREAM.txt`](UPSTREAM.txt), with original source hashes in
[`UPSTREAM.sha256`](UPSTREAM.sha256). The import script and standalone tests
are flat, non-production inputs; `CMakeLists.txt` copies `.cpp.in` test inputs
to the build directory so production module builds compile only upstream
implementation sources.

From the repository root, check the import reproducibility and runtime tests
with:

```sh
python3 src/thread_mpi/reimport.py /path/to/gromacs --check
cmake -S src/thread_mpi -B /tmp/thread-mpi-build
cmake --build /tmp/thread-mpi-build
ctest --test-dir /tmp/thread-mpi-build --output-on-failure
python3 src/thread_mpi/check-symbol-isolation.py /path/to/gromacs
```

MPI provider design, wrapper behavior, and validation limits are documented in
[`../mpiwrap/README.md`](../mpiwrap/README.md).
