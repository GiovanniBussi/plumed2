# Embedded thread-MPI

This module contains the namespaced GROMACS thread-MPI import. [`UPSTREAM.json`](UPSTREAM.json)
is the single provenance record: it pins the source tag and commit and records
SHA256 checksums for each original upstream file before import transformations.
The license is copied from the pinned upstream tree.

`reimport.py` discovers implementation sources and headers from the pinned
upstream directory, flattens them into this directory, rewrites internal
includes, and places C++ declarations and definitions in `PLMD::thread_mpi`.
When flattened basenames collide, it derives stable names from their source
roles and directory paths. Imported translation units retain the
`__PLUMED_HAS_THREAD_MPI` guard. The importer formats generated C++ and headers
with the project's required `astyle` options.

To verify the import against a local GROMACS checkout:

```sh
python3 src/thread_mpi/reimport.py /path/to/gromacs --check
```

To update the import, edit the `tag` and `commit` fields in `UPSTREAM.json`,
then rerun the script without `--check`. It discovers source files, refreshes
the per-file checksums in the same provenance record, and regenerates the
import. Review any build or source-layout changes in the upstream release.

The standalone runtime tests can be built with:

```sh
cmake -S src/thread_mpi -B /tmp/thread-mpi-build
cmake --build /tmp/thread-mpi-build
ctest --test-dir /tmp/thread-mpi-build --output-on-failure
python3 src/thread_mpi/check-symbol-isolation.py /path/to/gromacs
```

MPI provider design, wrapper behavior, and validation limits are documented in
[`../mpiwrap/README.md`](../mpiwrap/README.md).
